/*******************************************************************************
 *
 * HAL_UCS.c
 * Provides Functions to Initialize the UCS/FLL and clock sources
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

#if defined(__MSP430_HAS_UCS_RF__) || defined(__MSP430_HAS_UCS__)

#include "HAL_UCS.h"

/*******************************************************************************
 * Check and define required Defines
 ******************************************************************************/
#ifndef XT1LFOFFG                                               // Defines if not available in
                                                                // header file
#    define XT1LFOFFG   0
#endif

#ifndef XT1HFOFFG                                               // Defines if not available in
                                                                // header file
#    define XT1HFOFFG   0
#endif

#ifndef XT2OFFG                                                 // Defines if not available in
                                                                // header file
#    define XT2OFFG     0
#endif

#ifndef XTS                                                     // Defines if not available in
                                                                // header file
#    define XTS         0
#endif

void LFXT_Start(uint16_t xtdrive)
{
    // If the drive setting is not already set to maximum
    // Set it to max for LFXT startup
    if ((UCSCTL6 & XT1DRIVE_3) != XT1DRIVE_3){
        UCSCTL6_L |= XT1DRIVE1_L + XT1DRIVE0_L;                 // Highest drive setting for
                                                                // XT1startup
    }

    while (UCSCTL7 & XT1LFOFFG){                                // Check LFXT1 fault flag
        UCSCTL7 &= ~(XT1LFOFFG);                                // Clear LFXT1 fault flag

        // Clear the global fault flag. In case the LFXT1 caused the global fault flag to get
        // set this will clear the global error condition. If any error condition persists,
        // global flag will get again.
        SFRIFG1 &= ~OFIFG;
    }

    UCSCTL6 = (UCSCTL6 & ~(XT1DRIVE_3)) | (xtdrive);            // set requested Drive mode
}

uint16_t LFXT_Start_Timeout(uint16_t xtdrive, uint16_t timeout)
{
    // If the drive setting is not already set to maximum
    // Set it to max for LFXT startup
    if ((UCSCTL6 & XT1DRIVE_3) != XT1DRIVE_3){
        UCSCTL6_L |= XT1DRIVE1_L + XT1DRIVE0_L;                 // Highest drive setting for
                                                                // XT1startup
    }

    do {
        UCSCTL7 &= ~(XT1LFOFFG);                                // Clear LFXT1 fault flag

        // Clear the global fault flag. In case the LFXT1 caused the global fault flag to get
        // set this will clear the global error condition. If any error condition persists,
        // global flag will get again.
        SFRIFG1 &= ~OFIFG;
    } while ((UCSCTL7 & XT1LFOFFG) && --timeout);               // Check LFXT1 fault flag

    UCSCTL6 = (UCSCTL6 & ~(XT1DRIVE_3)) | (xtdrive);            // set Drive mode

    if (timeout)
        return (UCS_STATUS_OK);
    else
        return (UCS_STATUS_ERROR);
}

void XT1_Start(uint16_t xtdrive)
{
    // Check if drive value is the expected one
    if ((UCSCTL6 & XT1DRIVE_3) != xtdrive){
        UCSCTL6 &= ~XT1DRIVE_3;                                 // Clear XT1drive field
        UCSCTL6 |= xtdrive;                                     // Set requested value
    }

    UCSCTL6 &= ~XT1OFF;                                         // Enable XT1
    UCSCTL6 |= XTS;                                             // Enable HF mode

    while (UCSCTL7 & (XT1LFOFFG + XT1HFOFFG)){                  // Check XT1 fault flags
        UCSCTL7 &= ~(XT1LFOFFG + XT1HFOFFG);                    // Clear XT1 fault flags

        // Clear the global fault flag. In case the XT1 caused the global fault flag to get
        // set this will clear the global error condition. If any error condition persists,
        // global flag will get again.
        SFRIFG1 &= ~OFIFG;
    }
}

uint16_t XT1_Start_Timeout(uint16_t xtdrive, uint16_t timeout)
{
    // Check if drive value is the expected one
    if ((UCSCTL6 & XT1DRIVE_3) != xtdrive) {
        UCSCTL6 &= ~XT1DRIVE_3;                                 // Clear XT1drive field
        UCSCTL6 |= xtdrive;                                     // Set requested value
    }

    UCSCTL6 &= ~XT1OFF;                                         // Enable XT1
    UCSCTL6 |= XTS;                                             // Enable HF mode

    do {
        UCSCTL7 &= ~(XT1LFOFFG + XT1HFOFFG);                    // Clear XT1 fault flags

        // Clear the global fault flag. In case the XT1 caused the global fault flag to get
        // set this will clear the global error condition. If any error condition persists,
        // global flag will get again.
        SFRIFG1 &= ~OFIFG;
    } while ((UCSCTL7 & (XT1LFOFFG + XT1HFOFFG)) && --timeout); // Check XT1 fault flags

    if (timeout) {
        return UCS_STATUS_OK;
    }
    else {
        return UCS_STATUS_ERROR;
    }
}

