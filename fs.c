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

static const UosFS* findMount(const char* path, char const** fsPath);
static POSMUTEX_t fsMutex;

#define FILE_TABLE_OFFSET 3 // Account for stdin, stdout & stderr 

UOS_BITTAB_TABLE(UosFile, UOSCFG_MAX_OPEN_FILES);
static UosFileBittab fileTable;

typedef const UosFS* FSPtr;
static FSPtr mountTable[UOSCFG_MAX_MOUNT];

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
  FSPtr* mount = mountTable;

  for (i = 0; i < UOSCFG_MAX_MOUNT; i++, mount++)
    *mount = NULL;

  UOS_BITTAB_INIT(fileTable);
  uosDiskInit();
}

int uosMount(const UosFS* newMount)
{
  posMutexLock(fsMutex);

  int mi;
  FSPtr* mount = mountTable;

  for (mi = 0; mi < UOSCFG_MAX_MOUNT; mi++, mount++) {
    if (*mount == NULL) {
      
      *mount = newMount;
      if (newMount->i->init != NULL)
        newMount->i->init(newMount);

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

static const UosFS* findMount(const char* path, char const** fsPath)
{
  FSPtr* mount = mountTable;
  const UosFS* m;
  int i;
  const UosFS* match = NULL;

  // Assume that working directory is /
  if (!strncmp(path, "./", 2))
    path = path + 2;
  else
    if (path[0] == '/')
      path = path + 1;

  int pathLen = strlen(path);
  int mountLen;
  int extra = 0;
  int longestMatch = -1;

  for (i = 0; i < UOSCFG_MAX_MOUNT; i++) {
    
    m = *mount;
    if (m == NULL)
      break;

    if (!strcmp(m->mountPoint + 1, path)) {

      match = m;
      extra = 1;
      break;
    }

    mountLen = strlen(m->mountPoint + 1);
    if (pathLen > mountLen)
      if (mountLen == 0 || (!strncmp(m->mountPoint + 1, path, mountLen) && path[mountLen] == '/')) {
   
        if (mountLen > longestMatch) {

          longestMatch = mountLen;
          if (mountLen == 0)
            extra = 1;
          else
            extra = 0;

          match = m;
        }
      }

    mount++;
  }

  if (match == NULL)
    return NULL;

  *fsPath = path + strlen(match->mountPoint + extra);
  return match;
}

UosFile* uosFileOpen(const char* fileName, int flags, int mode)
{
  const char* fn;
  const UosFS* fs;

  fs = findMount(fileName, &fn);
  if (fs == NULL) {
 
    errno = ENOENT;
    return NULL;
  }

  int slot =  UOS_BITTAB_ALLOC(fileTable);
  if (slot == -1) {

    nosPrintf("uosFile: table full\n");
    return NULL;
  }

  UosFile* file = UOS_BITTAB_ELEM(fileTable, slot);

  file->fs = fs;
  if (fs->i->open(fs, file, fn, flags, mode) == -1) {

    UOS_BITTAB_FREE(fileTable, slot);
    return NULL;
  }

  return file;
}

int uosFileClose(UosFile* file)
{
  P_ASSERT("uosFileClose", file != NULL);

  if (file->i->close(file) == -1)
    return -1;

  UOS_BITTAB_FREE(fileTable, UOS_BITTAB_SLOT(fileTable, file));
  return 0;
}

int uosFileRead(UosFile* file, char* buf, int max)
{
  return file->i->read(file, buf, max);
}

int uosFileWrite(UosFile* file, const char* buf, int len)
{
  if (file->i->write == NULL) {

    errno = EPERM;
    return -1;
  }

  return file->i->write(file, buf, len);
}

int uosFileStat(const char* filename, UosFileInfo* st)
{
  const char* fn;
  const UosFS* fs;

  fs = findMount(filename, &fn);
  if (fs == NULL) {
 
    errno = ENOENT;
    return -1;
  }

  // Check for mount point match
  if (!strcmp("", fn)) {

    st->isDir = true;
    st->size = 0;
    return 0;
  }

  return fs->i->stat(fs, fn, st);
}

int uosFileFStat(UosFile* file, UosFileInfo* st)
{
  return file->i->fstat(file, st);
}

int uosFileSeek(UosFile* file, int offset, int whence)
{
  return file->i->lseek(file, offset, whence);
}

int uosFileUnlink(const char* filename)
{
  const char* fn;
  const UosFS* fs;

  fs = findMount(filename, &fn);
  if (fs == NULL) {
 
    errno = ENOENT;
    return -1;
  }

  // Check for mount point match
  if (!strcmp("", fn) || fs->i->unlink == NULL) {

    errno = EPERM;
    return -1;
  }

  return fs->i->unlink(fs, fn);
}

int uosFileSync(UosFile* file)
{
  if (file->i->sync)
    return file->i->sync(file);

  return 0;
}

#endif

