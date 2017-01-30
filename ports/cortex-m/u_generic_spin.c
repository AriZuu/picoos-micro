/*
 * Copyright (c) 2014-2015, Ari Suutari <ari@stonepile.fi>.
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

#if UOSCFG_SPIN_USECS == 2

void uosSpinInit(void)
{
#if __CORTEX_M >= 3

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; /* Global Enable for DWT */
    DWT->CYCCNT = 0;                                /* Reset the counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;            /* Enable cycle counter */

#endif
}

void uosSpinUSecs(uint16_t us)
{
#if __CORTEX_M >= 3

  uint32_t start = DWT->CYCCNT;
  uint32_t cycles = us * (SystemCoreClock / 1000000);

  if (cycles == 0)
    return;

  while (DWT->CYCCNT - start < cycles);

#else

  uint32_t cycles = us * (SystemCoreClock / 1000000) / 3;

  if (cycles == 0)
    return;

  asm volatile("\n"
               "delayloop:\n"
               " sub %[count], %[count], #1\n"
               " bcs delayloop" :: [count]"r"(cycles));
#endif
}
#endif
