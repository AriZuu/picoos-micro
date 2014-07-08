/*
 * Copyright (c) 2012-2013, Ari Suutari <ari@stonepile.fi>.
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

#ifndef unix
extern void *__heap_start;
extern void *__heap_end;
extern unsigned int _end[];
extern unsigned int __stack[];
extern unsigned int __data_start[];
extern unsigned int __data_load_start[];
extern unsigned int _edata[];
extern unsigned int _etext[];
extern unsigned int __bss_start[];
extern unsigned int __bss_end[];
#endif

void uosBootDiag()
{
#if NOSCFG_FEATURE_CONOUT == 1 && NOSCFG_FEATURE_PRINTF == 1
  nosPrint("pico]OS " POS_VER_S "\n");
#ifndef unix
  nosPrintf("Ram:    data+bss %u, heap %u, irq stack %u\n", (int)((char*)_end - (char*)__data_start),
                                                             (int)((char*)__heap_end - (char*)__heap_start),
                                                             PORTCFG_IRQ_STACK_SIZE);
#endif
  nosPrintf("Limits: %d tasks, %d events\n", POSCFG_MAX_TASKS, POSCFG_MAX_EVENTS);
#endif
}

void uosResourceDiag()
{
#if NOSCFG_FEATURE_CONOUT == 1 && NOSCFG_FEATURE_PRINTF == 1

#ifdef POS_DEBUGHELP
  int taskCount = 0;
  int eventCount = 0;
  struct PICOTASK* task;
  struct PICOEVENT* event;
#endif

#if POSCFG_ARGCHECK > 1

  nosPrint("Stack unused amounts:\n");

  int freeStack;
  unsigned char* sp;

  freeStack = 0;

#ifndef unix
  sp = portIrqStack;
  while (*sp == PORT_STACK_MAGIC) {
    ++sp;
    ++freeStack;
  }

  nosPrintf("  IRQ %d\n", freeStack);
#endif

#endif

#ifdef POS_DEBUGHELP

  task = picodeb_tasklist;
  while (task != NULL) {

#if POSCFG_ARGCHECK > 1

    freeStack = 0;

    sp = task->handle->stack;
    while (*sp == PORT_STACK_MAGIC) {
      ++sp;
      ++freeStack;
    }

    nosPrintf("  task %s %d\n", task->name, freeStack);

#endif

    taskCount++;
    task = task->next;
  }

  event = picodeb_eventlist;
  while (event != NULL) {

    eventCount++;
    event = event->next;
  }

  nosPrintf("%d tasks, %d events in use\n", taskCount, eventCount);
  nosPrintf("%d tasks, %d events conf max\n", POSCFG_MAX_TASKS, POSCFG_MAX_EVENTS);

#else
#if POSCFG_ARGCHECK > 1 && POSCFG_FEATURE_GETTASK == 1
  
  POSTASK_t current = posTaskGetCurrent();

  freeStack = 0;
  sp = current->stack;
  while (*sp == PORT_STACK_MAGIC) {
    ++sp;
    ++freeStack;
  }

  nosPrintf("  current task %d\n", freeStack);

#endif

#endif
#endif
}

