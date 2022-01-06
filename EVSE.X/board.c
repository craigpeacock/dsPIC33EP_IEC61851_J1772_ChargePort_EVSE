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
#include <stdlib.h>
#define FCY 40000000UL
#include <libpic30.h>
#include "xc.h"
#include "board.h"

// FSEC
#pragma config BWRP = OFF               // Boot Segment Write-Protect bit (Boot Segment may be written)
#pragma config BSS = DISABLED           // Boot Segment Code-Protect Level bits (No Protection (other than BWRP))
#pragma config BSEN = OFF               // Boot Segment Control bit (No Boot Segment)
#pragma config GWRP = OFF               // General Segment Write-Protect bit (General Segment may be written)
#pragma config GSS = DISABLED           // General Segment Code-Protect Level bits (No Protection (other than GWRP))
#pragma config CWRP = OFF               // Configuration Segment Write-Protect bit (Configuration Segment may be written)
#pragma config CSS = DISABLED           // Configuration Segment Code-Protect Level bits (No Protection (other than CWRP))
#pragma config AIVTDIS = OFF            // Alternate Interrupt Vector Table bit (Disabled AIVT)

// FBSLIM
#pragma config BSLIM = 0x1FFF           // Boot Segment Flash Page Address Limit bits (Enter Hexadecimal value)

// FSIGN

// FOSCSEL
#pragma config FNOSC = FRC              // Oscillator Source Selection (Primary Oscillator (XT, HS, EC))
#pragma config IESO = ON                // Two-speed Oscillator Start-up Enable bit (Start up device with FRC, then switch to user-selected oscillator source)

// FOSC
#pragma config POSCMD = HS              // Primary Oscillator Mode Select bits (HS Crystal Oscillator Mode)
#pragma config OSCIOFNC = OFF           // OSC2 Pin Function bit (OSC2 is clock output)
#pragma config IOL1WAY = OFF            // Peripheral pin select configuration bit (Allow only one reconfiguration)
#pragma config FCKSM = CSECMD           // Clock Switching Mode bits (Clock switching is enabled,Fail-safe Clock Monitor is disabled)       
#pragma config PLLKEN = ON              // PLL Lock Enable Bit (Clock switch to PLL source will wait until the PLL lock signal is valid)

// FWDT
#pragma config WDTPOST = PS32768        // Watchdog Timer Postscaler bits (1:32,768)
#pragma config WDTPRE = PR128           // Watchdog Timer Prescaler bit (1:128)
#pragma config WDTEN = OFF              // Watchdog Timer Enable bit (Watchdog timer enabled/disabled by user software)
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable bit (Watchdog Timer in Non-Window mode)
#pragma config WDTWIN = WIN25           // Watchdog Timer Window Select bits (WDT Window is 25% of WDT period)

// FPOR

// FICD
#pragma config ICS = PGD1               // ICD Communication Channel Select bits (Communicate on PGEC1 and PGED1)
#pragma config JTAGEN = OFF             // JTAG Enable bit (JTAG is disabled)
#pragma config BTSWP = OFF              // BOOTSWP Instruction Enable/Disable bit (BOOTSWP instruction is disabled)

// FDEVOPT
#pragma config PWMLOCK = OFF            // PWMx Lock Enable bit (Certain PWM registers may only be written after key sequency)
#pragma config ALTI2C1 = ON             // Alternate I2C1 Pin bit (I2C1 mapped to ASDA1/ASCL1 pins)
#pragma config ALTI2C2 = OFF            // Alternate I2C2 Pin bit (I2C2 mapped to SDA2/SCL2 pins)
#pragma config DBCC = OFF               // DACx Output Cross Connection bit (No Cross Connection between DAC outputs)

// FALTREG
#pragma config CTXT1 = OFF              // Specifies Interrupt Priority Level (IPL) Associated to Alternate Working Register 1 bits (Not Assigned)
#pragma config CTXT2 = OFF              // Specifies Interrupt Priority Level (IPL) Associated to Alternate Working Register 2 bits (Not Assigned)
#pragma config CTXT3 = OFF              // Specifies Interrupt Priority Level (IPL) Associated to Alternate Working Register 2 bits (Not Assigned)
#pragma config CTXT4 = OFF              // Specifies Interrupt Priority Level (IPL) Associated to Alternate Working Register 2 bits (Not Assigned)

// FBTSEQ
#pragma config BSEQ = 0xFFF             // Relative value defining which partition will be active after device Reset; the partition containing a lower boot number will be active (Enter Hexadecimal value)
#pragma config IBSEQ = 0xFFF            // The one's complement of BSEQ; must be calculated by the user and written during device programming. (Enter Hexadecimal value)

