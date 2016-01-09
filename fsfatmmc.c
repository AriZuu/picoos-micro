/*-----------------------------------------------------------------------*/
/* MMCv3/SDv1/SDv2 (in SPI mode) control module                          */
/*-----------------------------------------------------------------------*/
/*
 /  Copyright (C) 2014, ChaN, all right reserved.
 /
 / * This software is a free software and there is NO WARRANTY.
 / * No restriction on use. You can use, modify and redistribute it for
 /   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 / * Redistributions of source code must retain the above copyright notice.
 /
 /-------------------------------------------------------------------------
 */

/*
 * This has been modified & reformatted for Pico]OS micro layer by
 * Ari Suutari (ari@stonepile.fi)
 */

#include <picoos.h>
#include <picoos-u.h>

#if UOSCFG_FAT_MMC > 0

#include <stdbool.h>
#include "ff.h"
#include "diskio.h"

/* Definitions for MMC/SDC command */

#define CMD0    (0)       /* GO_IDLE_STATE */
#define CMD1    (1)       /* SEND_OP_COND (MMC) */
#define ACMD41  (0x80+41) /* SEND_OP_COND (SDC) */
#define CMD8    (8)       /* SEND_IF_COND */
#define CMD9    (9)       /* SEND_CSD */
#define CMD10   (10)      /* SEND_CID */
#define CMD12   (12)      /* STOP_TRANSMISSION */
#define ACMD13  (0x80+13) /* SD_STATUS (SDC) */
#define CMD16   (16)      /* SET_BLOCKLEN */
#define CMD17   (17)      /* READ_SINGLE_BLOCK */
#define CMD18   (18)      /* READ_MULTIPLE_BLOCK */
#define CMD23   (23)      /* SET_BLOCK_COUNT (MMC) */
#define ACMD23  (0x80+23) /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24   (24)      /* WRITE_BLOCK */
#define CMD25   (25)      /* WRITE_MULTIPLE_BLOCK */
#define CMD32   (32)      /* ERASE_ER_BLK_START */
#define CMD33   (33)      /* ERASE_ER_BLK_END */
#define CMD38   (38)      /* ERASE */
#define CMD55   (55)      /* APP_CMD */
#define CMD58   (58)      /* READ_OCR */

#define TMO(ms) (jiffies + MS(ms))
#define EXPIRED(tm) POS_TIMEAFTER(jiffies, tm)

static volatile DSTATUS Stat = STA_NOINIT;  /* Disk status */
static BYTE CardType;                       /* b0:MMC, b1:SDC, b2:Block addressing */

static int diskInit(const UosDisk* disk);
static int diskStatus(const UosDisk* disk);
static int diskRead(const UosDisk* disk, uint8_t* buff, int sector, int count);

#if _FS_READONLY != 1
static int diskWrite(const UosDisk* disk, const uint8_t* buff, int sector, int count);
#endif

static int diskIoctl(const UosDisk* disk, uint8_t cmd, void* buff);

const UosDiskConf uosMmcDiskConf = {

  .init   = diskInit,
  .status = diskStatus,
  .read   = diskRead,
#if _FS_READONLY != 1
  .write  = diskWrite,
#endif
  .ioctl  = diskIoctl
};

/*
 * Wait for card to be ready.
 * 1:Ready, 0:Timeout
 */
static int wait_ready(
    const UosMmcDisk* disk,
    UINT wt)        /* Timeout [ms] */
{
  BYTE d;
  JIF_t timeout = TMO(wt);

  do {

    d = uosSpiXchg(disk->dev, 0xff);

  } while (d != 0xFF && !EXPIRED(timeout));

  return (d == 0xFF) ? 1 : 0;
}

/*
 * Deselect the card and release SPI bus.
 */

static void deselect(const UosMmcDisk* disk)
{
  uosSpiCS(disk->dev, false);      /* Set CS# high */
  uosSpiXchg(disk->dev, 0xFF);     /* Dummy clock (force DO hi-z for multiple slave SPI) */
}


/*
 * Select the card and wait for ready
 * 1:Successful, 0:Timeout
 */
static int select(const UosMmcDisk* disk)
{
  uosSpiCS(disk->dev, true); /* Set CS# low */
  uosSpiXchg(disk->dev, 0xFF);                 /* Dummy clock (force DO enabled) */
  if (wait_ready(disk, 500))                   /* Wait for card ready */
    return 1;

  deselect(disk);
  return 0;                 /* Timeout */
}

/*
 * Receive a data block from MMC.
 */
