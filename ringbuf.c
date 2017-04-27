/*
 * Copyright (c) 2015, Ari Suutari <ari@stonepile.fi>.
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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

/*
 * This is loosely based on mailbox code that can be found in
 * Unix port of lwip contrib package.
 */

#include <picoos.h>
#include <picoos-u.h>

#include <string.h>

#if UOSCFG_RING > 0

struct uosRing {

  int         msgCount;
  int         msgSize;
  int         tail;
  int         head;
  uint8_t     *msgs;
  POSSEMA_t   notEmpty;
  POSSEMA_t   notFull;
  int         waitSend;
};

#define IS_FULL(ring) (((ring->head + 1) % ring->msgCount) == ring->tail)
#define IS_EMPTY(ring) (ring->head == ring->tail)
#define MSG_AT(ring, i) (ring->msgs + ring->msgSize * i)

UosRing* uosRingCreate(int msgSize, int msgCount)
{
  P_ASSERT("uosRingCreate: msgSize valid", msgSize > 0);
  P_ASSERT("uosRingCreate: msgCount valid", msgCount > 0);
  UosRing* ring = nosMemAlloc(sizeof(struct uosRing));

  if (ring == NULL)
    return NULL;

  ring->msgSize   = msgSize;
  ring->msgCount  = msgCount;
  ring->msgs      = nosMemAlloc(ring->msgSize * ring->msgCount);
  ring->tail      = 0;
  ring->head      = 0;
  ring->notEmpty  = nosSemaCreate(0, 0, "ringe*");
  ring->notFull   = nosSemaCreate(0, 0, "ringf*");
  ring->waitSend  = 0;
  
  return ring;
}

bool uosRingPut(UosRing* ring, const void *msg, UINT_t timeout)
{
  P_ASSERT("uosRingPut: ringbuffer valid", ring != NULL);

  bool first;
  bool fail;
  POS_LOCKFLAGS;

  POS_SCHED_LOCK;

  while (IS_FULL(ring)) {

    if (timeout == 0) {
  
      POS_SCHED_UNLOCK;
      return false;
    }

    ring->waitSend++;
 
    POS_SCHED_UNLOCK;
    fail = nosSemaWait(ring->notFull, timeout);
    POS_SCHED_LOCK;

    ring->waitSend--;

    if (fail) {

      POS_SCHED_UNLOCK;
      return false;
    }
  }

  memcpy(MSG_AT(ring, ring->head), msg, ring->msgSize);

  if (IS_EMPTY(ring))
    first = true;
  else
    first = false;

  ring->head = (ring->head + 1) % ring->msgCount;

  POS_SCHED_UNLOCK;

  if (first)
    nosSemaSignal(ring->notEmpty);

  return true;
}

bool uosRingGet(UosRing* ring, void *msg, UINT_t timeout)
{
  P_ASSERT("uosRingGet: ringbuffer valid", ring != NULL);

  bool waitSend;
  POS_LOCKFLAGS;

  POS_SCHED_LOCK;

  while (IS_EMPTY(ring)) {

    POS_SCHED_UNLOCK;

    if (nosSemaWait(ring->notEmpty, timeout))
      return false;
    
    POS_SCHED_LOCK;
  }

  memcpy(msg, MSG_AT(ring, ring->tail), ring->msgSize);
  ring->tail = (ring->tail + 1) % ring->msgCount;

  waitSend = ring->waitSend;
  POS_SCHED_UNLOCK;

  if (waitSend)
    nosSemaSignal(ring->notFull);

  return true;
}

void uosRingDestroy(UosRing* ring)
{
  P_ASSERT("uosRingDestroy: ringbuffer valid", ring != NULL);

  nosSemaDestroy(ring->notEmpty);
  nosSemaDestroy(ring->notFull);

  nosMemFree(ring->msgs);
  nosMemFree(ring);
}

#endif
