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
#include <string.h>
#include <libpic30.h>

#include "xc.h"
#include "board.h"
#include "pwm.h"
#include "inputcapture.h"

int main(void) {
    
    Init_PLL();             // Initialise System Clock/PLL
    Init_UART();
    Init_PWM();
    Init_InputCapture();
    
    ANSELE = 0x0;           // Use PORTE as digital IO
    TRISEbits.TRISE1 = 1;	// E1 is input for InputCapture

    printf("\r\ndsPIC33EP256MU806 IEC61851/SAE J1772 Demo Code\r\n");

    printf("Generating 1kHz Control Pilot Signal\r\n");
    
    CP_SetAmps(10.0);
    
    while (1) {
        
    }
     
    return 0;
}




