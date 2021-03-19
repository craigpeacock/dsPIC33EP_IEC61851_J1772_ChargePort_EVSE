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

#define FCY 40000000UL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpic30.h>

#include "xc.h"
#include "board.h"
#include "pwm.h"
#include "inputcapture.h"
#include "can.h"

int main(void) {
    TRISCbits.TRISC9 = 0;	
    TRISCbits.TRISC10 = 0;
    ANSELCbits.ANSC9 = 0;
    ANSELCbits.ANSC10 = 0;
    
    LATCbits.LATC9 = 1;
    LATCbits.LATC10 = 1;
       
    Init_PLL();             // Initialise System Clock/PLL
    Init_UART();
    Init_PWM();
    Init_InputCapture();
    Init_CAN1();
    
    uint8_t buffer[] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
    
    printf("\r\ndsPIC33EP128GS804 IEC61851/SAE J1772 Demo Code\r\n");

    printf("Generating 1kHz Control Pilot Signal\r\n");
    CP_SetAmps(10.0);
    
    while (1) {
        printf("Sending CAN Message\r\n");
        CAN1_MessageTransmit(0x1FF, sizeof(buffer), buffer, 0, CAN_MSG_TX_DATA_FRAME);
        
        LATCbits.LATC9 = 1;
        __delay_ms(200);
        LATCbits.LATC9 = 0;
        __delay_ms(200);
        
    }
}




