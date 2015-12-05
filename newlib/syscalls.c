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
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>

void __wrap___sfp_lock_acquire(void);
void __wrap___sfp_lock_release(void);
FILE* __wrap_fopen(const char* path, const char* mode);
FILE* __real_fopen(const char* path, const char* mode);

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

void uosNewlibInit()
{
// Disable output buffering buffering. We don't really need
// it.

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
}

#if NOSCFG_MEM_MANAGER_TYPE != 1

static void* breakNow = NULL;

/*
 * Use __heap_start & __heap_end as Pico]OS nano
 * layer allocator.
 */
void* _sbrk(int bytes)
{
  posTaskSchedLock();

  if (breakNow == NULL)
    breakNow = __heap_start;

  void* oldBreak = breakNow;

  if ((char*)breakNow + bytes >= (char*)__heap_end) {

    errno = ENOMEM;
    posTaskSchedUnlock();
    return (void*)-1;
  }

  breakNow = (void*)((char*)breakNow + bytes);
  posTaskSchedUnlock();
  return oldBreak;
}

#endif

/*
 * Make fopen/fclose thread safe by wrapping
 * default locking functions with Pico]OS scheduler lock.
 * This is far from perfect, but... it's a start.
 */
void __wrap___sfp_lock_acquire()
{
  posTaskSchedLock();
}

void __wrap___sfp_lock_release()
{
  posTaskSchedUnlock();
}

FILE* __wrap_fopen(const char* path, const char* mode)
{
  FILE* ret;

  posTaskSchedLock();
  ret = __real_fopen(path, mode);
  posTaskSchedUnlock();
  return ret;
}

int _open(const char *name, int flags, int mode)
{
#if UOSCFG_MAX_OPEN_FILES > 0

  UosFile* file = uosFileOpen(name, flags, mode);
  if (file != NULL)
    return uosFileSlot(file);

#else
  errno = ENOENT;
#endif

  return -1;
}

int _close(int fd)
{
  if (fd <= 2)
    return 0;

#if UOSCFG_MAX_OPEN_FILES > 0

  UosFile* file = uosFile(fd);

  if (file != NULL)
    return uosFileClose(file);

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
      if (*buf == 127) {

        if (i > 0) {

          nosPrintChar(8);
          nosPrintChar(' ');
          nosPrintChar(8);
          --i;
          --buf;
        }

        continue;
      }

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

#if UOSCFG_MAX_OPEN_FILES > 0

  UosFile* file = uosFile(fd);

  if (file != NULL)
    return uosFileRead(file, buf, len);

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

  if (fd <= 2) {

    errno = EIO;
    return -1;

  }

#if UOSCFG_MAX_OPEN_FILES > 0

  UosFile* file = uosFile(fd);

  if (file != NULL)
    return uosFileWrite(file, buf, len);

#endif

  errno = EBADF;
  return -1;
}

int _lseek(int fd, int offset, int whence)
{
  if (fd <= 2)
    return 0;

#if UOSCFG_MAX_OPEN_FILES > 0

  UosFile* file = uosFile(fd);

  if (file != NULL)
    return uosFileSeek(file, offset, whence);

#endif

  errno = EBADF;
  return (long) -1;
}

int _isatty (int fd)
{
  if (fd <= 2)
    return 1;

#if UOSCFG_MAX_OPEN_FILES > 0
  if (uosFile(fd) != NULL)
    return 0;
#endif

  errno = EBADF;
  return  -1;
}

int _stat(char *file, struct stat *st)
{
#if UOSCFG_MAX_OPEN_FILES > 0

  UosFileInfo fi;

  if (uosFileStat(file, &fi) == -1)
    return -1;

  memset(st, '\0', sizeof(struct stat));
  if (fi.isSocket)
    st->st_mode = S_IFSOCK;
  else
    st->st_mode = fi.isDir ? S_IFDIR : S_IFREG;

  st->st_size = fi.size;
  return 0;
#else

  errno = ENOENT;
  return -1;

#endif
}

int _fstat(int fd, struct stat *st)
{
  if (fd <= 2) {

    memset(st, '\0', sizeof(struct stat));
    st->st_mode = S_IFCHR;
    return 0;
  }

#if UOSCFG_MAX_OPEN_FILES > 0

  UosFile* file = uosFile(fd);

  if (file != NULL) {

    UosFileInfo fi;

    if (uosFileFStat(file, &fi) == -1)
      return -1;

    memset(st, '\0', sizeof(struct stat));
    st->st_mode = fi.isDir ? S_IFDIR : S_IFREG;
    st->st_size = fi.size;
    return 0;
  }

#endif

  errno = EBADF;
  return -1;
}

int fsync(int fd)
{
#if UOSCFG_MAX_OPEN_FILES > 0

  UosFile* file = uosFile(fd);

  if (file != NULL)
    return uosFileSync(file);

#endif

  errno = EBADF;
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
#if UOSCFG_MAX_OPEN_FILES > 0

  return uosFileUnlink(name);

#else

  errno = ENOENT;
  return -1;

#endif
}

int _gettimeofday(struct timeval *ptimeval, void *ptimezone)
{
  errno = ENOSYS;
  return -1;
}

#endif
#endif
