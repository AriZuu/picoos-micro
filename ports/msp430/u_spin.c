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

#if UOSCFG_SPIN_USECS == 1
void uosSpinUSecs(uint16_t usec)
{
#if defined(__MSP430_HAS_T1A3__)

  // Timer runs from MCLK, so we need
  // MCLK MHZ * usec ticks.

  TA1R = 0;
  TA1CCR0 = PORTCFG_CPU_CLOCK_MHZ * usec;
  TA1CTL |= MC_1;

  while(!(TA1CCTL0 & CCIFG))
    ;

  TA1CTL &= ~(MC_1);
  TA1CCTL0 &= ~CCIFG;

#elif defined (__MSP430_HAS_TB3__)

  // Timer runs from MCLK, so we need
  // MCLK MHZ * usec ticks.

  TB0R = 0;
  TB0CCR0 = PORTCFG_CPU_CLOCK_MHZ * usec;
  TB0CTL |= MC_1;

  while(!(TB0CCTL0 & CCIFG))
    ;

  TB0CTL &= ~(MC_1);
   TB0CCTL0 &= ~CCIFG;

#else
#error no suitable timer for uosSpinUSecs
#endif
}
#endif
