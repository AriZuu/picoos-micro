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

/**
 * @file    picoos-u.h
 * @brief   Include file of u-layer library for pico]OS
 * @author  Ari Suutari
 */

/**
 * @mainpage picoos-micro - u-layer for pico]OS
 * @section overview Overview
 * This library contains miscellaneous routines built on pico]OS pico & nano layers.
 *
 * <b> Table Of Contents </b>
 * - @ref api
 * - @ref config
 */

/** @defgroup api   u-layer API */
/** @defgroup config   Configuration */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "uoscfg.h"
#include <stdint.h>

/**
 * @ingroup api
 * @{
 */

/**
 * Initialize u-layer. Must be called before any other 
 * API function.
 */
void uosInit(void);
void uosBootDiag(void);
void uosResourceDiag(void);

#if defined(__MSP430__) && UOSCFG_SPIN_USECS == 2

#define uosSpinUSecs(t) __delay_cycles(PORTCFG_CPU_CLOCK_MHZ * (t))

#else

void uosSpinUSecs(uint16_t);

#endif
/** @} */

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */
