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
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 * Syscall implementations for newlib or newlib-nano.
 */

#if UOSCFG_NEWLIB_SYSCALLS == 1

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
  errno = ENOENT;
  return -1;
}

int _close(int fd)
{
  errno = EBADF;
  return -1;
}

int _lseek(int fd, int offset, int whence)
{
  if (fd == STDOUT_FILENO || fd == STDERR_FILENO)
    return 0;

  errno = EBADF;
  return (long) -1;
}

int _isatty (int fd)
{
  if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
    return  1;
  }

  errno = EBADF;
  return  -1;
}

/*
 * Read characters from console, if it is configured.
 */
int _read(int fd, char *buf, int len)
{
#if NOSCFG_FEATURE_CONIN == 1

  if (fd == STDIN_FILENO) {

    int i;

    for (i = 0; i < len; i++, buf++) {

      *buf = nosKeyGet();
      nosPrintChar(*buf);
      if (*buf == '\n')
        return i;
    }

    return i;
  }
#endif

  errno = EBADF;
  return -1;
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

/*
 * Write to console, if configured.
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

#endif

