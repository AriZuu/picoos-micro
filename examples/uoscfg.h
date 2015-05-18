/*
 * Copyright (c) 2006-2013, Ari Suutari <ari@stonepile.fi>.
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
 * @file    uoscfg.h
 * @brief   picoos-micro library configuration file
 * @author  Ari Suutari <ari@stonepile.fi>
 */

/**
 * @ingroup config
 * @{
 */

/**
 * Compile FAT FS with readonly support for smaller code size.
 */
#define _FS_READONLY 1

/**
 * Select version of uosSpinUSecs to be included:
 * - 0: Don't include uosSpinUSecs,
 * - 1: Use hardware timer to implement uosSpinUSecs (works well only if system clock is fast enough),
 * - 2: Use delay loop (works well for msp430 for example).
 */
#define UOSCFG_SPIN_USECS 1

/**
 * Configure number of open files. If > 0 file system layer is enabled.
 */
#define UOSCFG_MAX_OPEN_FILES 10

/** 
 * Enable FAT filesystem and configure number of simultaneously open files.
 */
#define UOSCFG_FAT 10
/** @} */
