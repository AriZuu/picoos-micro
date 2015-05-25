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
#include <string.h>
#include <errno.h>

#if UOSCFG_MAX_OPEN_FILES > 0

#ifndef UOSCFG_MAX_MOUNT
#define UOSCFG_MAX_MOUNT 2
#endif

static POSMUTEX_t fsMutex;

#define FILE_TABLE_OFFSET 3 // Account for stdin, stdout & stderr 

typedef const UosMount* UosMountPtr;
UOS_BITTAB_TABLE(UosFile, UOSCFG_MAX_OPEN_FILES);
static UosFileBitmap fileTable;

static UosMountPtr mountTable[UOSCFG_MAX_MOUNT];

int uosBitTabAlloc(uint8_t* bitmap, int size)
{
  int i;

  posMutexLock(fsMutex);
  i = 0;
  while (i < size) {

    int ibyte = i / 8;
    int ibit = 0;
    uint8_t m = 1;
    uint8_t b = bitmap[ibyte];

    while (i + ibit < size && ibit < 8) {

      if ((b & m) == 0) {
   
        bitmap[ibyte] |= m;
        posMutexUnlock(fsMutex);
        return i + ibit;
      }

      m = m << 1;
      ibit++;
    }

    i += 8;
  }

  posMutexUnlock(fsMutex);
  return -1;
}

void uosBitTabFree(uint8_t* bitmap, int b)
{
  int ibyte = b / 8;
  int ibit = b % 8;

  posMutexLock(fsMutex);
  bitmap[ibyte] &= ~(1 << ibit);
  posMutexUnlock(fsMutex);
}

void uosFileInit()
{
  fsMutex = posMutexCreate();
  P_ASSERT("uosFileInit", fsMutex != NULL);

  int i;
  UosMountPtr* mount = mountTable;

  for (i = 0; i < UOSCFG_MAX_MOUNT; i++, mount++)
    *mount = NULL;

  UOS_BITTAB_INIT(fileTable);
}

int uosMount(const UosMount* m)
{
  posMutexLock(fsMutex);

  int i;
  UosMountPtr* mount = mountTable;

  for (i = 0; i < UOSCFG_MAX_MOUNT; i++, mount++) {
    if (*mount == NULL) {
      
      *mount = m;
      if (m->fs->init != NULL)
        m->fs->init(m);

      posMutexUnlock(fsMutex);
      return 0;
    }
  }

  posMutexUnlock(fsMutex);
  return -1;
}

int uosFileSlot(UosFile* file)
{
  if (file == NULL)
    return -1;

  return UOS_BITTAB_SLOT(fileTable, file) + FILE_TABLE_OFFSET;
}

UosFile* uosFile(int fd)
{
  return UOS_BITTAB_ELEM(fileTable, (fd - FILE_TABLE_OFFSET));
}

UosFile* uosFileOpen(const char* fileName, int flags, int mode)
{
  UosMountPtr* mount = mountTable;
  int i;
  const UosMount* m = NULL;

  if (fileName[0] == '.')
    fileName = fileName + 1; // working directory is always /

  for (i = 0; i < UOSCFG_MAX_MOUNT; i++) {
    
    m = *mount;
    if (m == NULL) {
 
      errno = ENOENT;
      return NULL;
    }

    if (!strncmp(m->mountPoint, fileName, strlen(m->mountPoint)))
      break;

    mount++;
  }

  if (i >= UOSCFG_MAX_MOUNT) {

    errno = ENOENT;
    return NULL;
  }

  int slot =  UOS_BITTAB_ALLOC(fileTable);
  if (slot == -1)
    return NULL;

  UosFile* file = UOS_BITTAB_ELEM(fileTable, slot);

  file->mount = m;

  char fn[80];
  strcpy(fn, m->dev);
  strcat(fn, fileName + strlen(m->mountPoint));
  if (file->mount->fs->open(file, fn, flags, mode) == -1) {

    UOS_BITTAB_FREE(fileTable, slot);
    return NULL;
  }

  return file;
}

int uosFileClose(UosFile* file)
{
  P_ASSERT("uosFileClose", file != NULL);

  if (file->mount->fs->close(file) == -1)
    return -1;

  UOS_BITTAB_FREE(fileTable, UOS_BITTAB_SLOT(fileTable, file));
  return 0;
}

int uosFileRead(UosFile* file, char* buf, int max)
{
  return file->mount->fs->read(file, buf, max);
}

int uosFileWrite(UosFile* file, const char* buf, int len)
{
  return file->mount->fs->write(file, buf, len);
}

#endif
