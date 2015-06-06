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
 * <b>FAT filesystem:</b>
 *
 * Implementation of FAT filesystem from <a href="http://elm-chan.org/fsw/ff/00index_e.html">elm-chan.</a>
 * Currently only readonly mode is used and application must provide
 * functions like disk_initialize, disk_read and disk_status that handle
 * access to real hardware (like SD-card for example).
 */

/** @defgroup api   &mu;-layer API */
/** @defgroup config   Configuration */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "uoscfg.h"
#include <stdint.h>

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

#if UOSCFG_MAX_OPEN_FILES > 0 || DOX == 1

struct _uosFile;
struct _uosMount;

/**
 * @ingroup api Macro for defining a table of objects where used/free
 * status is managed by separate bitmap for efficient
 * space usage.
 */
#define UOS_BITTAB_TABLE(type, size) \
  typedef struct { \
     uint8_t  bitmap[size / 8 + 1]; \
     type table[size]; \
  } type##Bitmap

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
#define UOS_BITTAB_FREE(bm, slot) uosBitTabFree(bm.bitmap, slot);

/**
 * Allocate entry from bitmap table.
 */
int uosBitTabAlloc(uint8_t* bitmap, int size);

/**
 * Free an entry that was allocated from bitmap table.
 */
void uosBitTabFree(uint8_t* bitmap, int slot);

/**
 * File information.
 */
typedef struct {
  bool   isDir;
  size_t size;
} UosFileInfo;

/**
 * Structure for filesystem type. Provides function pointers
 * for common operations like read, write & close.
 */
typedef struct {

  int (*init)(const struct _uosMount* mount);
  int (*open)(struct _uosFile* file, const char* filename, int flags, int mode);
  int (*read)(struct _uosFile* file, char* buf, int max);
  int (*write)(struct _uosFile* file, const char* buf, int len);
  int (*close)(struct _uosFile* file);
  int (*stat)(const struct _uosMount* mount, const char* filename, UosFileInfo* st);
} UosFS;

/*
 * Mount table entry.
 */
typedef struct _uosMount {
  const char* mountPoint;
  const UosFS* fs;
  const char* dev;
} UosMount;

/*
 * Structure for open file descriptor.
 */
typedef struct _uosFile {

  const UosMount* mount;
  union {

    void* fsobj;
    int   fsfd;
  } u;
    
} UosFile;

/**
 * Initialize fs layer. Called automatically by uosInit().
 */

void uosFileInit(void);

/**
 * Mount a filesystem.
 */

int uosMount(const UosMount* mount);

/**
 * Convert file object into traditional fd number.
 */
int uosFileSlot(UosFile* file);

/**
 * Convert traditional fd number into file object.
 */
UosFile* uosFile(int fd);

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

#if UOSCFG_FAT > 0

extern const UosFS uosFatFS;

#endif
#endif

#if UOSCFG_FS_ROM > 0 || DOX == 1

typedef struct {
  const char* fileName;
  const uint8_t* contents;
  int       size;
} UosRomFile;

extern const UosFS uosRomFS;

#endif

/**
 * Initialize newlib syscall layer.
 */
void uosNewlibInit(void);

/** @} */

#if UOSCFG_NEWLIB_SYSCALLS == 1
int fsync(int);
#endif

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */
#endif
