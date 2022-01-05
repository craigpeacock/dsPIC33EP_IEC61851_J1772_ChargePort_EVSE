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

#include <stdio.h>
#include <stdint.h>
#include "xc.h"
#include "board.h"
#include "pwm.h"

void Init_PWM(void)
{
    // PWM Clock provided by Auxiliary Clock. 
    ACLKCONbits.ENAPLL = 0;         // APLL is disabled
    ACLKCONbits.SELACLK = 0;        // Use Primary PLL (120MHz)
    ACLKCONbits.APSTSCLR = 0b111;   // Divided by 1

    // On the dsPIC33EP128GS804, PWMxx Output Pin Ownership is set to 1 by default;
    IOCON1bits.PENH = 0;        // Ensure PWM1H connected to CTRL_PILOT/RA4 is disabled. 
    IOCON1bits.PENL = 0;        // Ensure PWM1L connected to EN_CHARGE/RA3 is disabled.
    IOCON2bits.PENH = 0;        // Ensure PWM2H connected to RB13 is disabled.
    IOCON2bits.PENL = 0;        // Ensure PWM2L connected to RB14 is disabled.
    IOCON3bits.PENH = 0;        // Ensure PWM3H connected to RB11 is disabled.
    IOCON3bits.PENL = 0;        // Ensure PWM3L connected to RB12 is disabled.
    
    TRISCbits.TRISC3 = 0;       // Make RC3 an output. Not required for PWM Peripheral, 
                                // but for CP_Set(High)/(Low)
    
    // Set up re-mappable I/O
    __builtin_write_OSCCONL(OSCCON & ~(1<<6));  // Unlock Peripheral Pin Select Registers
    RPOR11bits.RP51R = 0b0110100;               // Output: Assign PWM4L to RP51/RC3
    __builtin_write_OSCCONL(OSCCON | (1<<6));   // Lock Peripheral Pin Select Registers
    
    // Set up Primary Master PWM Time Base:
    // Tcy = 120MHz / x8/32 Prescaler / 30000 = 1.000kHz switching frequency
    // PWM Module 1:
    PTCONbits.PTEN = 0;         // Disable PWM Module 
    PTCON2bits.PCLKDIV = 0b101; // PWM Time Base Input Clock Prescaler is 1:32
    PTPER = 30000;              // PWM Time Base Register will count up and reset on match
    //STPER = 30000;
    // 1% = 30000/100 = 300
    
    // PWM Module 4:
    // Use primary master time base for sync & clock source
    // SDCx registers provide duty cycle information
    PWMCON4bits.MTBS = 0;       // Use Primary Master Time Base
    PWMCON4bits.CAM = 0;        // Standard Edged-Aligned Mode. Module counts up to PTPER.
        
    // PWM I/O Pair in Independent Output Mode. 
    // PDC4 specifies duty cycle for PWM4H, SDC4 duty cycle for PWM4L 
    IOCON4bits.PMOD = 0b11;
    
    // Enable PWM4L pin. This is not a physical pin, but is a re-mappable output.
    IOCON4bits.PENL = 1;
    
    FCLCON4bits.FLTMOD = 0b11;  // Fault mode disabled
    
    // Set up initial Duty Cycle to 50% (half of PTPER)
    SDC4 = 15000;                 
    
    // Set up special trigger for ADC Positive Pulse Voltage Measurement
    TRIG4 = 9000;               //(3mS settling time)
    //STRIG4 = 25000;
}

/* 
 *  CP_Set() is used to set the Control Pilot signal. Valid options are:
 *   CP_Set(HIGH)    - Set the Pilot High
 *   CP_Set(LOW)     - Set the Pilot Low
 *   CP_Set(DIGITAL) - Set the Pilot to indicate digital 
 *                     communications (5% Duty Cycle)
 *   CP_Set([amps])  - Set current where amps is tenths of an amp - i.e. 10A = 100
 */
void CP_Set(int16_t amps)
{
    
    if (amps == HIGH) {
        PTCONbits.PTEN = 0;         // Disable (disconnect) PWM Module  
        LATCbits.LATC3 = 1;         // Set Digital Pin High
        return;
    }
    
    if (amps == LOW) {
        PTCONbits.PTEN = 0;         // Disable (disconnect) PWM Module 
        LATCbits.LATC3 = 0;         // Set Digital Pin Low
        return;
    }
    
    if (amps == DIGITAL) {
        // Set Control Pilot to use Digital Communication (5% Duty Cycle)
        SDC4 = 5 * 300;
        PTCONbits.PTEN = 1;         // Enable PWM Module 
        return;
    }
    
    if (amps < 60) { 
        // Minimum value is 6 amps.
        amps = 60;
    }
    
    if ((amps >= 60) && (amps < 510)) {
        SDC4 = amps / 6 * 300; 
        PTCONbits.PTEN = 1;         // Enable PWM Module
        print_timestamp();
        printf("Setting Control Pilot to %02d.%01d Amps, SDC4 = 0x%04X\r\n",amps/10, amps%10, SDC4);
    } else { 
        SDC4 = (amps / 25 + 64) * 300; 
        PTCONbits.PTEN = 1;         // Enable PWM Module 
    }
    
}
