/*
 * Copyright (c) 2016, Ari Suutari <ari@stonepile.fi>.
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
#include <string.h>

#if UOSCFG_MAX_OPEN_FILES > 0

#include <unistd.h>
#include <fcntl.h>

#endif

static POSMUTEX_t configMutex;
static UosConfigKeyValue* config;

const char* uosConfigGet(const char* key)
{
  UosConfigKeyValue* ptr;

  posMutexLock(configMutex);
  ptr = config;
  while (ptr) {

    if (ptr->key[0])
      if (!strcmp(key, ptr->key)) {

        posMutexUnlock(configMutex);
        return ptr->value;
      }

    ptr = ptr->next;
  }

  posMutexUnlock(configMutex);
  return NULL;
}

const char* uosConfigSet(const char* key, const char* value)
{
  UosConfigKeyValue* ptr;

  posMutexLock(configMutex);

// Search for key.

  ptr = config;
  while (ptr) {

    if (ptr->key[0] && !strcmp(key, ptr->key))
      break;

    ptr = ptr->next;
  }

// If key was not found, search for empty slot.

  if (ptr == NULL) {

    ptr = config;
    while (ptr) {

      if (!ptr->key[0])
        break;
  
      ptr = ptr->next;
    }
  }

// Config full, allocate new entry.

  if (ptr == NULL) {

    ptr = nosMemAlloc(sizeof(UosConfigKeyValue));
    P_ASSERT("configAlloc", ptr != NULL);

    memset(ptr, '\0', sizeof(UosConfigKeyValue));
    ptr->next = config;
    config = ptr;
  }

  if (ptr->key[0] == '\0')
    strlcpy(ptr->key, key, sizeof(ptr->key));

  strlcpy(ptr->value, value, sizeof(ptr->value));
  posMutexUnlock(configMutex);
  return ptr->value;
}

#if UOSCFG_CONFIG_PREALLOC > 0
static UosConfigKeyValue kvStatic[UOSCFG_CONFIG_PREALLOC];
#endif

void uosConfigInit(void)
{
  int i;
  UosConfigKeyValue* ptr;

  configMutex = posMutexCreate();
  
#if UOSCFG_CONFIG_PREALLOC > 0

  memset(kvStatic, '\0', sizeof(kvStatic));

  config = kvStatic;
  ptr = config;
  for (i = 0; i < UOSCFG_CONFIG_PREALLOC - 1; i++) {

    ptr->next = ptr + 1;
    ++ptr;
  }

#else

  config = NULL;

#endif
}

int uosConfigSaveEntries(void* context, UosConfigSaver saver)
{
  UosConfigKeyValue* ptr = config;

  while (ptr) {

    if (ptr->key[0])
      if (saver(context, ptr->key, ptr->value) == -1)
        return -1;

    ptr = ptr->next;
  }

  return 0;
}

#if UOSCFG_MAX_OPEN_FILES > 0

static int fileSaver(void* context, const char* key, const char* value)
{
  char buf[UOS_CONFIG_KEYSIZE + UOS_CONFIG_VALUESIZE + 3];
  char* ptr;

  ptr = stpcpy(buf, key);
  ptr = stpcpy(ptr, "=");
  ptr = stpcpy(ptr, value);
  ptr = stpcpy(ptr, "\n");

  return uosFileWrite((UosFile*)context, buf, strlen(buf));
}

int uosConfigSave(const char* filename)
{
  UosFile* file = uosFileOpen(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  int ret = 0;

  if (file == NULL)
    return -1;

  if (uosConfigSaveEntries(file, fileSaver) == -1)
    ret = -1;

  uosFileClose(file);
  return ret;
}

int uosConfigLoad(const char* filename)
{
  UosFile* file = uosFileOpen(filename, O_RDONLY, 0);

  char buf[50];
  int  bytesInBuf = 0;
  int len;
  char* ptr;
  char* eq;

  do {

    // Fill buffer from file. At least one key-value pair should
    // fit into it.
    len = uosFileRead(file, buf + bytesInBuf, sizeof(buf) - bytesInBuf - 1);
    if (len) {

      bytesInBuf += len;
      buf[bytesInBuf] = '\0';

      // Find delimeter. If none is found, buffer was too small.
      ptr = strchr(buf, '\n');
      if (ptr == NULL) {

        uosFileClose(file);
        return -1;
      }

      do {
  
        *ptr = '\0';
        eq = strchr(buf, '=');
        if (eq) {

          *eq = '\0';
          uosConfigSet(buf, eq + 1);
        }

        // Move buffer towards beginning overwriting used bytes.
        bytesInBuf -= (1 + ptr - buf);
        memmove(buf, ptr + 1, bytesInBuf + 1);

      } while((ptr = strchr(buf, '\n')));
    }
  } while (len != 0);

  uosFileClose(file);
  return 0;
}

#endif
