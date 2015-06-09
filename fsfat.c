/*
 * Copyright (c) 2015, Ari Suutari <ari@stonepile.fi>.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT,  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <picoos.h>
#include <picoos-u.h>
#include <stdbool.h>
#include <string.h>

#if UOSCFG_FAT > 0

#if !defined(UOSCFG_MAX_OPEN_FILES) || UOSCFG_MAX_OPEN_FILES == 0
#error UOSCFG_MAX_OPEN_FILES must be > 0
#endif

#endif

#if UOSCFG_FAT > 0 && UOSCFG_MAX_OPEN_FILES > 0

#include <errno.h>

/*
 * FAT fs.
 */

#include <stdlib.h>

#include "ff.h"
#include "diskio.h"

UOS_BITTAB_TABLE(FIL, UOSCFG_FAT);
static FILBitmap openFiles;;

static int fatInit(const UosMount*);
static int fatOpen(UosFile* file, const char *name, int flags, int mode);
static int fatClose(UosFile* file);
static int fatRead(UosFile* file, char* buf, int max);
static int fatWrite(UosFile* file, const char* buf, int max);
static int fatStat(const UosMount* mount, const char* fn, UosFileInfo* st);

const UosFS uosFatFS = {

  .init  = fatInit,
  .open  = fatOpen,
  .close = fatClose,
  .read  = fatRead,
  .write = fatWrite,
  .stat  = fatStat
};

static FATFS fs;

static bool initialized = false;

static int fatInit(const UosMount* m)
{
  if (!initialized) {

    UOS_BITTAB_INIT(openFiles);

    f_mount(&fs, m->dev, 1);
    initialized = true;
  }
  else
    P_ASSERT("fatInit", false);

  return 0;
}

static int fatOpen(UosFile* file, const char *name, int flags, int mode)
{
  P_ASSERT("fatOpen", file->mount->fs == &uosFatFS);

// Find free FAT descriptor.

  int slot = UOS_BITTAB_ALLOC(openFiles);
  if (slot == -1) {

    nosPrintf("fatFs: table full\n");
    errno = EMFILE;
    return -1;
  }

  FIL* f = UOS_BITTAB_ELEM(openFiles, slot);

  FRESULT fr;
  char fullName[80];

  strcpy(fullName, file->mount->dev);
  strcat(fullName, name);

  file->u.fsobj = f;
  fr = f_open(f, fullName, FA_OPEN_EXISTING | FA_READ);
  if (fr == FR_OK)
    return 0;

  if (fr == FR_NO_FILE)
    errno = ENOENT;
  else if (fr == FR_NO_PATH)
    errno = ENOTDIR;
  else
    errno = EIO;
    
  UOS_BITTAB_FREE(openFiles, slot);
  return -1;
}

static int fatClose(UosFile* file)
{
  P_ASSERT("fatClose", file->mount->fs == &uosFatFS);

  FIL* f = (FIL*)file->u.fsobj;
  if (f_close(f) != 0) {

    errno = EIO;
    return -1;
  }

  UOS_BITTAB_FREE(openFiles, UOS_BITTAB_SLOT(openFiles, f));
  return 0;
}

static int fatRead(UosFile* file, char *buf, int len)
{
  P_ASSERT("fatRead", file->mount->fs == &uosFatFS);

  FIL* f = (FIL*)file->u.fsobj;

  FRESULT fr;
  UINT retLen;

  fr = f_read(f, buf, len, &retLen);
  if (fr != FR_OK) {

    errno = EIO;
    return -1;
  }

  return retLen;
}

static int fatWrite(UosFile* file, const char *buf, int len)
{
  P_ASSERT("fatRead", file->mount->fs == &uosFatFS);

//  FatFile* f = (FatFile*)file->u.fsobj;

  errno = EBADF;
  return -1;
}

static int fatStat(const UosMount* mount, const char* fn, UosFileInfo* st)
{
  FRESULT fr;
  FILINFO info;
  char fullName[80];

  strcpy(fullName, mount->dev);
  strcat(fullName, fn);

  fr = f_stat(fullName, &info);
  if (fr == FR_OK) {

    st->isDir = info.fattrib & AM_DIR;
    st->size = info.fsize;
    return 0;
  }

  if (fr == FR_NO_FILE)
    errno = ENOENT;
  else if (fr == FR_NO_PATH)
    errno = ENOTDIR;
  else
    errno = EIO;
    
  return -1;
}

static const UosDisk* driver = NULL;

/*
 * Set driver
 */
void uosSetDiskDriver(const UosDisk* d)
{
  driver = d;
}

/* 
 * Get disk status.
 */
DSTATUS disk_status(BYTE pdrv)
{
  if (driver == NULL)
    return STA_NOINIT;

  return driver->status(pdrv);
}

/* 
 * Initialize drive.
 */
DSTATUS disk_initialize(BYTE pdrv)
{
  if (driver == NULL)
    return STA_NOINIT;

  return driver->init(pdrv);
}

/*
 * Read sectors.
 */
DRESULT disk_read(BYTE pdrv,
	          BYTE *buff,
	          DWORD sector,
	          UINT count)
{
  if (driver == NULL)
    return STA_NOINIT;

  return driver->read(pdrv, buff, sector, count);
}

#if _FS_READONLY != 1

/*
 * Write sectors.
 */

DRESULT disk_write(BYTE pdrv,
	           const BYTE *buff,
	           DWORD sector,
	           UINT count)
{
  if (driver == NULL)
    return STA_NOINIT;

  return driver->write(pdrv, buff, sector, count);
}

DWORD __attribute__((weak)) get_fattime()
{
  return 0;
}

#endif

#if _USE_IOCTL

/* 
 * Ioctl.
 */
DRESULT disk_ioctl(BYTE pdrv,
	           BYTE cmd,
	           void *buff)
{
  if (driver == NULL)
    return STA_NOINIT;

  return driver->ioctl(pdrv, cmd, buff);
}
#endif

#if _FS_REENTRANT

/*
 * Create semaphore for FAT volume.
 */
int ff_cre_syncobj(BYTE vol, _SYNC_t* sem)
{
  *sem = posSemaCreate(1);
  if (*sem == NULL)
    return 0;

  return 1;
}

/*
 * Destroy FAT volume semaphore.
 */
int ff_del_syncobj(_SYNC_t sem)
{
  posSemaDestroy(sem);
  return 0;
}

/*
 * Try to lock volume semaphore.
 */
int ff_req_grant(_SYNC_t sem)
{
  if (posSemaWait(sem, _FS_TIMEOUT) == 0)
    return 1;

  return 0;
}

/*
 * Release volume semaphore.
 */
void ff_rel_grant(_SYNC_t sem)
{
  posSemaSignal(sem);
}

#endif

#if _USE_LFN == 3

/*
 * LFN with a working buffer on the heap
 */

void* ff_memalloc(UINT size)
{
  return nosMemAlloc(size);
}

void ff_memfree(void* ptr)
{
  nosMemFree(ptr);
}

#endif
#endif
