/*
 * Copyright (c) 2012-2014, Ari Suutari <ari@stonepile.fi>.
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

#ifndef _PICOOS_U_H
#define _PICOOS_U_H
/**
 * @file    picoos-u.h
 * @brief   Include file of u-layer library for pico]OS
 * @author  Ari Suutari <ari@stonepile.fi>
 */

/**
 * @mainpage picoos-micro - &mu;-layer for pico]OS
 * <b> Table Of Contents </b>
 * - @ref api
 * - @ref config
 * @section overview Overview
 * This library contains miscellaneous routines built on pico]OS pico & nano layers.
 *
 * @subsection features Features
 * <b>Microsecond delay:</b>
 *
 * Implementation of microsecond delay using a spin-loop. Depending on CPU it uses either 
 * simple delay loop or hardware timer.
 * 
 * <b>UosFile API</b>
 *
 * Filesystem api which allows mounting different filesystems into directory tree.
 * This API provides also support for integrating newlib stdio features.
 *
 * <b>FAT filesystem:</b>
 *
 * Implementation of FAT filesystem from <a href="http://elm-chan.org/fsw/ff/00index_e.html">elm-chan.</a>
 * Integrated into UosFile filesystem API.
 *
 * <b>ROM filesystem:</b>
 *
 * Simple ROM filesystem. Simple array of filenames contains pointers
 * to file data in flash memory.
 *
 * <b>Ring buffer/mailbox</b>
 * 
 * Implementation of ring buffer that can be used also as mailbox.
 *
 * <b>Generic SPI bus</b>
 *
 * Simple interface to implement SPI bus that can be shared 
 * between tasks.
 */

/** @defgroup api   &mu;-layer API */
/** @defgroup config   Configuration */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "uoscfg.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef UOSCFG_MAX_MOUNT
#define UOSCFG_MAX_MOUNT 2
#endif

/**
 * @ingroup api
 * @{
 */

/**
 * Initialize &mu;-layer. Must be called before any other 
 * API function.
 */
void uosInit(void);

/**
 * Print memory sizes and required copyright messages when
 * system starts.
 */
void uosBootDiag(void);

/**
 * Print information about resource usage. Currently output
 * includes free stack space for each task, free interrupt
 * stack space and number of tasks and events in use.
 */
void uosResourceDiag(void);

/**
 * Initialize possible hardware timer for uosSpinUSecs. Called
 * internally by uosInit().
 */
void uosSpinInit(void);

#if defined(__MSP430__) && UOSCFG_SPIN_USECS == 2

#define uosSpinUSecs(t) __delay_cycles(PORTCFG_CPU_CLOCK_MHZ * (t))

#else

/** 
 * Spin in loop until requested number of microseconds have passed.
 * Uses either hardware timer or delay loop depending of
 * ::UOSCFG_SPIN_USECS setting.
 */
void uosSpinUSecs(uint16_t uSecs);

#endif

/** @} */

#if UOSCFG_SPI_BUS > 0 || DOX == 1

/**
 * @ingroup api
 * @{
 */

struct uosSpiBus;
struct uosSpiDev;

/**
 * Config for generic SPI bus.
 */
typedef struct uosSpiBusConf {

  void    (*init)(struct uosSpiBus* bus);
  void    (*control)(struct uosSpiBus* bus, bool fullSpeed);
  void    (*cs)(struct uosSpiBus* bus, bool select);
  uint8_t (*xchg)(const struct uosSpiBus* bus, uint8_t data);
  void    (*xmit)(const struct uosSpiBus*, const uint8_t* data, int len);
  void    (*rcvr)(const struct uosSpiBus*, uint8_t* data, int len);
} UosSpiBusConf;

/**
 * Structure for generic SPI bus.
 */
typedef struct uosSpiBus {

  const UosSpiBusConf* cf;
  POSMUTEX_t busMutex;
  struct uosSpiDev* currentDev;
  bool active;
} UosSpiBus;

/**
 * Config for generic SPI bus device.
 */
typedef struct __attribute__((aligned(4))) uosSpiDevConf {
// nothing here yet.
} UosSpiDevConf;

/**
 * Generic SPI bus device.
 */
typedef struct uosSpiDev {

  const UosSpiDevConf* cf;
  UosSpiBus* bus;
} UosSpiDev;

/**
 * Initialize SPI bus. Must be called before any other operations.
 */
void    uosSpiInit(UosSpiBus* bus);

/**
 * Initialize SPI device.
 */
void uosSpiDevInit(UosSpiDev* dev, const UosSpiDevConf* cf, UosSpiBus* bus);

/**
 * Control SPI bus speed (low or full).
 */
void    uosSpiControl(UosSpiBus* bus, bool fullSpeed);