uint16_t ticks;
uint16_t secs;

void Init_PLL(void)
{
    // Set up PLL
    // 12MHz Primary Osc /2 * 40 /4 = FOSC = 120MHz
    // CPU Clock Fosc /2 = 60MHz (60MIPS)
    // Peripheral Clock (Fp) = Fosc /2 
    PLLFBD = 38;                        // PLL Feedback Divisor Bits M  = 40
    CLKDIVbits.PLLPOST = 0b01;          // PLL Phase Detector Input Divider N1 = /4 (Default))
    CLKDIVbits.PLLPRE = 0b00000;        // PLL VCO Output Divider N2 = /2 (Default))
    OSCTUN = 0;                         // FRC Oscillator Tuning Bits

    // Initiate Clock Switch to Primary Oscillator with PLL (NOSC=0x3)
    __builtin_write_OSCCONH(0x03);      // NOSC = 0x03
    __builtin_write_OSCCONL(0x01);      // Request Osc Switch to Selection Specified by NOSC
    while (OSCCONbits.COSC != 0x3);     // Wait until Primary Oscillator with PLL
}

void Init_UART(void)
{
    __builtin_write_OSCCONL(OSCCON & ~(1<<6));  // Unlock Peripheral Pin Select Registers
    RPOR8bits.RP44R = 1;                        // Assign UART1 TX to RP44 (RB12)
    __builtin_write_OSCCONL(OSCCON | (1<<6));   // Lock Peripheral Pin Select Registers
    
    __C30_UART=1;           // Select UART1 for STDIO
    U1BRG = 64;             // Baud Rate (60MHz/2) / (4*115200 BPS) - 1
    U1MODEbits.BRGH = 1;    // 4 Clocks per bit period
    U1MODEbits.UARTEN = 1;	// Enable UART
}

void Init_GPIO(void)
{
    LED1_ANSEL = 0;
    LED2_ANSEL = 0;

    LED1 = 0;
    LED2 = 0;
        
    LED1_DIR = 0;
    LED2_DIR = 0;
    
    CHARGE_EN = 0;
    CHARGE_EN_DIR = 0;
    
    SWCAN_EN = 0;
    SWCAN_EN_ANSEL = 0;
    SWCAN_EN_DIR = 0;
    
    OUT1_DIR = 0;
    OUT1_ANSEL = 0;
    OUT1 = 0;
}

void Init_SOLENOID(void)
{
    // Setup Locking Solenoid
    LOCK_IN_1_DIR = 0;
    LOCK_IN_2_DIR = 0;
    LOCK_IN_1 = 0;
    LOCK_IN_2 = 0;
    
    UNLOCKEDSW_DIR = 1;
    UNLOCKEDSW_ANSEL = 0;   // Digital
}

void LockSolenoid(unsigned int lock)
{
    if (lock) {
        // Check if we are unlocked
        if (UNLOCKEDSW) {
            // Lock
            print_timestamp();
            printf("Locking Charge Port\r\n");
            LOCK_IN_1 = 1;
            __delay_ms(200);
            LOCK_IN_1 = 0;
            
        }
    } else {
        // Check if we are locked
        if (!UNLOCKEDSW) {
            // Unlock 
            print_timestamp();
            printf("Unlocking Charge Port\r\n");
            LOCK_IN_2 = 1;
            __delay_ms(200);
            LOCK_IN_2 = 0;
        }
    }

    //if (UNLOCKEDSW) printf("LOCK: Unlocked\r\n");
    //else            printf("LOCK: Locked\r\n");
}

void Init_TMR4(void)
{
    // Set-up Timer 4 to trigger every 10mS
    // 30MHz / 64 (prescaler) = 2.13uS * 4688 = 10.0mS
    T4CONbits.TCS = 0;              // Clock from peripheral clock (40MHz)
    T4CONbits.TCKPS = 0b10;         // 1:64 prescale
    PR4 = 4688;                     
    T4CONbits.TON = 1;              // Turn timer on
    ticks = 0;
    secs = 0;
    IFS1bits.T4IF = 0;              // Clear T3 interrupt flag
    IPC6bits.T4IP = 1;              // Set interrupt priority as 1
    IEC1bits.T4IE = 1;              // Enable T3 interrupt       
}

void __attribute__ ((__interrupt__, no_auto_psv)) _T4Interrupt(void)
{
    ticks++;
    if (ticks == 100) {
        secs++;
        ticks = 0;
    }
    IFS1bits.T4IF = 0; 
}

