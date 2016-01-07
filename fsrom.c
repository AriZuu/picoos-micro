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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if UOSCFG_FS_ROM > 0

#include <fcntl.h>
#include <unistd.h>

typedef struct {
  const UosRomFile* fe;
  int position;
} RomOpenFile;

typedef struct {
  UosFS base;
  const UosRomFile* data;
} RomFS;

UOS_BITTAB_TABLE(RomFS, UOSCFG_MAX_MOUNT);
UOS_BITTAB_TABLE(RomOpenFile, UOSCFG_FS_ROM);
static RomFSBittab mountedRoms;
static RomOpenFileBittab openFiles;;

static int romOpen(const UosFS* mount, UosFile* file, const char* fn, int flags, int mode);
static int romClose(UosFile* file);
static int romRead(UosFile* file, char *buf, int len);
static int romStat(const UosFS* fs, const char* filename, UosFileInfo* st);
static int romFStat(UosFile* file, UosFileInfo* st);
static int romSeek(UosFile* file, int offset, int whence);

const UosFSConf uosRomFSConf = {
  .open   = romOpen,
  .stat   = romStat,
};

const UosFileConf uosRomFileConf = {
  .close  = romClose,
  .read   = romRead,
  .fstat  = romFStat,
  .lseek  = romSeek,
};

int uosMountRom(const char* mountPoint, const UosRomFile* data)
{
  int slot = UOS_BITTAB_ALLOC(mountedRoms);
  if (slot == -1) {

    nosPrintf("romFs: mount table full\n");
    errno = ENOSPC;
    return -1;
  }

  RomFS* m = UOS_BITTAB_ELEM(mountedRoms, slot);

  m->base.mountPoint = mountPoint;
  m->base.cf = &uosRomFSConf;
  m->data = data;
  return uosMount(&m->base);
}

static int romOpen(const UosFS* mount, UosFile* file, const char* fn, int flags, int mode)
{
  P_ASSERT("romOpen", file->fs->cf == &uosRomFSConf);

  if (flags & O_ACCMODE) {

    errno = EPERM;
    return -1;
  }

  RomFS* rfs = (RomFS*)file->fs;
  const UosRomFile* fe = rfs->data;
  while (fe->fileName != NULL) {

    if (!strcmp(fn, fe->fileName))
      break;

    fe = fe + 1;
  }

  if (fe->fileName == NULL) {
  
    errno = ENOENT;
    return -1;
  }

  int slot = UOS_BITTAB_ALLOC(openFiles);
  if (slot == -1) {

    nosPrintf("romFs: table full\n");
    errno = EMFILE;
    return -1;
  }

  RomOpenFile* rf = UOS_BITTAB_ELEM(openFiles, slot);

  file->fsPriv = rf;
  file->cf = &uosRomFileConf;
  rf->fe = fe;
  rf->position = 0;
  
  return 0;
}

static int romClose(UosFile* file)
{
  P_ASSERT("romClose", file->fs->cf == &uosRomFSConf);

  RomOpenFile* f = (RomOpenFile*)file->fsPriv;
  UOS_BITTAB_FREE(openFiles, UOS_BITTAB_SLOT(openFiles, f));
  return 0;
}

static int romRead(UosFile* file, char *buf, int len)
{
  P_ASSERT("romRead", file->fs->cf == &uosRomFSConf);

  RomOpenFile* f = (RomOpenFile*)file->fsPriv;

// Check EOF.
  if (f->position >= f->fe->size || len == 0)
    return 0;

  int left = f->fe->size - f->position;
  if (len > left)
    len = left;

  memcpy(buf, f->fe->contents + f->position, len);
  f->position += len;
  return len;
}

static int romStat(const UosFS* fs, const char* fn, UosFileInfo* st)
{
  RomFS* rfs = (RomFS*)fs;
  const UosRomFile* fe = rfs->data;
  int l;

  while (fe->fileName != NULL) {

    if (!strcmp(fn, fe->fileName)) {

      st->isDir = false; 
      st->size = fe->size;
      return 0;
    }

    l = strlen(fn);
    if (!strncmp(fn, fe->fileName, l) && fe->fileName[l] == '/') {

      st->isDir = true; 
      st->size = 0;
      return 0;
    }

    fe = fe + 1;
  }

  errno = ENOENT;
  return -1;
}

static int romFStat(UosFile* file, UosFileInfo* st)
{
  RomOpenFile* f = (RomOpenFile*)file->fsPriv;

  st->isDir = false;
  st->size = f->fe->size;
  return 0;
}

static int romSeek(UosFile* file, int offset, int whence)
{
  RomOpenFile* f = (RomOpenFile*)file->fsPriv;
  long pos;

  switch (whence) {
  case SEEK_SET:
    pos = offset;
    break;

  case SEEK_CUR:
    pos = f->position + offset;
    break;

  case SEEK_END:
    pos = f->fe->size + offset;
    break;

  default:
    errno = EINVAL;
    return -1;
    break;
  }

  if (pos < 0 || pos >= f->fe->size) {

    errno = EINVAL;
    return -1;
  }

  f->position = pos;
  return 0;
}

#endif
