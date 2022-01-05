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

struct CONTROL_PILOT control_pilot;

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
     * We also need to detect loss of pilot signal. The integrated IC timer has 
     * no ability to interrupt on overflow, so we set up Timer 3 to undertake 
     * this supervisory function. Timer 3 will overflow in 1.6mS. Each positive 
     * edge detected will reset Timer 3. If Timer 3 rolls over, this means there 
     * has been no pilot signal.
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

    control_pilot.has_changed = false;
    control_pilot.t_on = 0;
    control_pilot.period = 0;
    
    // Set-up Timer 3 to trigger every 1.6mS if not reset by CP positive edge 
    // 40MHz / 64 (prescaler) / 1000 (PR2) = 1.6mS
    T3CONbits.TCS = 0;              // Clock from peripheral clock (40MHz)
    T3CONbits.TCKPS = 0b10;         // 1:64 prescale
    PR3 = 1000;                     // Trig every 1000 clocks (1.6mS)
    T3CONbits.TON = 1;              // Turn timer on
    IFS0bits.T3IF = 0;              // Clear T3 interrupt flag
    IPC2bits.T3IP = 1;              // Set interrupt priority as 1
    IEC0bits.T3IE = 1;              // Enable T3 interrupt    

    
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
    TMR3 = 0;           // Reset Timer 3 
    IFS0bits.IC1IF = 0; // Reset respective interrupt flag
}

void __attribute__ ((__interrupt__, no_auto_psv)) _IC2Interrupt(void)
{
    // Occurs on falling edge, end of cycle
    // IC1BUF holds duration of positive pulse 
    // IC2BUF holds duration of negative pulse
    unsigned int t_on = IC1BUF;
    unsigned int period = IC1BUF + IC2BUF;
    
    // Check if the value has significantly changed since last sample
    if ((t_on < (control_pilot.t_on - 5)) | (t_on > (control_pilot.t_on + 5)) |
        (period < (control_pilot.period - 5)) | (period > (control_pilot.period + 5))) {
        control_pilot.t_on = IC1BUF;
        control_pilot.period = IC1BUF + IC2BUF;
        
        control_pilot.has_changed = true;
    }

    IFS0bits.IC2IF = 0; // Reset respective interrupt flag
}

void __attribute__ ((__interrupt__, no_auto_psv)) _T3Interrupt(void)
{
    // No Control Pilot Signal ...
    
    if (control_pilot.period) {
        control_pilot.t_on = 0;
        control_pilot.period = 0;
        control_pilot.has_changed = true;
    }
    
    IFS0bits.T3IF = 0; 
}

int Get_CP_ChargeRate(void)
{
    unsigned int DutyCycle;
    int ChargeRateAmps = 0;
    
    print_timestamp()
    if (control_pilot.period) {
        printf("CP: Duty cycle = %.1f%% ", ( control_pilot.t_on * 100.0) / control_pilot.period);
        printf("@ %.1fHz \r\n", 1 / ( control_pilot.period * 0.000000025));
    } else {
        printf("CP: No signal present\r\n");
    }
    
    // Check period 
    // IEC 61851 requires 1kHz signal to be within +/- 0.5%
    // 995Hz = 40201 25nS counts, 1005Hz = 39801 25nS counts
    if ((control_pilot.period >= 39801) & (control_pilot.period <= 40201)) {
        // CP Frequency is acceptable, check duty cycle
        // DutyCycle is in tenths of percent.
        DutyCycle = (control_pilot.t_on / 40);
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
    } else if (control_pilot.period == 0) {
        // Loss of CP, stop charging.
        ChargeRateAmps = 0;
    } else {
        printf("Warning: Control Pilot frequency out of spec\r\n");
        ChargeRateAmps = CP_ERROR;
    }   
    return (ChargeRateAmps);
}
