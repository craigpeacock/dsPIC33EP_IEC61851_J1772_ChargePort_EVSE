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

enum STATE {
    IDLE,
    ERROR,
    REQ_DIGITAL_COMMS,
    PWM_CHARGING,
    DISCONNECT
};

int main(void) {
   
    // Setup GPIO
    LED1_ANSEL = 0;
    LED2_ANSEL = 0;
    
    LED1 = 0;
    LED2 = 0;
        
    LED1_DIR = 0;
    LED2_DIR = 0;
    
    CHARGE_EN = 0;
    CHARGE_EN_DIR = 0;
      
    Init_PLL();             
    Init_UART();
    Init_InputCapture();
    Init_CAN1();
   
    printf("\r\ndsPIC33EP128GS804 IEC61851/SAE J1772 Demo Code\r\n");
    
    unsigned int ChargeRate;
    enum STATE state = IDLE;
    
    while (1) {
        
        ChargeRate = Get_CP_ChargeRate();
        if (ChargeRate == CP_ERROR) state = ERROR;
        else if (ChargeRate == CP_REQ_DIGITAL_MODE) state = REQ_DIGITAL_COMMS;
        
        switch (state) {
            
            case IDLE:
                printf("Idle\r\n");
                if (ChargeRate >= 600) state = PWM_CHARGING;
                break;
            
            case ERROR:
                printf("Error\r\n");
                state = IDLE;
                break;

            case REQ_DIGITAL_COMMS:
                // Request for digital comms on control pilot.
                printf("Requesting Digital Communication\r\n");
                // Do nothing at this stage
                state = IDLE;
                break;
            
            case PWM_CHARGING:
                if (ChargeRate == 0) state = DISCONNECT;
                else {
                    printf("Maximum Charge Rate %u.%01uA\r\n", ChargeRate / 100, ChargeRate % 100);
                    CHARGE_EN = 1;
                    // Communicate with our charger here
                }
                break;
                
            case DISCONNECT:
                // Request to stop charging and disconnect
                printf("Preparing for disconnect\r\n");
                // Tell our charger to stop pulling current
                
                // Stop charging
                CHARGE_EN = 0;      

                // Unlock chargeport

                state = IDLE;
                break;
        }
        
        LED1 = ~LED1;
        __delay_ms(100);
    }
}
