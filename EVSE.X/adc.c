/*
dsPIC33EP128GS804 IEC61851/SAE J1772 Demo Code
Copyright (C) 2022 Craig Peacock

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "xc.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "adc.h"
#include "board.h"

uint16_t CP_ANResult;
uint8_t CP_ANResultFlag;

void Init_ADC(void)
{
    ANSELCbits.ANSC5 = 1;               // AN0ALT = Analog
    TRISCbits.TRISC5 = 1;               // AN0ALT = Input
    
    ADCON4Hbits.C0CHS = 0b11;           // Select AN0ALT
    
    // Configure the common ADC clock.
    ADCON3Hbits.CLKSEL = 0;             // ADC Module clocked from FSYS System Clock
    ADCON3Hbits.CLKDIV = 8;             // ADC Clock Source Divider (1:8)
    ADCORE0Hbits.ADCS = 8;              // ADC Core Input Divider (1:8)
    
    ADCON3Lbits.REFSEL = 0;             // AVdd as voltage reference
    ADCON1Hbits.FORM = 0;               // Integer format
    ADMOD0Lbits.SIGN0 = 0;              // AN0/RA0 is unsigned
    ADMOD0Lbits.DIFF0 = 0;              // AN0/RA0 is Single Ended
    ADCORE0Hbits.RES = 0b11;            // 12-bit resolution
   
    // Prepare for calibration
    ADCON5Hbits.WARMTIME = 15;          // Set initialization time to max
    ADCON1Lbits.ADON = 1;               // Enable ADC
    ADCON5Lbits.C0PWR = 1;              // Turn on analog power
    while(ADCON5Lbits.C0RDY == 0);      // Wait for core to become ready
    ADCON3Hbits.C0EN = 1;               // Turn on digital power
    
    // Calibrate core 
    ADCAL0Lbits.CAL0EN = 1;             // Enable CAL for the ADC core 0
    ADCAL0Lbits.CAL0DIFF = 0;           // Single-ended input calibration
    ADCAL0Lbits.CAL0RUN = 1;            // Start calibration
    while(ADCAL0Lbits.CAL0RDY == 0);    // Poll for the calibration end
    ADCAL0Lbits.CAL0DIFF = 1;           // Differential input calibration
    ADCAL0Lbits.CAL0RUN = 1;            // Start calibration
    while(ADCAL0Lbits.CAL0RDY == 0);    // Poll for the calibration end
    ADCAL0Lbits.CAL0EN = 0;             // End the core 0 calibration

    // Trigger from PWM Module (Allow software trigger)
    ADTRIG0Lbits.TRGSRC0 = 4;           // Use PWM Special Event Trigger
    
    // Enable interrupts
    _ADCAN0IF = 0;                      // Clear interrupt flag for AN0
    _ADCAN0IE = 1;                      // Enable interrupt for AN0
    ADIELbits.IE0 = 1;                  // Enable interrupt for AN0
    
    //_ADCAN1IF = 0;                      // Clear interrupt flag for AN1
    //_ADCAN1IE = 1;                      // Enable interrupt for AN1
    //ADIELbits.IE1 = 1;                  // Enable interrupt for AN1
}

/*
 * CP_ReadAN() - Read analog value of Control Pilot
 * If PWM is not running, software trigger ADC and return instantaneous value.
 * If PWM is running, obtain value from Positive Pulse (Triggered from PWM module)
 *
 */
uint16_t CP_ReadAN(void)
{
    uint16_t result;

    if (PTCONbits.PTEN) {
        // PWM module running, obtain last value from ISR
        result = CP_ANResult;
    } else {
        // PWM module not running, obtain instantaneous value 
        CP_ANResultFlag = 0;
        ADCON3Lbits.CNVCHSEL = 0b0;         // Select AN0 for single conversion
        ADCON3Lbits.CNVRTCH = 1;            // Trigger software individual channel conversion
        
        while (CP_ANResultFlag == 0);       // Wait for our ISR
        result = CP_ANResult;
    }
    
    return (result);
}

void __attribute__((interrupt, no_auto_psv)) _ADCAN0Interrupt(void)
{
    CP_ANResult = ADCBUF0;
    //printf("ISR: ADCBUF0 %u\r\n", CP_ANResult);  
    CP_ANResultFlag = 1;
    _ADCAN0IF = 0;     
}

void __attribute__((interrupt, no_auto_psv)) _ADCAN1Interrupt(void)
{
    unsigned int Value = ADCBUF0;
    printf("ADCBUF0 %u\r\n", Value);  
    _ADCAN1IF = 0;     
}