static int rcvr_datablock(
    const UosMmcDisk* disk,
    BYTE *buff,       /* Data buffer to store received data */
    UINT btr)         /* Byte count (must be multiple of 4) */
{
  BYTE token;
  JIF_t timeout = TMO(200);

  do {                        /* Wait for data packet in timeout of 200ms */

    token = uosSpiXchg(disk->dev, 0xFF);

  } while ((token == 0xFF) && !EXPIRED(timeout));

  if (token != 0xFE)
    return 0;                 /* If not valid data token, retutn with error */

  uosSpiRcvr(disk->dev, buff, btr);       /* Receive the data block into buffer */
  uosSpiXchg(disk->dev, 0xFF);            /* Discard CRC */
  uosSpiXchg(disk->dev, 0xFF);

  return 1;                   /* Return with success */
}

/*
 * Send block to MMC.
 */

#if _FS_READONLY != 1

static int xmit_datablock(
    const UosMmcDisk* disk,
    const BYTE *buff, /* 512 byte data block to be transmitted */
    BYTE token)       /* Data/Stop token */
{
  BYTE resp;

  if (!wait_ready(disk, 500))
    return 0;

  uosSpiXchg(disk->dev, token);             /* Xmit data token */
  if (token != 0xFD) {                      /* Is data token */

    uosSpiXmit(disk->dev, buff, 512);       /* Xmit the data block to the MMC */
    uosSpiXchg(disk->dev, 0xFF);            /* CRC (Dummy) */
    uosSpiXchg(disk->dev, 0xFF);

    resp = uosSpiXchg(disk->dev, 0xFF);     /* Reveive data response */
    if ((resp & 0x1F) != 0x05)              /* If not accepted, return with error */
      return 0;
  }

  return 1;
}
#endif

/*
 * Send a command packet to MMC.
 * Returns R1 resp (bit7==1:Send failed)
 */
static BYTE send_cmd(
    const UosMmcDisk* disk,
    BYTE cmd,     /* Command index */
    DWORD arg)    /* Argument */
{
  BYTE n, res;

  if (cmd & 0x80) { /* ACMD<n> is the command sequense of CMD55-CMD<n> */

    cmd &= 0x7F;
    res = send_cmd(disk, CMD55, 0);
    if (res > 1)
      return res;
  }

  /* Select the card and wait for ready except to stop multiple block read */
  if (cmd != CMD12) {

    deselect(disk);
    if (!select(disk))
      return 0xFF;
  }

  /* Send command packet */
  uosSpiXchg(disk->dev, 0x40 | cmd);                /* Start + Command index */
  uosSpiXchg(disk->dev, (BYTE) (arg >> 24));        /* Argument[31..24] */
  uosSpiXchg(disk->dev, (BYTE) (arg >> 16));        /* Argument[23..16] */
  uosSpiXchg(disk->dev, (BYTE) (arg >> 8));         /* Argument[15..8] */
  uosSpiXchg(disk->dev, (BYTE) arg);                /* Argument[7..0] */
  n = 0x01;                             /* Dummy CRC + Stop */

  if (cmd == CMD0)
    n = 0x95;                           /* Valid CRC for CMD0(0) + Stop */

  if (cmd == CMD8)
    n = 0x87;                           /* Valid CRC for CMD8(0x1AA) Stop */

  uosSpiXchg(disk->dev, n);

  /* Receive command response */
  if (cmd == CMD12)
    uosSpiXchg(disk->dev, 0xFF);          /* Skip a stuff byte when stop reading */

  n = 10;                               /* Wait for a valid response in timeout of 10 attempts */
  do {

    res = uosSpiXchg(disk->dev, 0xFF);

  } while ((res & 0x80) && --n);

  return res;                           /* Return with the response value */
}

/*
 * Initialize Disk Drive.
 */
