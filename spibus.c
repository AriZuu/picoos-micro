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

#if UOSCFG_SPI_BUS > 0

/*
 * Default implementation for spi transmit.
 */
static void defaultXmit(
    const UosSpiBus* bus,
    const uint8_t *p,
    int cnt)
{
  while (cnt--)
    bus->cf->xchg(bus, *p++);
}

/*
 * Default implementation for spi receive.
 */
static void defaultRcvr(
    const UosSpiBus* bus,
    uint8_t *p,
    int cnt)
{
  while (cnt--)
    *p++ = bus->cf->xchg(bus, 0xff);
}

void uosSpiInit(struct uosSpiBus* bus)
{
  bus->busMutex = posMutexCreate();
  bus->currentAddr = UOS_SPI_BUS_NO_ADDRESS;
  bus->active = false;
  bus->cf->init(bus);
}

void uosSpiControl(struct uosSpiBus* bus, bool fullSpeed)
{
  P_ASSERT("uosSpiControl", bus->active);
  bus->cf->control(bus, fullSpeed);
}

void uosSpiBegin(struct uosSpiBus* bus, uint8_t addr)
{
  P_ASSERT("uosSpiBegin", !bus->active);

  posMutexLock(bus->busMutex);
  bus->currentAddr = addr;
  bus->cf->cs(bus, true);
  bus->active = true;
}

void uosSpiCS(struct uosSpiBus* bus, uint8_t addr, bool select)
{
  P_ASSERT("uosSpiControl", bus->active);

  bus->currentAddr = addr;
  bus->cf->cs(bus, select);
}

uint8_t uosSpiXchg(struct uosSpiBus* bus, uint8_t data)
{
  P_ASSERT("uosSpiXchg", bus->active);

  return bus->cf->xchg(bus, data);
}

void uosSpiXmit(const struct uosSpiBus* bus, const uint8_t* data, int len)
{
  P_ASSERT("uosSpiXmit", bus->active);

  if (bus->cf->xmit)
    bus->cf->xmit(bus, data, len);
  else
    defaultXmit(bus, data, len);
}

void uosSpiRcvr(const struct uosSpiBus* bus, uint8_t* data, int len)
{
  P_ASSERT("uosSpiRcvr", bus->active);

  if (bus->cf->rcvr)
    bus->cf->rcvr(bus, data, len);
  else
    defaultRcvr(bus, data, len);
}

void uosSpiEnd(struct uosSpiBus* bus)
{
  P_ASSERT("uosSpiEnd", bus->active);

  bus->cf->cs(bus, false);
  bus->currentAddr = UOS_SPI_BUS_NO_ADDRESS;
  bus->active = false;
  posMutexUnlock(bus->busMutex);
}

#endif