/**
 * Allocate SPI bus for current task, but do not assert CS.
 */
void    uosSpiBeginNoCS(UosSpiDev* dev);

/**
 * Allocate SPI bus for current task and assert CS.
 */
void    uosSpiBegin(UosSpiDev* dev);

/**
 * Directly manipulate CS line. Call to uosSpiBegin is still required.
 */
void    uosSpiCS(UosSpiDev* dev, bool select);

/**
 * Exchange byte on SPI bus.
 */
uint8_t uosSpiXchg(UosSpiDev* dev, uint8_t data);

/**
 * Transmit multiple bytes on SPI bus.
 */
void    uosSpiXmit(UosSpiDev* dev, const uint8_t* data, int len);

/**
 * Receive multiple bytes from SPI bus.
 */
void    uosSpiRcvr(UosSpiDev* dev, uint8_t* data, int len);

/**
 * Free SPI bus from current task. If chip select was turned
 * low by uosSpiBegin, turn it high again.
 */
void    uosSpiEnd(UosSpiDev* dev);

/** @} */

#endif

#if UOSCFG_MAX_OPEN_FILES > 0 || DOX == 1

struct uosFile;
struct uosFS;
struct uosDisk;

/**
 * Macro for defining a table of objects where used/free
 * status is managed by separate bitmap for efficient
 * space usage.
 */
#define UOS_BITTAB_TABLE(type, size) \
  typedef struct { \
     uint8_t  bitmap[size / 8 + 1]; \
     type table[size]; \
  } type##Bittab

/**
 * Initialize bitmap table so that all elements
 * are marked free.
 */
#define UOS_BITTAB_INIT(bm) memset(&bm.bitmap, '\0', sizeof(bm.bitmap))

/**
 * Macro for converting address of table element into
 * table index.
 */
#define UOS_BITTAB_SLOT(bm, elem) (elem == NULL ? -1 : (elem - bm.table))

/**
 * Macro for converting table index into table element pointer.
 */
#define UOS_BITTAB_ELEM(bm, slot) (slot == -1 ? NULL : (bm.table + slot))

/**
 * Macro for allocating an entry from bitmap table.
 */
#define UOS_BITTAB_ALLOC(bm) uosBitTabAlloc(bm.bitmap, sizeof(bm.table)/sizeof(bm.table[0]))

/**
 * Macro for freeing a bitmap table entry.
 */
#define UOS_BITTAB_FREE(bm, slot) uosBitTabFree(bm.bitmap, slot)

/**
 * Macro for checking for free bitmap table entry.
 */
#define UOS_BITTAB_IS_FREE(bm, slot) uosBitTabIsFree(bm.bitmap, slot)

/**
 * Allocate entry from bitmap table.
 */
int uosBitTabAlloc(uint8_t* bitmap, int size);

/**
 * Free an entry that was allocated from bitmap table.
 */
void uosBitTabFree(uint8_t* bitmap, int slot);

/**
 * Check if entry is free in bitmap table.
 */
bool uosBitTabIsFree(uint8_t* bitmap, int slot);

/**
 * @ingroup api
 * @{
 */

/**
 * File information.
 */
typedef struct uosFileInfo {
  bool   isDir;
  bool   isSocket;
  int    size;
} UosFileInfo;

/**
 * Config for file operations. Provides function pointers
 * for common operations like read, write & close.
 */
typedef struct uosFileConf {

  int (*read)(struct uosFile* file, char* buf, int max);
  int (*write)(struct uosFile* file, const char* buf, int len);
  int (*close)(struct uosFile* file);
  int (*fstat)(struct uosFile* file, UosFileInfo* st);
  int (*lseek)(struct uosFile* file, int offset, int whence);
  int (*sync)(struct uosFile* file);
} UosFileConf;

/**
 * Config for filesystem type. Provides function pointers
 * for fs operations like open & unlink.
 */
typedef struct uosFSConf {

  int (*init)(const struct uosFS* mount);
  int (*open)(const struct uosFS* mount, struct uosFile* file, const char* filename, int flags, int mode);
  int (*stat)(const struct uosFS* mount, const char* filename, UosFileInfo* st);
  int (*unlink)(const struct uosFS* mount, const char* name);
} UosFSConf;

/**
 * Config for disk drives. Provides function
 * pointers for disk access.
 */
typedef struct uosDiskConf {

  int (*init)(const struct uosDisk* disk);
  int (*status)(const struct uosDisk* disk);
  int (*read)(const struct uosDisk* disk, uint8_t* buff, int sector, int count);
  int (*write)(const struct uosDisk* disk, const uint8_t* buff, int sector, int count);
  int (*ioctl)(const struct uosDisk* disk, uint8_t cmd, void* buff);
} UosDiskConf;