void XT1_Bypass(void)
{
    UCSCTL6 |= XT1BYPASS;

    while (UCSCTL7 & (XT1LFOFFG + XT1HFOFFG)) {                 // Check XT1 fault flags
        UCSCTL7 &= ~(XT1LFOFFG + XT1HFOFFG);                    // Clear XT1 fault flags

        // Clear the global fault flag. In case the XT1 caused the global fault flag to get
        // set this will clear the global error condition. If any error condition persists,
        // global flag will get again.
        SFRIFG1 &= ~OFIFG;
    }
}

void XT1_Stop(void)
{
    UCSCTL6 |= XT1OFF;                                          // Switch off XT1 oscillator
}

void XT2_Start(uint16_t xtdrive)
{
#ifdef XT2DRIVE_3
    // Check if drive value is the expected one
    if ((UCSCTL6 & XT2DRIVE_3) != xtdrive){
        UCSCTL6 &= ~XT2DRIVE_3;                                 // Clear XT2 drive field
        UCSCTL6 |= xtdrive;                                     // Set requested value
    }
#endif

    UCSCTL6 &= ~XT2OFF;

    while (UCSCTL7 & XT2OFFG){                                  // Check XT2 fault flag
        UCSCTL7 &= ~(XT2OFFG);                                  // Clear XT2 fault flag
#if defined (__CC430F5133__) || (__CC430F5135__) || (__CC430F5137__) || \
            (__CC430F6125__) || (__CC430F6126__) || (__CC430F6127__) || \
            (__CC430F6135__) || (__CC430F6137__) || (__CC430F5123__) || \
            (__CC430F5125__) || (__CC430F5143__) || (__CC430F5145__) || \
            (__CC430F5147__) || (__CC430F6143__) || (__CC430F6145__) || \
            (__CC430F6147__)
        // CC430 uses a different fault mechanism. It requires 3 VLO clock cycles delay.
        // if 20MHz CPU, 5000 clock cycles are required in worst case.
        __delay_cycles(5000);
#endif

        // Clear the global fault flag. In case the XT2 caused the global fault flag to get
        // set this will clear the global error condition. If any error condition persists,
        // global flag will get again.
        SFRIFG1 &= ~OFIFG;
    }
}

uint16_t XT2_Start_Timeout(uint16_t xtdrive, uint16_t timeout)
{
#ifdef XT2DRIVE_3
    // Check if drive value is the expected one
    if ((UCSCTL6 & XT2DRIVE_3) != xtdrive){
        UCSCTL6 &= ~XT2DRIVE_3;                                 // Clear XT2drive field
        UCSCTL6 |= xtdrive;                                     // Set requested value
    }
#endif

    UCSCTL6 &= ~XT2OFF;

    do {
        UCSCTL7 &= ~(XT2OFFG);                                  // Clear XT2 fault flag
#if defined (__CC430F5133__) || (__CC430F5135__) || (__CC430F5137__) || \
            (__CC430F6125__) || (__CC430F6126__) || (__CC430F6127__) || \
            (__CC430F6135__) || (__CC430F6137__) || (__CC430F5123__) || \
            (__CC430F5125__) || (__CC430F5143__) || (__CC430F5145__) || \
            (__CC430F5147__) || (__CC430F6143__) || (__CC430F6145__) || \
            (__CC430F6147__)
        // CC430 uses a different fault mechanism. It requires 3 VLO clock cycles delay.
        // If 20MHz CPU, 5000 clock cycles are required in worst case.
        __delay_cycles(5000);
#endif

        // Clear the global fault flag. In case the XT2 caused the global fault flag to get
        // set this will clear the global error condition. If any error condition persists,
        // global flag will get again.
        SFRIFG1 &= ~OFIFG;
    } while ((UCSCTL7 & XT2OFFG) && --timeout);                 // Check XT2 fault flag

    if (timeout){
        return UCS_STATUS_OK;
    }
    else {
        return UCS_STATUS_ERROR;
    }
}

void XT2_Bypass(void)
{
#ifdef XT2BYPASS                                                // On devices without XT2 this
                                                                // function will be empty
    UCSCTL6 |= XT2BYPASS;

    while (UCSCTL7 & XT2OFFG) {                                 // Check XT2 fault flag
        UCSCTL7 &= ~(XT2OFFG);                                  // Clear XT2 fault flag

        // Clear the global fault flag. In case the XT2 caused the global fault flag to get
        // set this will clear the global error condition. If any error condition persists,
        // global flag will get again.
        SFRIFG1 &= ~OFIFG;
    }
#endif
}

void XT2_Stop(void)
{
    UCSCTL6 |= XT2OFF;                                          // Switch off XT2 oscillator
}

