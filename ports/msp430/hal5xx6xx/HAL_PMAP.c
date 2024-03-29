/*******************************************************************************
 *
 * HAL_PMAP.c
 * Port Mapper Library for PMAP controller of MSP430F5xx/6xx family
 *
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#include <picoos.h>
#include "HAL_PMAP.h"

// Check and define PMAP function only if the device has port mapping capability
// Note: This macro is defined in the device-specific header file if this
// feature is available on a given MSP430.
#ifdef __MSP430_HAS_PORT_MAPPING__

void configure_ports(const uint8_t *port_mapping, uint8_t *PxMAPy,
                     uint8_t num_of_ports, uint8_t port_map_reconfig)
{
    uint16_t i;

    // Store current interrupt state, then disable all interrupts
#if __GNUC__ == 4
    uint16_t globalInterruptState = __read_status_register() & GIE;
#else
    uint16_t globalInterruptState = __get_SR_register() & GIE;
#endif
    __dint();

    // Get write-access to port mapping registers:
    PMAPPWD = PMAPPW;

    if (port_map_reconfig){
        // Allow reconfiguration during runtime:
        PMAPCTL = PMAPRECFG;
    }

    // Configure Port Mapping:
    for (i = 0; i < num_of_ports * 8; i++){
        PxMAPy[i] = port_mapping[i];
    }

    // Disable write-access to port mapping registers:
    PMAPPWD = 0;

    // Restore previous interrupt state
#if __GNUC__ == 4
    __bis_status_register(globalInterruptState);
#else
    __bis_SR_register(globalInterruptState);
#endif
}

#endif  /* __MSP430_HAS_PORT_MAPPING__ */
