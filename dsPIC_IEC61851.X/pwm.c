/*
dsPIC33EP256MU806 IEC61851/SAE J1772 Demo Code
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
#include "xc.h"
#include "board.h"
#include "pwm.h"

void Init_PWM(void)
{
    // Set up Primary Master PWM Time Base:
    // Tcy = 80MHz / 16 Prescaler / 5000 = 1.000kHz switching frequency
    // PWM Module 1:
    PTCON2bits.PCLKDIV = 0b100; // PWM Time Base Input Clock Prescaler is 1:16
    PTPER = 5000;               // PWM Time Base Register will count up and reset on match
    PTCONbits.PTEN = 1;         // Enable PWM Module
    
    // PWM Module 1:
    // Use primary master time base for sync & clock source
    // SDCx registers provide duty cycle information
    PWMCON1bits.CAM = 0;        // Standard Edged-Aligned Mode. Module counts up to PTPER.
    
    // PWM I/O Pair in Independent Output Mode. 
    // PDC1 specifies duty cycle for PWM1H, SDC1 duty cycle for PWM1L 
    IOCON1bits.PMOD = 0b11;
    
    // Enable PWM1L pin (connected to RE0)
    IOCON1bits.PENL = 1;
    
    // Set up initial Duty Cycle to 50% (half of PTPER)
    SDC1 = 127;                 
}

void CP_DigitalComms(void)
{
    // Set Control Pilot to use Digital Communication (5% Duty Cycle)
    SDC1 = 5 * 50;
}

void CP_SetAmps(double amps)
{
    if (amps < 6) { 
        amps = 6;
    }
    
    if ((amps >= 6) && (amps < 51)) {
        SDC1 = amps / 0.6 * 50; 
    } else { 
        SDC1 = (amps / 2.5 + 64) * 50; 
    }
    printf("Setting Control Pilot to %.02f Amps, SDC1 = 0x%04X\r\n",amps, SDC1);
}