void Init_FLL_Settle(uint16_t fsystem, uint16_t ratio)
{
    volatile uint16_t x = ratio * 32;

    Init_FLL(fsystem, ratio);

    while (x--){
        __delay_cycles(30);
    }
}

void Init_FLL(uint16_t fsystem, uint16_t ratio)
{
    uint16_t d, dco_div_bits;
    uint16_t mode = 0;

    // Save actual state of FLL loop control, then disable it. This is needed to
    // prevent the FLL from acting as we are making fundamental modifications to
    // the clock setup.
#if __GNUC__ == 4
    uint16_t srRegisterState = __read_status_register() & SCG0;
#else
    uint16_t srRegisterState = __get_SR_register() & SCG0;
#endif

    d = ratio;
    dco_div_bits = FLLD__2;                                     // Have at least a divider of 2

    if (fsystem > 16000){
        d >>= 1;
        mode = 1;
    }
    else {
        fsystem <<= 1;                                          // fsystem = fsystem * 2
    }

    while (d > 512){
        dco_div_bits = dco_div_bits + FLLD0;                    // Set next higher div level
        d >>= 1;
    }

#if __GNUC__ == 4
    __bis_status_register(SCG0);                                    // Disable FLL
#else
    __bis_SR_register(SCG0);                                    // Disable FLL
#endif

    UCSCTL0 = 0x0000;                                           // Set DCO to lowest Tap

    UCSCTL2 &= ~(0x03FF);                                       // Reset FN bits
    UCSCTL2 = dco_div_bits | (d - 1);

    if (fsystem <= 630)                                         //           fsystem < 0.63MHz
        UCSCTL1 = DCORSEL_0;
    else if (fsystem <  1250)                                   // 0.63MHz < fsystem < 1.25MHz
        UCSCTL1 = DCORSEL_1;
    else if (fsystem <  2500)                                   // 1.25MHz < fsystem <  2.5MHz
        UCSCTL1 = DCORSEL_2;
    else if (fsystem <  5000)                                   // 2.5MHz  < fsystem <    5MHz
        UCSCTL1 = DCORSEL_3;
    else if (fsystem <  10000)                                  // 5MHz    < fsystem <   10MHz
        UCSCTL1 = DCORSEL_4;
    else if (fsystem <  20000)                                  // 10MHz   < fsystem <   20MHz
        UCSCTL1 = DCORSEL_5;
    else if (fsystem <  40000)                                  // 20MHz   < fsystem <   40MHz
        UCSCTL1 = DCORSEL_6;
    else
        UCSCTL1 = DCORSEL_7;

#if __GNUC__ == 4
    __bic_status_register(SCG0);                                    // Re-enable FLL
#else
    __bic_SR_register(SCG0);                                    // Re-enable FLL
#endif

    while (UCSCTL7 & DCOFFG) {                                  // Check DCO fault flag
        UCSCTL7 &= ~DCOFFG;                                     // Clear DCO fault flag

        // Clear the global fault flag. In case the DCO caused the global fault flag to get
        // set this will clear the global error condition. If any error condition persists,
        // global flag will get again.
        SFRIFG1 &= ~OFIFG;
    }

#if __GNUC__ == 4
    __bis_status_register(srRegisterState);                         // Restore previous SCG0
#else
    __bis_SR_register(srRegisterState);                         // Restore previous SCG0
#endif

    if (mode == 1) {                                            // fsystem > 16000
        SELECT_MCLK_SMCLK(SELM__DCOCLK + SELS__DCOCLK);         // Select DCOCLK
    }
    else {
        SELECT_MCLK_SMCLK(SELM__DCOCLKDIV + SELS__DCOCLKDIV);   // Select DCODIVCLK
    }
}

uint16_t Clear_All_Osc_Flags(uint16_t timeout)
{
    do {
        UCSCTL7 &= ~(DCOFFG + XT1LFOFFG + XT1HFOFFG + XT2OFFG); // Clear all osc fault flags
#if defined (__CC430F5133__) || (__CC430F5135__) || (__CC430F5137__) || \
            (__CC430F6125__) || (__CC430F6126__) || (__CC430F6127__) || \
            (__CC430F6135__) || (__CC430F6137__) || (__CC430F5123__) || \
            (__CC430F5125__) || (__CC430F5143__) || (__CC430F5145__) || \
            (__CC430F5147__) || (__CC430F6143__) || (__CC430F6145__) || \
            (__CC430F6147__)
        // CC430 uses a different fault mechanism. It requires 3 VLO clock cycles delay.
        // If 20MHz CPU, 5000 clock cycles are required in worst case.
        __delay_cycles(5000);
#endif
        SFRIFG1 &= ~OFIFG;                                      // Clear the global osc fault flag.
    } while ((SFRIFG1 & OFIFG) && --timeout);                   // Check XT1 fault flags

    return (UCSCTL7 & (DCOFFG + XT1LFOFFG + XT1HFOFFG + XT2OFFG));
}

#endif
