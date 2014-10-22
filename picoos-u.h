/*
 * Copyright (c) 2012-2014, Ari Suutari <ari@stonepile.fi>.
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
 * @author  Ari Suutari <ari@stonepile.fi>
 */

/**
 * @mainpage picoos-micro - &mu;-layer for pico]OS
 * <b> Table Of Contents </b>
 * - @ref api
 * - @ref config
 * @section overview Overview
 * This library contains miscellaneous routines built on pico]OS pico & nano layers.
 *
 * @subsection features Features
 * <b>Microsecond delay:</b>
 *
 * Implementation of microsecond delay using a spin-loop. Depending on CPU it uses either 
 * simple delay loop or hardware timer.
 *
 * <b>FAT filesystem:</b>
 *
 * Implementation of FAT filesystem from <a href="http://elm-chan.org/fsw/ff/00index_e.html">elm-chan.</a>
 * Currently only readonly mode is used and application must provide
 * functions like disk_initialize, disk_read and disk_status that handle
 * access to real hardware (like SD-card for example).
 */

/** @defgroup api   &mu;-layer API */
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
 * Initialize &mu;-layer. Must be called before any other 
 * API function.
 */
void uosInit(void);

/**
 * Print memory sizes and required copyright messages when
 * system starts.
 */
void uosBootDiag(void);

/**
 * Print information about resource usage. Currently output
 * includes free stack space for each task, free interrupt
 * stack space and number of tasks and events in use.
 */
void uosResourceDiag(void);

#if defined(__MSP430__) && UOSCFG_SPIN_USECS == 2

#define uosSpinUSecs(t) __delay_cycles(PORTCFG_CPU_CLOCK_MHZ * (t))

#else

/** 
 * Spin in loop until requested number of microseconds have passed.
 * Uses either hardware timer or delay loop depending of
 * ::UOSCFG_SPIN_USECS setting.
 */
void uosSpinUSecs(uint16_t uSecs);

/**
 * Initialize possible hardware timer for uosSpinUSecs. Called
 * internally by uosInit().
 */
void uosSpinInit(void);

#endif
/** @} */

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */
