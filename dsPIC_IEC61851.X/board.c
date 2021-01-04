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
#include <stdlib.h>
#include <libpic30.h>
#include "xc.h"
#include "board.h"

// FGS
#pragma config GWRP = OFF               // General Segment Write-Protect bit (General Segment may be written)
#pragma config GSS = OFF                // General Segment Code-Protect bit (General Segment Code protect is disabled)
#pragma config GSSK = OFF               // General Segment Key bits (General Segment Write Protection and Code Protection is Disabled)

// FOSCSEL
#pragma config FNOSC = FRC              // Initial Oscillator Source Selection bits (Internal Fast RC (FRC))
#pragma config IESO = ON                // Two-speed Oscillator Start-up Enable bit (Start up device with FRC, then switch to user-selected oscillator source)

// FOSC
#pragma config POSCMD = XT              // Primary Oscillator Mode Select bits (XT Crystal Oscillator Mode)
#pragma config OSCIOFNC = OFF           // OSC2 Pin Function bit (OSC2 is clock output)
#pragma config IOL1WAY = OFF            // Peripheral pin select configuration (Allow multiple reconfigurations)
#pragma config FCKSM = CSECMD           // Clock Switching Mode bits (Clock switching is enabled,Fail-safe Clock Monitor is disabled)

// FWDT
#pragma config WDTPOST = PS32768        // Watchdog Timer Postscaler bits (1:32,768)
#pragma config WDTPRE = PR128           // Watchdog Timer Prescaler bit (1:128)
#pragma config PLLKEN = ON              // PLL Lock Wait Enable bit (Clock switch to PLL source will wait until the PLL lock signal is valid.)
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable bit (Watchdog Timer in Non-Window mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable bit (Watchdog timer enabled/disabled by user software)

// FPOR
#pragma config FPWRT = PWR128           // Power-on Reset Timer Value Select bits (128ms)
#pragma config BOREN = ON               // Brown-out Reset (BOR) Detection Enable bit (BOR is enabled)
#pragma config ALTI2C1 = OFF            // Alternate I2C pins for I2C1 (SDA1/SCK1 pins are selected as the I/O pins for I2C1)

// FICD
#pragma config ICS = PGD1               // ICD Communication Channel Select bits (Communicate on PGEC1 and PGED1)
#pragma config RSTPRI = PF              // Reset Target Vector Select bit (Device will obtain reset instruction from Primary flash)
#pragma config JTAGEN = OFF             // JTAG Enable bit (JTAG is disabled)

// FAS
#pragma config AWRP = OFF               // Auxiliary Segment Write-protect bit (Aux Flash may be written)
#pragma config APL = OFF                // Auxiliary Segment Code-protect bit (Aux Flash Code protect is disabled)
#pragma config APLK = OFF               // Auxiliary Segment Key bits (Aux Flash Write Protection and Code Protection is Disabled) 


void Init_PLL(void)
{
    // Set up PLL
    // 8MHz Primary Osc /2 * 40 /2 = FOSC = 80MHz
    // CPU Clock is Fosc /2 = 40MHz (40MIPS)
    PLLFBD = 38;                        // PLL Feedback Divisor Bits M  = 40
    CLKDIVbits.PLLPOST = 0;             // PLL Phase Detector Input Divider N1 = /2
    CLKDIVbits.PLLPRE = 0;              // PLL VCO Output Divider N2 = /2
    OSCTUN = 0;                         // FRC Oscillator Tuning Bits

    // Initiate Clock Switch to Primary Oscillator with PLL (NOSC=0x3)
    __builtin_write_OSCCONH(0x03);      // NOSC = 0x03
    __builtin_write_OSCCONL(0x01);      // Request Osc Switch to Selection Specified by NOSC
    while (OSCCONbits.COSC != 0x3);     // Wait until Primary Oscillator with PLL
}

void Init_AUXPLL(void)
{
    // Set up Auxiliary Clock for USB
    // 8MHz Primary OSC / 2 * 24 / 2 = 48MHz
    ACLKCON3bits.SELACLK = 1;           // Auxiliary PLL provides the source clock for auxiliary clock divider
    ACLKCON3bits.ASRCSEL = 1;           // Primary oscillator is the clock source for APLL
    ACLKCON3bits.APLLPOST = 0b110;      // PLL VCO Output Divider - Divide by 2
    ACLKCON3bits.APLLPRE = 0b001;       // PLL Phase Detector - Divide by 2
    ACLKDIV3 = 0x7;                     // PLL Feedback Divisor Bits 24
    ACLKCON3bits.ENAPLL = 1;            // Enable PLL
    while(ACLKCON3bits.APLLCK != 1);    // Wait for lock
}

void Init_UART(void)
{
    __builtin_write_OSCCONL(OSCCON & ~(1<<6));  // Unlock Peripheral Pin Select Registers
    RPOR0bits.RP64R = 1;                        // Assign UART1 TX to RP64 (RD0)
    __builtin_write_OSCCONL(OSCCON | (1<<6));   // Lock Peripheral Pin Select Registers
    
    __C30_UART=1;           // Select UART1 for STDIO
    U1BRG = 129;            // Baud Rate 80MHz/2/(16*19200 BPS) - 1
    U1MODEbits.UARTEN = 1;	// Enable UART
}
