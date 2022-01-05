/*
dsPIC33EP128GS804 IEC61851/SAE J1772 Demo Code
Copyright (C) 2021 Craig Peacock

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
#include "adc.h"
#include "board.h"

struct PROXIMITY proximity;

void Init_ADC(void)
{
    proximity.previous_value = 0;
    proximity.has_changed = 0;
    
    ANSELAbits.ANSA0 = 1;               // AN0 = Analog
    TRISAbits.TRISA0 = 1;               // AN0 = Input
    
    ADCON4Hbits.C0CHS = 0x00;           // Select AN0
    // ADCON4Hbits.C0CHS = 0x11;        // Select AN0ALT
    
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

    // Set-up Timer 2 to trigger ever 10mS / 100 samples a second
    // 40MHz / 64 (prescaler) / 6250 (PR2)
    ADTRIG0Lbits.TRGSRC0 = 13;          // Use TMR2 for AN0 trigger
    T2CONbits.TCS = 0;                  // Clock from peripheral clock (40MHz)
    T2CONbits.TCKPS = 0b10;             // 1:64 prescale
    PR2 = 6250;                         // Trig every 6250 clocks
    T2CONbits.TON = 1;                  // Turn timer on
    
    // Set-up Filter to average last 16 samples
    ADFL0CONbits.MODE = 0b11;           // Averaging Mode
    //ADFL0CONbits.OVRSAM = 0b001;        // x16
    ADFL0CONbits.OVRSAM = 0b101;        // x64
    ADFL0CONbits.FLCHSEL = 0;           // Channel 0
    _ADFLTR0IF = 0;                     // Clear interrupt flag for ADFLTR0
    _ADFLTR0IE = 1;                     // Enable interrupt for ADFLTR0
    ADFL0CONbits.IE = 1;                // Enable interrupts
    ADFL0CONbits.FLEN = 1;              // Filter is enabled
    
    // Enable interrupts
    //_ADCAN0IF = 0;                      // Clear interrupt flag for AN0
    //_ADCAN0IE = 1;                      // Enable interrupt for AN0
    //ADIELbits.IE0 = 1;                  // Enable interrupt for AN0
    
    TRISBbits.TRISB13 = 0;

}

// Filtered result - Occurs every 10mS
void __attribute__((interrupt, no_auto_psv)) _ADFLTR0Interrupt(void)
{
    LATBbits.LATB13 = 1;
    
    // Check if the value has significantly changed since last sample
    if ((ADFL0DAT < (proximity.previous_value - 20)) | (ADFL0DAT > (proximity.previous_value + 20))) {
        // Value has changed
        //printf("ADFL0DAT %u\r\n", ADFL0DAT);
        proximity.has_changed = true;
        proximity.raw_value = ADFL0DAT;
    }

    proximity.previous_value = ADFL0DAT;
    
    LATBbits.LATB13 = 0;
    _ADFLTR0IF = 0;
}

// Raw/unfiltered result - Occurs every 10mS
void __attribute__((interrupt, no_auto_psv)) _ADCAN0Interrupt(void)
{
    unsigned int data = ADCBUF0;
    LATBbits.LATB13 = 1;
    printf("ADCBUF0 %u\r\n", data);  
    //printf(", %.02f", (((double)data / 4095) * 3.3 ));
    LATBbits.LATB13 = 0;
    _ADCAN0IF = 0;     
}

#define TOLERANCE_COUNTS    40
// 20 counts = +/- ~0.5% 
// 40 counts = +/- ~ 1%

#define PROX_TOLERANCE(raw_value, ideal_value)       (raw_value > (ideal_value - TOLERANCE_COUNTS)) & (raw_value < (ideal_value + TOLERANCE_COUNTS)) 

int Get_Proximity(void)
{
    print_timestamp();
    printf("PP: %.02fV ", (((double)proximity.raw_value / 4095) * 3.3 ));
    printf("(ADC_%04d) ",proximity.raw_value);
    
    if (proximity.raw_value > 4076) {
        printf("Port unplugged\r\n");
        return(PP_UNPLUGGED);
    }
    if (PROX_TOLERANCE(proximity.raw_value, 1868)) {
        printf("Tethered cable attached\r\n");
        return(PP_TETHERED);
    }
    if (PROX_TOLERANCE(proximity.raw_value, 3345)) {
        printf("Tethered cable, release button pressed\r\n");
        return(PP_RELEASE);
    }
    //We are unable to detect 13A cable as we clip values above 3.3V...
    //if (PROX_TOLERANCE(proximity.raw_value, 4095)) {
    //    printf("13A detachable cable detected\r\n");
    //    return(13);
    //}
    if (PROX_TOLERANCE(proximity.raw_value, 3861)) {
        printf("20A detachable cable detected\r\n");
        return(20);
    }
    if (PROX_TOLERANCE(proximity.raw_value, 2367)) {
        printf("32A detachable cable detected\r\n");
        return(32);
    }
    if (PROX_TOLERANCE(proximity.raw_value, 1403)) {
        printf("63A detachable cable detected\r\n");
        return(63);
    }
    if (PROX_TOLERANCE(proximity.raw_value, 804)) {
        printf("80A detachable cable detected\r\n");
        return(80);
    }
    printf("Unknown state\r\n");
    return(PP_ERROR);
}