static int diskInit(const UosDisk* adisk)
{
  const UosMmcDisk* disk = (const UosMmcDisk*)adisk;
  BYTE n, cmd, ty, ocr[4];

//  if (pdrv)
//    return STA_NOINIT;        /* Supports only single drive */

  disk->cf->close(disk);            /* Turn off the socket power to reset the card */
  if (Stat & STA_NODISK)
    return Stat;                /* No card in the socket */

  uosSpiBeginNoCS(disk->dev);
  uosSpiControl(disk->dev->bus, false);

  disk->cf->open(disk);             /* Turn on the socket power */

  for (n = 10; n; n--)
    uosSpiXchg(disk->dev, 0xFF);      /* 80 dummy clocks */

  ty = 0;
  if (send_cmd(disk, CMD0, 0) == 1) { /* Enter Idle state */

    JIF_t timeout = TMO(1000);
    if (send_cmd(disk, CMD8, 0x1AA) == 1) { /* SDv2? */

      for (n = 0; n < 4; n++)
        ocr[n] = uosSpiXchg(disk->dev, 0xFF); /* Get trailing return value of R7 resp */

      if (ocr[2] == 0x01 && ocr[3] == 0xAA) { /* The card can work at vdd range of 2.7-3.6V */

        while (!EXPIRED(timeout) && send_cmd(disk, ACMD41, 1UL << 30))
          ; /* Wait for leaving idle state (ACMD41 with HCS bit) */

        if (!EXPIRED(timeout) && send_cmd(disk, CMD58, 0) == 0) { /* Check CCS bit in the OCR */

          for (n = 0; n < 4; n++)
            ocr[n] = uosSpiXchg(disk->dev, 0xFF);

          ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; /* SDv2 */
        }
      }
    }
    else { /* SDv1 or MMCv3 */

      if (send_cmd(disk, ACMD41, 0) <= 1) {

        ty = CT_SD1;
        cmd = ACMD41; /* SDv1 */
      }
      else {

        ty = CT_MMC;
        cmd = CMD1; /* MMCv3 */
      }

      while (!EXPIRED(timeout) && send_cmd(disk, cmd, 0))
        ; /* Wait for leaving idle state */

      if (!!EXPIRED(timeout) || send_cmd(disk, CMD16, 512) != 0) /* Set R/W block length to 512 */
        ty = 0;
    }
  }

  CardType = ty;
  uosSpiControl(disk->dev->bus, true);
  deselect(disk);

  if (ty) { /* Initialization succeded */

    Stat &= ~STA_NOINIT; /* Clear STA_NOINIT */
  }
  else { /* Initialization failed */

    disk->cf->close(disk);
  }

  uosSpiEnd(disk->dev);
  return Stat;
}

/*
 * Get Disk Status
 */
static int diskStatus(const UosDisk* disk)
{
//  if (pdrv)
//    return STA_NOINIT; /* Supports only single drive */

  return Stat;
}

/*
 * Read Sector(s)
 */
static int diskRead(
    const UosDisk* adisk,     /* Physical drive */
    uint8_t *buff,            /* Pointer to the data buffer to store read data */
    int sector,               /* Start sector number (LBA) */
    int count)                /* Sector count (1..128) */
{
  const UosMmcDisk* disk = (const UosMmcDisk*)adisk;
  BYTE cmd;

  //if (pdrv || !count)
  if (!count)
    return RES_PARERR;

  if (Stat & STA_NOINIT)
    return RES_NOTRDY;

  uosSpiBeginNoCS(disk->dev);
  if (!(CardType & CT_BLOCK))
    sector *= 512;                  /* Convert to byte address if needed */

  cmd = count > 1 ? CMD18 : CMD17;  /*  READ_MULTIPLE_BLOCK : READ_SINGLE_BLOCK */
  if (send_cmd(disk, cmd, sector) == 0) {

    do {

      if (!rcvr_datablock(disk, buff, 512))
        break;

      buff += 512;
    } while (--count);

    if (cmd == CMD18)
      send_cmd(disk, CMD12, 0);            /* STOP_TRANSMISSION */
  }

  deselect(disk);

  uosSpiEnd(disk->dev);
  return count ? RES_ERROR : RES_OK;
}

/*
 * Write Sector(s)
 */
#if _FS_READONLY != 1
int diskWrite(
    const UosDisk* adisk,  /* Physical drive */
    const uint8_t *buff,   /* Pointer to the data to be written */
    int sector,            /* Start sector number (LBA) */
    int count)             /* Sector count (1..128) */
{
  const UosMmcDisk* disk = (const UosMmcDisk*)adisk;
  //if (pdrv || !count)
  if (!count)
    return RES_PARERR;

  if (Stat & STA_NOINIT)
    return RES_NOTRDY;

  if (Stat & STA_PROTECT)
    return RES_WRPRT;

  uosSpiBeginNoCS(disk->dev);

  if (!(CardType & CT_BLOCK))
    sector *= 512; /* Convert to byte address if needed */

  if (count == 1) { /* Single block write */

    if ((send_cmd(disk, CMD24, sector) == 0) /* WRITE_BLOCK */
        && xmit_datablock(disk, buff, 0xFE))
    count = 0;
  }
  else { /* Multiple block write */

    if (CardType & CT_SDC)
      send_cmd(disk, ACMD23, count);

    if (send_cmd(disk, CMD25, sector) == 0) { /* WRITE_MULTIPLE_BLOCK */

      do {

        if (!xmit_datablock(disk, buff, 0xFC))
          break;

        buff += 512;

      } while (--count);

      if (!xmit_datablock(disk, 0, 0xFD)) /* STOP_TRAN token */
        count = 1;
    }
  }

  deselect(disk);
  uosSpiEnd(disk->dev);

  return count ? RES_ERROR : RES_OK;
}
#endif

