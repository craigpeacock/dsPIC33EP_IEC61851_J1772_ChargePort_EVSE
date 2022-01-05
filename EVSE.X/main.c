/*
dsPIC33EP128GS804 IEC61851/SAE J1772 Demo Code
Copyright (C) 2022 Craig Peacock

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

enum {
    STATE_A,
    STATE_A_LOOP, 
    STATE_B,
    STATE_B_LOOP,
    STATE_C,
    STATE_C_LOOP,
    STATE_D,
    STATE_D_LOOP,
    STATE_E,
    STATE_E_LOOP,
};

#define CP_NO_VEHICLE(n)            (n >= 3925)
#define CP_VEHICLE_CONNECTED(n)    ((n >= 3413) && (n <= 3755))
#define CP_READY(n)                ((n >= 2901) && (n <= 3243))
#define CP_READY_VENTILATION(n)    ((n >= 1365) && (n <= 1707))
#define CP_DIODE_CHECK(n)           (n >= 171)

int8_t DiodeCheck(void)
{
    uint16_t n;
    CP_Set(LOW);
    __delay_ms(5);    
    n = CP_ReadAN();  
    CP_Set(HIGH);
    //printf("Diode Check %u\r\n",n);
    if (CP_DIODE_CHECK(n)) {
        printf("Error: Diode test failed\n\r");
        return -1;
    } else 
        return 0;
}

void EnablePower(void)
{
    LockSolenoid(LOCK);
    __delay_ms(100);  
    OUT1 = 1;   // Enable Power
    print_timestamp();
    printf("Power Enabled\r\n");  
}

void DisablePower(void)
{
    OUT1 = 0;   // Disable Power
    print_timestamp();
    printf("Power Disabled\r\n");  
    __delay_ms(100);  
    LockSolenoid(UNLOCK);
}

int main(void) {

    Init_PLL();
    Init_GPIO();
    Init_UART();
    Init_TMR4();
    Init_SOLENOID();
    Init_ADC();
    Init_PWM();
         
    uint16_t state = STATE_A;
    int16_t charge_rate = 100; // 10 Amps 
    uint16_t n;
    
    printf("\r\ndsPIC33EP128GS804 IEC61851/SAE J1772 Demo EVSE Code\r\n");
    
    while (1) {
        switch (state) {
            case STATE_A:
                print_timestamp();
                printf("Ready, no Vehicle Detected\r\n");
                CP_Set(HIGH);      
                __delay_ms(5);
                state = STATE_A_LOOP;
            case STATE_A_LOOP: 
                n = CP_ReadAN(); 
                if (CP_VEHICLE_CONNECTED(n)) state = STATE_B;   
                /*
                 * Vehicles drawing less than 16A, single phase may use a 
                 * simplified circuit (Fixed 882 or 246 ohm resistor) as per
                 * the standard. This means they can jump directly to State C 
                 * or D.
                 */
                if (CP_READY(n))             state = STATE_C;
                if (CP_READY_VENTILATION(n)) state = STATE_D;
                break;

            case STATE_B:
                print_timestamp();
                printf("Vehicle Detected\r\n");
                if (DiodeCheck()) {
                    state = STATE_E;
                    break;
                }
                state = STATE_B_LOOP;
            case STATE_B_LOOP:
                n = CP_ReadAN(); 
                if (CP_NO_VEHICLE(n))        state = STATE_A;
                if (CP_READY(n))             state = STATE_C;
                if (CP_READY_VENTILATION(n)) state = STATE_D;
                break;   
            
            case STATE_C:
                print_timestamp();
                printf("Vehicle Ready for Charging\r\n");
                CP_Set(charge_rate);
                EnablePower();
                state = STATE_C_LOOP;
            case STATE_C_LOOP:
                n = CP_ReadAN(); 
                if (CP_NO_VEHICLE(n))        state = STATE_A;
                if (CP_VEHICLE_CONNECTED(n)) state = STATE_B;                
                if (CP_READY_VENTILATION(n)) state = STATE_D;
                break;
                
            case STATE_D:
                print_timestamp();
                printf("Vehicle Ready for Charging - Ventilation Required\r\n");
                CP_Set(charge_rate);
                EnablePower();
                LockSolenoid(LOCK);
                state = STATE_D_LOOP;
            case STATE_D_LOOP:
                n = CP_ReadAN(); 
                if (CP_NO_VEHICLE(n))        state = STATE_A;
                if (CP_VEHICLE_CONNECTED(n)) state = STATE_B;                
                if (CP_READY(n))             state = STATE_C;
                break;
                
            case STATE_E:
                print_timestamp();
                printf("Error\r\n");
                CP_Set(LOW);
                DisablePower();
                state = STATE_E_LOOP;
            case STATE_E_LOOP:    
                // Stay here until reset.
                break;   
        }

        // Provide heartbeat 
        LED1 = ~LED1;
        __delay_ms(100);  
    }
}
