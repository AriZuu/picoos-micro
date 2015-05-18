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

#if UOSCFG_FAT > 0

#include <errno.h>

/*
 * FAT fs.
 */

#include <stdlib.h>

#include "ff.h"

/*
 * Open file table for FAT fs.
 */

typedef struct {

  FIL	file;
  bool  inUse;
} FatFile;

static FatFile	openFiles[UOSCFG_FAT];
static POSMUTEX_t openFileMutex = NULL;

static int fatInit(const UosMount*);
static int fatOpen(UosFile* file, const char *name, int flags, int mode);
static int fatClose(UosFile* file);
static int fatRead(UosFile* file, char* buf, int max);
static int fatWrite(UosFile* file, const char* buf, int max);

const UosFS uosFatFS = {

  .init  = fatInit,
  .open  = fatOpen,
  .close = fatClose,
  .read  = fatRead,
  .write = fatWrite
};

static FATFS fs;

static int fatInit(const UosMount* m)
{
  if (openFileMutex == NULL) {

    int i;

    for (i = 0; i < UOSCFG_FAT; i++)
      openFiles[i].inUse = false;

    openFileMutex = posMutexCreate();

    f_mount(&fs, m->dev, 1);
  }
  else
    P_ASSERT("fatInit", false);

  return 0;
}

int fatOpen(UosFile* file, const char *name, int flags, int mode)
{
  P_ASSERT("fatOpen", file->fs == &uosFatFS);
  int fd;

// Find free FAT descriptor.
  posMutexLock(openFileMutex);
  for (fd = 0; fd < UOSCFG_FAT; fd++)
    if (!openFiles[fd].inUse)
      break;

  if (fd >= UOSCFG_FAT) {

    posMutexUnlock(openFileMutex);
    errno = EMFILE;
    return -1;
  }

  openFiles[fd].inUse = true;
  posMutexUnlock(openFileMutex);

  FRESULT fr;

  file->u.fsobj = &openFiles[fd];
  fr = f_open(&openFiles[fd].file, name, FA_OPEN_EXISTING | FA_READ);
  if (fr == FR_OK)
    return 0;

  if (fr == FR_NO_FILE)
    errno = ENOENT;
  else if (fr == FR_NO_PATH)
    errno = ENOTDIR;
  else
    errno = EIO;
    
  posMutexLock(openFileMutex);
  openFiles[fd].inUse = false;
  posMutexUnlock(openFileMutex);
  return -1;
}

int fatClose(UosFile* file)
{
  P_ASSERT("fatOpen", file->fs == &uosFatFS);

  FatFile* f = (FatFile*)file->u.fsobj;
  if (f_close(&f->file) != 0) {

    errno = EIO;
    return -1;
  }

  posMutexLock(openFileMutex);
  f->inUse = false;
  posMutexUnlock(openFileMutex);
  return 0;
}

int fatRead(UosFile* file, char *buf, int len)
{
  P_ASSERT("fatRead", file->fs == &uosFatFS);

  FatFile* f = (FatFile*)file->u.fsobj;

  if (f->inUse) {

    FRESULT fr;
    UINT retLen;

    fr = f_read(&f->file, buf, len, &retLen);
    if (fr != FR_OK) {

      errno = EIO;
      return -1;
    }

    return retLen;
  }

  errno = EBADF;
  return -1;
}

int fatWrite(UosFile* file, const char *buf, int len)
{
  P_ASSERT("fatRead", file->fs == &uosFatFS);

//  FatFile* f = (FatFile*)file->u.fsobj;

  errno = EBADF;
  return -1;
}

#endif
