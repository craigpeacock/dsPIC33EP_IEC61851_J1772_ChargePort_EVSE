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
#include <stdbool.h>
#include "xc.h"
#include "board.h"
#include "inputcapture.h"

unsigned int t_on;
unsigned int period;
bool active;

void Init_InputCapture(void)
{
    /* 
     * Measure Duty Cycle of 1kHz Control Pilot Signal connected to RP20/RA4.
     * 
     * To do this, we use two Input Compare modules connected to the same source. 
     * One (IC1) is tasked with measuring the positive cycle and the other (IC2) 
     * the negative cycle. 
     * 
     * The count-up timer in IC1 is reset by a sync event from IC2 (rising edge 
     * on RE1) and captures the timer count on a falling edge. This count, the 
     * positive pulse width, is written to IC1BUF. 
     * 
     * The opposite occurs for IC2 with the negative pulse width written to IC2BUF.
     * 
     * Both timers in IC1/IC2 are clocked from the peripheral clock (40MHz) with
     * no divisor. Hence 1 count represents 25nS. The 16 bit register can record 
     * a pulse length of 1.63mS before rolling over.  
     *
     */    
    
    TRISAbits.TRISA4 = 1;           // A4 is input for InputCapture
    CNPUAbits.CNPUA4 = 1;           // Enable weak pull-up
    
    __builtin_write_OSCCONL(OSCCON & ~(1<<6));  // Unlock Peripheral Pin Select Registers
    RPINR7bits.IC1R = 20;                       // RP20/RA4
    RPINR7bits.IC2R = 20;                       // RP20/RA4
    __builtin_write_OSCCONL(OSCCON | (1<<6));   // Lock Peripheral Pin Select Registers

    // Input Compare 1 - Positive Cycle
    IC1CON1bits.ICM = 0b000;        // Disable IC1 module
    IC1CON1bits.ICTSEL = 7;         // Select peripheral clock as the IC1 timebase
    IC1CON1bits.ICI = 0b00;         // Interrupt on every capture event
    IC1CON1bits.ICM = 0b010;        // Generate capture event on falling edge
    IC1CON2bits.ICTRIG = 0;         // Operate timer in Sync mode
    IC1CON2bits.SYNCSEL = 0b10001;  // Sync on IC2 event
    
    // Input Compare 2 - Negative Cycle
    IC2CON1bits.ICM = 0b000;        // Disable IC2 module
    IC2CON1bits.ICTSEL = 7;         // Select peripheral clock as the IC2 timebase
    IC2CON1bits.ICI = 0b00;         // Interrupt on every capture event
    IC2CON1bits.ICM = 0b011;        // Generate capture event on rising edge
    IC2CON2bits.ICTRIG = 0;         // Operate timer in Sync mode
    IC2CON2bits.SYNCSEL = 0b10000;  // Sync on IC1 event

    active = false;
    
    IFS0bits.IC1IF = 0;             // Clear the IC1 interrupt status flag
    IFS0bits.IC2IF = 0;             // Clear the IC2 interrupt status flag
    IPC0bits.IC1IP = 1;             // Set module interrupt priority as 1
    IPC1bits.IC2IP = 1;             // Set module interrupt priority as 1
    IEC0bits.IC1IE = 1;             // Enable IC1 interrupts
    IEC0bits.IC2IE = 1;             // Enable IC2 interrupts
}

void __attribute__ ((__interrupt__, no_auto_psv)) _IC1Interrupt(void)
{
    // Occurs on rising edge, do nothing - wait for full cycle.
    // printf("IC1BUF = 0x%04X\r\n",IC1BUF);
    IFS0bits.IC1IF = 0; // Reset respective interrupt flag
}

void __attribute__ ((__interrupt__, no_auto_psv)) _IC2Interrupt(void)
{
    // Occurs on falling edge, end of cycle
    // IC1BUF holds duration of positive pulse 
    // IC2BUF holds duration of negative pulse
    t_on = IC1BUF;
    period = IC1BUF + IC2BUF;
    active = true;

    IFS0bits.IC2IF = 0; // Reset respective interrupt flag
}

int Get_CP_ChargeRate(void)
{
    unsigned int DutyCycle;
    int ChargeRateAmps = 0;
    
    //printf("Frequency %.1fHz \r\n", 1 / (period * 0.000000025));
    //printf("Duty Cycle = %.1f%% ", (t_on * 100.0) / period);
    
    if (active) {
        active = false;
        // Check period 
        // IEC 61851 requires 1kHz signal to be within +/- 0.5%
        // 995Hz = 40201 25nS counts, 1005Hz = 39801 25nS counts
        if ((period >= 39801) & (period <= 40201)) {
            // CP Frequency is acceptable, check duty cycle
            // DutyCycle is in tenths of percent.
            DutyCycle = (t_on / 40);
            //printf("Duty Cycle %u\r\n", DutyCycle);

            if (DutyCycle < 30) {
                // Charging not allowed
                ChargeRateAmps = CP_ERROR;
            } else if (DutyCycle < 70) {
                // Digital Comms
                ChargeRateAmps = CP_REQ_DIGITAL_MODE;
            } else if (DutyCycle < 80) {
                // Charging not allowed
                ChargeRateAmps = CP_ERROR;
            } else if (DutyCycle < 100) {
                ChargeRateAmps = 600;
            } else if (DutyCycle < 850) {
                // Available current = (% duty cycle) x 0.6 A
                ChargeRateAmps = DutyCycle * 6;
            } else if (DutyCycle < 960) {
                // Available current = (% duty cycle - 64) x 2.5 A
                ChargeRateAmps = (DutyCycle - 640) * 25;
            } else if (DutyCycle < 970) {
                ChargeRateAmps = 8000;
            } else {
                // Over 97%, Charging not allowed
                ChargeRateAmps = CP_ERROR;
            }
        } else {
            //printf("Warning: Control Pilot frequency out of spec\r\n");
            ChargeRateAmps = CP_ERROR;
        }
    }
    return (ChargeRateAmps);
}
