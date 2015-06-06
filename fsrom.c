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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if UOSCFG_FS_ROM > 0

typedef struct {
  const UosRomFile* fe;
  int position;
} RomOpenFile;

extern UosRomFile uosRomFiles[];

UOS_BITTAB_TABLE(RomOpenFile, UOSCFG_FS_ROM);
static RomOpenFileBitmap openFiles;;

static int romOpen(UosFile* file, const char* fn, int flags, int mode);
static int romClose(UosFile* file);
static int romRead(UosFile* file, char *buf, int len);
static int romStat(const char* filename, UosFileInfo* st);

const UosFS uosRomFS = {
  .open  = romOpen,
  .close = romClose,
  .read  = romRead,
  .stat  = romStat
};

static int romOpen(UosFile* file, const char* fn, int flags, int mode)
{
  P_ASSERT("romOpen", file->mount->fs == &uosRomFS);
  const UosRomFile* fe = uosRomFiles;
  while (fe->fileName != NULL) {

    if (!strcmp(fn + 1, fe->fileName))
      break;

    fe = fe + 1;
  }

  if (fe->fileName == NULL) {
  
    errno = ENOENT;
    return -1;
  }

  int slot = UOS_BITTAB_ALLOC(openFiles);
  if (slot == -1) {

    errno = EMFILE;
    return -1;
  }

  RomOpenFile* rf = UOS_BITTAB_ELEM(openFiles, slot);

  file->u.fsobj = rf;
  rf->fe = fe;
  rf->position = 0;
  
  return 0;
}

static int romClose(UosFile* file)
{
  P_ASSERT("romClose", file->mount->fs == &uosRomFS);

  RomOpenFile* f = (RomOpenFile*)file->u.fsobj;
  UOS_BITTAB_FREE(openFiles, UOS_BITTAB_SLOT(openFiles, f));
  return 0;
}

static int romRead(UosFile* file, char *buf, int len)
{
  P_ASSERT("romRead", file->mount->fs == &uosRomFS);

  RomOpenFile* f = (RomOpenFile*)file->u.fsobj;

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

static int romStat(const char* fn, UosFileInfo* st)
{
  const UosRomFile* fe = uosRomFiles;
  int l;

  fn = fn + 1;
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

#endif
