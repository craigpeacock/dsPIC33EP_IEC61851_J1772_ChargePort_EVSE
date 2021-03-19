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

#define LED1 	LATEbits.LATE0
#define LED2	LATEbits.LATE1
#define LED3 	LATEbits.LATE2
#define LED4	LATEbits.LATE3

#define SW1 	PORTEbits.RE4
#define SW2     PORTEbits.RE5
#define SW3     PORTEbits.RE6
#define SW4     PORTEbits.RE7

#define INA     LATDbits.LATD8
#define INB     LATDbits.LATD9
#define INA_DIR TRISDbits.TRISD8
#define INB_DIR TRISDbits.TRISD9

void Init_PLL(void);
void Init_AUXPLL(void);
void Init_UART(void);

#endif	/* XC_HEADER_TEMPLATE_H */