/**
 * Structure for disk drives.
 */
typedef struct uosDisk {

  const UosDiskConf* cf;
} UosDisk;

/**
 * Mount table entry.
 */
typedef struct uosFS {

  const UosFSConf*  cf;
  const char*     mountPoint;
} UosFS;

/**
 * Structure for open file descriptor.
 */
typedef struct uosFile {

  const UosFileConf*  cf;
  const UosFS*      fs;
  void*             fsPriv;
    
} UosFile;

/**
 * Initialize fs layer. Called automatically by uosInit().
 */

void uosFileInit(void);

/**
 * Convert file object into traditional fd number.
 */
int uosFileSlot(UosFile* file);

/**
 * Convert traditional fd number into file object.
 */
UosFile* uosFile(int fd);

/**
 * Perform internal filesystem mount.
 */
int uosMount(const UosFS* mount);

/**
 * Allocate file descriptor (internal use only).
 */
UosFile* uosFileAlloc(void);

/**
 * Free file descriptor (internal use only).
 */
int uosFileFree(UosFile* file);

/**
 * Open file from mounted filesystem.
 */
UosFile* uosFileOpen(const char* fileName, int flags,  int mode);

/**
 * Read from file.
 */
int uosFileRead(UosFile* file, char* buf, int max);

/**
 * Write to file.
 */
int uosFileWrite(UosFile* file, const char* buf, int len);

/**
 * Close file.
 */
int uosFileClose(UosFile* file);

/**
 * Get file information.
 */
int uosFileStat(const char* filename, UosFileInfo* st);

/**
 * Get file information.
 */
int uosFileFStat(UosFile* file, UosFileInfo* st);

/**
 * Seek.
 */
int uosFileSeek(UosFile* file, int offset, int whence);

/**
 * Remove file.
 */
int uosFileUnlink(const char* filename);

/**
 * Flush file to disk.
 */
int uosFileSync(UosFile* file);

/**
 * Add a known disk. Returns disk number.
 */
int uosAddDisk(const UosDisk* disk);

/**
 * Get disk by drive number.
 */
const UosDisk* uosGetDisk(int diskNumber);

#if UOSCFG_FAT > 0 || DOX == 1

/**
 * Mount a fat filesystem.
 */
int uosMountFat(const char* mountPoint, int diskNumber);

#if UOSCFG_FAT_MMC > 0 || DOX == 1

extern const UosDiskConf uosMmcDiskConf;

struct uosMmcDisk;

/**
 * Config for MMC/SD card SPI.
 */
typedef struct uosMmcSpiConf {

  void    (*open)(const struct uosMmcDisk* disk);
  void    (*close)(const struct uosMmcDisk* disk);
} UosMmcSpiConf;

/**
 * Disk using MMC/SD card via SPI bus.
 */
typedef struct uosMmcDisk {

  UosDisk base;
  const UosMmcSpiConf* cf;
  UosSpiDev* dev;
} UosMmcDisk;

/**
 * Simple implementation of SPI block transmit.
 */
void uosMmcSpiXmit(const UosMmcDisk*, const uint8_t* data, int len);

/**
 * Simple implementation of SPI block receive.
 */
void uosMmcSpiRcvr(const UosMmcDisk*, uint8_t* data, int len);

#endif
#endif

#if UOSCFG_FS_ROM > 0 || DOX == 1

typedef struct {
  const char* fileName;
  const uint8_t* contents;
  int       size;
} UosRomFile;

/**
 * Mount a rom filesystem.
 */
int uosMountRom(const char* mountPoint, const UosRomFile* data);

#endif

/** @} */

#endif

/**
 * Initialize newlib syscall layer.
 */
void uosNewlibInit(void);

#if UOSCFG_RING > 0 || DOX == 1

/**
 * @ingroup api
 * @{
 */

typedef struct uosRing UosRing;

/**
 * Create new ring buffer mailbox.
 */
UosRing* uosRingCreate(int msgSize, int msgCount);

/**
 * Put a message to ring buffer. Waits for timeout ticks
 * if there is not room. If called from interrupt handler,
 * timeout must be set to zero (don't wait).
 */
bool uosRingPut(UosRing* ring, const void *msg, UINT_t timeout);

/**
 * Get a message from ring buffer. Waits for timeout
 * ticks until there is a message available.
 * If timeout occurs, function returns false.
 */
bool uosRingGet(UosRing* ring, void *msg, UINT_t timeout);

/**
 * Destroy a ring buffer mailbox.
 */
void uosRingDestroy(UosRing* ring);

/** @} */

#endif


#if UOSCFG_NEWLIB_SYSCALLS == 1
int fsync(int);
#endif

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */
#endif