/*
 * Miscellaneous Functions
 */
#if _USE_IOCTL
static int diskIoctl(
    const UosDisk* adisk,   /* Physical drive */
    uint8_t cmd,            /* Control code */
    void *buff)             /* Buffer to send/receive control data */
{
  const UosMmcDisk* disk = (const UosMmcDisk*)adisk;
  DRESULT res;
  BYTE n, csd[16], *ptr = buff;
  DWORD csize;

//  if (pdrv)
 //   return RES_PARERR;

  res = RES_ERROR;

  if (Stat & STA_NOINIT)
    return RES_NOTRDY;

  uosSpiBeginNoCS(disk->dev);
  switch (cmd)
  {
  case CTRL_SYNC: /* Make sure that no pending write process. Do not remove this or written sector might not left updated. */
    if (select(disk))
      res = RES_OK;

    break;

  case GET_SECTOR_COUNT: /* Get number of sectors on the disk (DWORD) */
    if ((send_cmd(disk, CMD9, 0) == 0) && rcvr_datablock(disk, csd, 16)) {

      if ((csd[0] >> 6) == 1) { /* SDC ver 2.00 */

        csize = csd[9] + ((WORD) csd[8] << 8) + ((DWORD) (csd[7] & 63) << 16) + 1;
        *(DWORD*) buff = csize << 10;
      }
      else { /* SDC ver 1.XX or MMC*/

        n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
        csize = (csd[8] >> 6) + ((WORD) csd[7] << 2) + ((WORD) (csd[6] & 3) << 10) + 1;
        *(DWORD*) buff = csize << (n - 9);
      }

      res = RES_OK;
    }
    break;

  case GET_BLOCK_SIZE: /* Get erase block size in unit of sector (DWORD) */
    if (CardType & CT_SD2) { /* SDv2? */

      if (send_cmd(disk, ACMD13, 0) == 0) { /* Read SD status */

        uosSpiXchg(disk->dev, 0xFF);
        if (rcvr_datablock(disk, csd, 16)) { /* Read partial block */

          for (n = 64 - 16; n; n--)
            uosSpiXchg(disk->dev, 0xFF); /* Purge trailing data */

          *(DWORD*) buff = 16UL << (csd[10] >> 4);
          res = RES_OK;
        }
      }
    }
    else { /* SDv1 or MMCv3 */

      if ((send_cmd(disk, CMD9, 0) == 0) && rcvr_datablock(disk, csd, 16)) { /* Read CSD */

        if (CardType & CT_SD1) { /* SDv1 */

          *(DWORD*) buff = (((csd[10] & 63) << 1) + ((WORD) (csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
        }
        else { /* MMCv3 */

          *(DWORD*) buff = ((WORD) ((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
        }

        res = RES_OK;
      }
    }
    break;

    /* Following commands are never used by FatFs module */

  case MMC_GET_TYPE: /* Get card type flags (1 byte) */
    *ptr = CardType;
    res = RES_OK;
    break;

  case MMC_GET_CSD: /* Receive CSD as a data block (16 bytes) */
    if (send_cmd(disk, CMD9, 0) == 0 /* READ_CSD */ &&
        rcvr_datablock(disk, ptr, 16))
      res = RES_OK;

    break;

  case MMC_GET_CID: /* Receive CID as a data block (16 bytes) */
    if (send_cmd(disk, CMD10, 0) == 0 /* READ_CID */ &&
        rcvr_datablock(disk, ptr, 16))
      res = RES_OK;

    break;

  case MMC_GET_OCR: /* Receive OCR as an R3 resp (4 bytes) */
    if (send_cmd(disk, CMD58, 0) == 0) { /* READ_OCR */

      for (n = 4; n; n--)
        *ptr++ = uosSpiXchg(disk->dev, 0xFF);

      res = RES_OK;
    }
    break;

  case MMC_GET_SDSTAT: /* Receive SD statsu as a data block (64 bytes) */
    if (send_cmd(disk, ACMD13, 0) == 0) { /* SD_STATUS */

      uosSpiXchg(disk->dev, 0xFF);
      if (rcvr_datablock(disk, ptr, 64))
        res = RES_OK;
    }
    break;

  default:
    res = RES_PARERR;
  }

  deselect(disk);
  uosSpiEnd(disk->dev);
  return res;
}
#endif
#endif
