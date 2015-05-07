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

/*
 * Syscall implementations for newlib or newlib-nano.
 */

#if UOSCFG_NEWLIB_SYSCALLS == 1 

#include <stdlib.h>

#ifdef _NEWLIB_VERSION

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>

#undef errno
extern int errno;

/*
 * Prototypes for functions are not provided by
 * newlib (now odd).
 */
void* _sbrk(int bytes);
int _open(const char *name, int flags, int mode);
int _close(int fd);
int _lseek(int fd, int offset, int whence);
int _isatty (int fd);
int _read(int fd, char *buf, int len);
int _stat(char *file, struct stat *st);
int _fstat(int fd, struct stat *st);
int _write(int fd, char *buf, int len);
void _exit(int ret);
int _kill(int pid, int sig);
int _getpid(void);
int _unlink(char* name);
int _gettimeofday(struct timeval *ptimeval, void *ptimezone);

#if UOSCFG_FAT == 1

#include "ff.h"

/*
 * Open file table for FAT fs.
 */
#ifndef UOSCFG_FAT_OPENFILES
#define UOSCFG_FAT_OPENFILES 2
#endif

typedef struct {

  FIL	file;
  bool  inUse;
} UOS_FILE;

static UOS_FILE	openFiles[UOSCFG_FAT_OPENFILES];
static POSMUTEX_t openFileMutex;

#define FD2FILE(fd) (openFiles + (fd - 3))

static int allocFd()
{
  int fd;
  
  posMutexLock(openFileMutex);
  for (fd = 0; fd < UOSCFG_FAT_OPENFILES; fd++)
    if (!openFiles[fd].inUse)
      break;

  if (fd >= UOSCFG_FAT_OPENFILES) {

    posMutexUnlock(openFileMutex);
    errno = EMFILE;
    return -1;
  }

  openFiles[fd].inUse = true;
  posMutexUnlock(openFileMutex);
  return fd + 3;
}

static void freeFd(int fd)
{
  posMutexLock(openFileMutex);
  FD2FILE(fd)->inUse = false;
  posMutexUnlock(openFileMutex);
}

#endif

void uosNewlibInit()
{
#if UOSCFG_FAT == 1

  int i;

  for (i = 0; i < UOSCFG_FAT_OPENFILES; i++)
    openFiles[i].inUse = false;

  openFileMutex = posMutexCreate();

#endif
}

#if NOSCFG_MEM_MANAGER_TYPE == 0

static void* breakNow = NULL;

/*
 * Use __heap_start & __heap_end as Pico]OS nano
 * layer allocator.
 */
void* _sbrk(int bytes)
{
  if (breakNow == NULL)
    breakNow = __heap_start;

  void* oldBreak = breakNow;

  if ((char*)breakNow + bytes >= (char*)__heap_end) {

    errno = ENOMEM;
    return (void*)-1;
  }

  breakNow = (void*)((char*)breakNow + bytes);
  return oldBreak;
}

#endif

int _open(const char *name, int flags, int mode)
{
#if UOSCFG_FAT == 1
// Find free descriptor.

  int fd = allocFd();
  if (fd == -1)
    return fd;
  
  FRESULT fr;

  fr = f_open(&FD2FILE(fd)->file, name, FA_OPEN_EXISTING | FA_READ);
  if (fr == FR_OK)
    return fd;

  if (fr == FR_NO_FILE)
    errno = ENOENT;
  else if (fr == FR_NO_PATH)
    errno = ENOTDIR;
  else
    errno = EIO;
    
  freeFd(fd);

#else
  errno = ENOENT;
#endif

  return -1;
}

int _close(int fd)
{
  if (fd <= 2)
    return 0;

#if UOSCFG_FAT == 1

  UOS_FILE* file = FD2FILE(fd);

  if (file != NULL && file->inUse) {

    if (f_close(&file->file) != 0) {

      errno = EIO;
      return -1;
    }

    freeFd(fd);
    return 0;
  }

#endif

  errno = EBADF;
  return -1;
}

/*
 * Read characters from file or console.
 */
int _read(int fd, char *buf, int len)
{
#if NOSCFG_FEATURE_CONIN == 1

  if (fd == STDIN_FILENO) {

    int i = 0;

    while (i < len) {

      *buf = nosKeyGet();
      nosPrintChar(*buf);
      if (*buf == '\r') {

        nosPrintChar('\n');
        *buf = '\n';
        return i + 1;
      }

      if (*buf == '\n') {

        nosPrintChar('\r');
        return i + 1;
      }

      if (*buf == 127 && i > 0) {

        --i;
        --buf;
        continue;
      }

      ++i;
      ++buf;
    }

    return i;

  }

#endif

  if (fd <= 2) {

    errno = EIO;
    return -1;

  }

#if UOSCFG_FAT == 1

  UOS_FILE* file = FD2FILE(fd);

  if (file->inUse) {

    FRESULT fr;
    UINT retLen;

    fr = f_read(&file->file, buf, len, &retLen);
    if (fr != FR_OK) {

      errno = EIO;
      return -1;
    }

    return retLen;
  }

#endif

  errno = EBADF;
  return -1;
}

/*
 * Write to file or console.
 */
int _write(int fd, char *buf, int len)
{
  int i;

#if NOSCFG_FEATURE_CONOUT == 1

  if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {

    for (i = 0; i < len; i++, buf++) {

      if (*buf == '\n')
        nosPrintChar('\r');

      nosPrintChar(*buf);
    }

    return len;
  }
#endif

  errno = EBADF;
  return -1;
}

int _lseek(int fd, int offset, int whence)
{
  if (fd <= 2)
    return 0;

  errno = EBADF;
  return (long) -1;
}

int _isatty (int fd)
{
  if (fd <= 2)
    return 1;

#if UOSCFG_FAT == 1
  if (FD2FILE(fd)->inUse)
    return 0;
#endif

  errno = EBADF;
  return  -1;
}

int _stat(char *file, struct stat *st)
{
  st->st_mode = S_IFCHR;
  return 0;
}

int _fstat(int fd, struct stat *st)
{
  st->st_mode = S_IFCHR;
  return 0;
}

int fsync(int fd)
{
  errno = EIO;
  return -1;
}

void _exit(int ret)
{
  while(1);
}

int _kill(int pid, int sig)
{
  errno = EINVAL;
  return -1;
}

int _getpid()
{
  return 1; // Only one "process".
}

int _unlink(char* name)
{
  errno = ENOENT;
  return -1;
}

int _gettimeofday(struct timeval *ptimeval, void *ptimezone)
{
  errno = ENOSYS;
  return -1;
}

#endif
#endif
