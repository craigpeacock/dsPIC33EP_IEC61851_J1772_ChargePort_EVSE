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
#include <stdbool.h>

#include "xc.h"
#include "board.h"
#include "pwm.h"
#include "inputcapture.h"
#include "can.h"
#include "adc.h"

#define ENABLE_SWCAN

enum STATE {
    S_IDLE,
    S_ERROR,
    S_REQ_DIGITAL_COMMS,
    S_DIGITAL_COMMS,
    S_PWM_CHARGING,
    S_DISCONNECT,
    S_UNLOCK
};

int main(void) {

    Init_PLL();
    Init_GPIO();
    Init_UART();
    Init_TMR4();
    Init_SOLENOID();
    Init_InputCapture();
    //Init_CAN1();
    Init_SWCAN2();
    Init_ADC();
   
    printf("\r\ndsPIC33EP128GS804 IEC61851/SAE J1772 Demo Code\r\n");
    
    // Unlock Charge Port
    if (!UNLOCKEDSW) LockSolenoid(UNLOCK);
    
    int ChargeRate;
    int CableRate;
    enum STATE state = S_IDLE;
    
    while (1) {

        if (proximity.has_changed | control_pilot.has_changed) {
        
            if (proximity.has_changed) {
                CableRate = Get_Proximity();
                proximity.has_changed = false;
            }

            if (control_pilot.has_changed & (state != S_DIGITAL_COMMS)) {
                ChargeRate = Get_CP_ChargeRate();
                if (ChargeRate == CP_ERROR) state = S_ERROR;
                if (ChargeRate == CP_REQ_DIGITAL_MODE) state = S_REQ_DIGITAL_COMMS;
                if (ChargeRate >= 600) state = S_PWM_CHARGING;
                if (ChargeRate == 0) state = S_DISCONNECT;
                control_pilot.has_changed = false;
            }

            switch (state) {

                case S_IDLE:
                    //printf("Idle\r\n");
                    break;

                case S_ERROR:
                    print_timestamp();
                    printf("Error\r\n");
                    // Handle error here
                    state = S_IDLE;
                    break;
                    
                case S_REQ_DIGITAL_COMMS:
                    // Request for digital comms on control pilot.
                    print_timestamp();
                    printf("Requesting digital communication\r\n");
#ifdef ENABLE_SWCAN
                    CHARGE_EN = 1;
                    __delay_ms(100);
                    print_timestamp();
                    printf("Enabling SWCAN Interface\r\n");
                    SWCAN_EN = 1;
                    state = S_DIGITAL_COMMS;
#else
                    // Do nothing at this stage
                    state = S_IDLE;
#endif
                    break;
                    
                case S_DIGITAL_COMMS:
                    break;

                case S_PWM_CHARGING:
                    LockSolenoid(LOCK);
                    print_timestamp();
                    printf("Maximum charge rate %u.%01uA ", ChargeRate / 100, ChargeRate % 100);
                    if (CableRate > 0) printf("(Cable rated to %dA) ",CableRate);
                    printf("\r\n");
                    // Tell EVSE to deliver power
                    CHARGE_EN = 1;
                    // Communicate with our charger here
                    break;

                case S_DISCONNECT:
                    // Request to stop charging and disconnect
                    //printf("Preparing for disconnect\r\n");
                    // Tell our charger to stop pulling current

                    // Tell EVSE to switch off power
                    CHARGE_EN = 0;      
                      
                    state = S_IDLE;
                    break;
                
                case S_UNLOCK:
                    // Unlock chargeport
                    LockSolenoid(UNLOCK);  
                    state = S_IDLE;
                    break;
            }
        }

        LED1 = ~LED1;
        __delay_ms(100);

    }
}
