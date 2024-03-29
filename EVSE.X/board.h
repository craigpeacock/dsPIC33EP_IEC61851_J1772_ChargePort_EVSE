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

#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H

#define LED1            LATCbits.LATC9
#define LED2            LATCbits.LATC10

#define LED1_DIR        TRISCbits.TRISC9
#define LED2_DIR        TRISCbits.TRISC10

#define LED1_ANSEL      ANSELCbits.ANSC9
#define LED2_ANSEL      ANSELCbits.ANSC10

#define CHARGE_EN       LATAbits.LATA3
#define CHARGE_EN_DIR   TRISAbits.TRISA3

#define SWCAN_EN        LATAbits.LATA1
#define SWCAN_EN_DIR    TRISAbits.TRISA1
#define SWCAN_EN_ANSEL  ANSELAbits.ANSA1

// Contactor Outputs
#define OUT1            LATCbits.LATC6
#define OUT1_DIR        TRISCbits.TRISC6
#define OUT1_ANSEL      ANSELCbits.ANSC6

#define OUT2            LATBbits.LATB12
#define OUT2_DIR        TRISBbits.TRISB12

// Locking Solenoid
#define LOCK_IN_1       LATCbits.LATC0
#define LOCK_IN_2       LATCbits.LATC13
#define LOCK_IN_1_DIR   TRISCbits.TRISC0
#define LOCK_IN_2_DIR   TRISCbits.TRISC13

#define UNLOCKEDSW        PORTCbits.RC12
#define UNLOCKEDSW_DIR    TRISCbits.TRISC12
#define UNLOCKEDSW_ANSEL  ANSELCbits.ANSC12

#define LOCK            true
#define UNLOCK          false

extern uint16_t ticks;
extern uint16_t secs;

#define print_timestamp()   printf("%03d:%02d.%02d ", secs/60, secs%60, ticks);

void Init_PLL(void);
void Init_AUXPLL(void);
void Init_UART(void);
void Init_GPIO(void);
void Init_SOLENOID(void);
void Init_TMR4(void);
void LockSolenoid(unsigned int lock);

#endif	/* XC_HEADER_TEMPLATE_H */

