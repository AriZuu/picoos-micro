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

//typedef const UosDisk* DiskRef;
typedef const UosFS* FSPtr;

UOS_BITTAB_TABLE(UosFile, UOSCFG_MAX_OPEN_FILES);
UOS_BITTAB_TABLE(FSPtr, UOSCFG_MAX_MOUNT);
static UosFileBittab fileTable;
static FSPtrBittab mountTable;

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

bool uosBitTabIsFree(uint8_t* bitmap, int b)
{
  int ibyte = b / 8;
  int ibit = b % 8;
  bool ret;

  posMutexLock(fsMutex);
  ret = (bitmap[ibyte] & (1 << ibit)) == 0;
  posMutexUnlock(fsMutex);

  return ret;
}

void uosFileInit()
{
  fsMutex = posMutexCreate();
  P_ASSERT("uosFileInit", fsMutex != NULL);
}

int uosMount(const UosFS* newMount)
{
  int slot =  UOS_BITTAB_ALLOC(mountTable);
  if (slot == -1) {

#if NOSCFG_FEATURE_PRINTF
    nosPrintf("uosMount: table full\n");
#endif
    return -1;
  }

  FSPtr* mount = UOS_BITTAB_ELEM(mountTable, slot);

  *mount = newMount;
  if (newMount->cf->init != NULL)
    newMount->cf->init(newMount);

  return 0;
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
    
    if (UOS_BITTAB_IS_FREE(mountTable, i))
      continue;

    m = *(UOS_BITTAB_ELEM(mountTable, i));
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
  }

  if (match == NULL)
    return NULL;

  *fsPath = path + strlen(match->mountPoint + extra);
  return match;
}

UosFile* uosFileAlloc(void)
{
  int slot =  UOS_BITTAB_ALLOC(fileTable);
  if (slot == -1) {

#if NOSCFG_FEATURE_PRINTF
    nosPrintf("uosFile: table full\n");
#endif
    return NULL;
  }

  return UOS_BITTAB_ELEM(fileTable, slot);
}

int uosFileFree(UosFile* file)
{
  P_ASSERT("uosFileClose", file != NULL);

  UOS_BITTAB_FREE(fileTable, UOS_BITTAB_SLOT(fileTable, file));
  return 0;
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

  P_ASSERT("uosFileOpen", fs->cf->open != NULL);

  UosFile* file = uosFileAlloc();
  if (file == NULL)
    return NULL;

  file->fs = fs;
  if (fs->cf->open(fs, file, fn, flags, mode) == -1) {

    uosFileFree(file);
    return NULL;
  }

  return file;
}

int uosFileClose(UosFile* file)
{
  P_ASSERT("uosFileClose", file != NULL);

  if (file->cf->close(file) == -1)
    return -1;

  uosFileFree(file);
  return 0;
}

int uosFileRead(UosFile* file, char* buf, int max)
{
  return file->cf->read(file, buf, max);
}

int uosFileWrite(UosFile* file, const char* buf, int len)
{
  if (file->cf->write == NULL) {

    errno = EPERM;
    return -1;
  }

  return file->cf->write(file, buf, len);
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

  memset(st, '\0', sizeof (UosFileInfo));

  // Check for mount point match
  if (!strcmp("", fn)) {

    st->isDir = true;
    st->size = 0;
    return 0;
  }

  P_ASSERT("uosFileStat", fs->cf->stat != NULL);
  return fs->cf->stat(fs, fn, st);
}

int uosFileFStat(UosFile* file, UosFileInfo* st)
{
  P_ASSERT("uosFileFStat", file->cf->fstat != NULL);

  memset(st, '\0', sizeof (UosFileInfo));
  return file->cf->fstat(file, st);
}

int uosFileSeek(UosFile* file, int offset, int whence)
{
  P_ASSERT("uosFileSeek", file->cf->lseek != NULL);
  return file->cf->lseek(file, offset, whence);
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
  if (!strcmp("", fn) || fs->cf->unlink == NULL) {

    errno = EPERM;
    return -1;
  }

  P_ASSERT("uosFileUnlink", fs->cf->unlink != NULL);
  return fs->cf->unlink(fs, fn);
}

int uosFileSync(UosFile* file)
{
  if (file->cf->sync)
    return file->cf->sync(file);

  return 0;
}

#endif

