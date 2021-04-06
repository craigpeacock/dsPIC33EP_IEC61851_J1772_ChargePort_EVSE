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
#include <string.h>
#include "xc.h"
#include "board.h"
#include "can.h"

/* ECAN Routines
 * Supports up to 32 message buffers. Message buffers 0-7 can be configured for
 * either transmit or receive. 8-31 are for receive only.
 * All data is sent and received via DMA
 */

unsigned int ecan1_msgbuf[NUM_OF_CAN_BUFFERS][8] __attribute__((aligned(NUM_OF_CAN_BUFFERS * 16)));

void Init_CAN1(void)
{
    // Set up re-mappable I/O
    ANSELBbits.ANSB5 = 0;
    __builtin_write_OSCCONL(OSCCON & ~(1<<6));  // Unlock Peripheral Pin Select Registers
    RPINR26bits.C1RXR = 47;                     // Input: Assign CAN1 RX to RP47/RB15
    RPOR5bits.RP37R = 14;                       // Output: Assign CAN1 TX to RP37/RB5
    __builtin_write_OSCCONL(OSCCON | (1<<6));   // Lock Peripheral Pin Select Registers
    
    // ECAN module must be configuration mode to set baud rate registers
    C1CTRL1bits.REQOP = 4;              // Request configuration mode
    while( C1CTRL1bits.OPMODE != 4 );   // Check operation mode 
       
    // Baud Rate Prescaler bits:
    //C1CFG1bits.BRP = 0;               //   1M Baud TQ = 2x1x1/FCAN 
    C1CFG1bits.BRP = 1;                 // 500K Baud TQ = 2x2x1/FCAN
    //C1CFG1bits.BRP = 3;               // 250K Baud TQ = 2x4x1/FCAN
    //C1CFG1bits.BRP = 29;              //33.3K Baud TQ = 2x30x1/FCAN
    
    //TQ = Sync (1) + PRSEG (5) + SEG1PH (8) + SEG2PH (6) = 20  
    C1CFG2bits.SAM = 1;                 // Bus line is sampled 3 times at sample point
    C1CFG2bits.SEG2PHTS = 1;            // PS2 is freely programmable
    // Synchronization Jump Width bits:
    C1CFG1bits.SJW = 3;                 // Length is 4 x TQ
    // Propagation Time Segment bits:
    C1CFG2bits.PRSEG = 4;               // Length is 5 x TQ
    // Phase Segment 1 bits:
    C1CFG2bits.SEG1PH = 7;              // Length is 8 x TQ
    // Phase Segment 2 bits:
    C1CFG2bits.SEG2PH = 5;              // Length is 6 x TQ
    
    /* 
     * Setup Transmit:   
     * Data to be sent is written to RAM and DMA is used to copy to CAN 
     * peripheral. CAN message buffers 0 to 7 can be used for transmit.  
     * Each buffer can have one of four priority modes (set by TXnPRI bits).  
     * Once CAN message buffer is loaded, set TXREQn to initiate transfer.
     */
    DMA0CONbits.SIZE = 0x0;             // Data transfer size: word
    DMA0CONbits.DIR = 0x1;              // Direction: Device RAM to peripheral
    DMA0CONbits.AMODE = 0x2;            // Peripheral indirect addressing mode
    DMA0CONbits.MODE = 0x0;             // Continuous, ping-pong disabled
    DMA0REQ = 0x46;                     // ECAN1 - TX Data Request
    DMA0CNT = 7;                        // 8 words per DMA transfer
    DMA0PAD = (volatile unsigned int)&C1TXD;    // Peripheral register
    DMA0STAL = (unsigned int) &ecan1_msgbuf;    // Device RAM address 
    DMA0STAH = 0;
    DMA0CONbits.CHEN = 1;               // Enable
    
    C1CTRL1bits.WIN = 0;                // Window Select Bit
    C1TR01CONbits.TXEN0 = 1;            // Buffer 0 is a transmit buffer
    C1TR01CONbits.TX0PRI = TX_MSG_HIGH_PRIORITY; // Buffer 0 has highest message priority
        
    C1TR01CONbits.TXEN1 = 1;            // Buffer 1 is a transmit buffer
    C1TR01CONbits.TX1PRI = TX_MSG_LOW_PRIORITY; // Buffer 1 has highest message priority

    /* 
     * Setup Receive: 
     * DMA is used to copy data from CAN peripheral to RAM (message buffers 8 
     * to 31).
     * A FIFO can be configured to set up a circular buffer and/or acceptance 
     * filters can be used to file messages into different slots/buffers.
     */
    DMA1CONbits.SIZE = 0x0;             // Data transfer size: word
    DMA1CONbits.DIR = 0x0;              // Direction: Peripheral to Device RAM
    DMA1CONbits.AMODE = 0x2;            // Peripheral indirect addressing mode
    DMA1CONbits.MODE = 0x0;             // Continuous, ping-pong disabled
    DMA1REQ = 0x22;                     // ECAN1 - RX 
    DMA1CNT = 7;                        // 8 words per DMA transfer    
    DMA1PAD = (volatile unsigned int)&C1RXD;    // Peripheral register
    DMA1STAL = (unsigned int) &ecan1_msgbuf;    // Device RAM address 
    DMA1STAH = 0;
    IFS0bits.DMA1IF = 0;                // Clear the DMA1 Interrupt Flag
    IEC0bits.DMA1IE = 1;                // Enable DMA1 Interrupt
    DMA1CONbits.CHEN = 0x1;             // Enable
    
    C1CTRL1bits.WIN = 0;                // Window Select Bit
    C1RXFUL1 = C1RXFUL2 = 0x0000;       // Clear all receive buffer full status bits
    C1RXOVF1 = C1RXOVF2 = 0x0000;       // Clear all receive buffer overflow status bits

    /* 
     * Set-up Acceptance Filters:
     * In this example we set up three filters with a common mask.
     * Frames with ID 0x3XX are copied to CAN message buffer 8. 
     * Frames with ID 0x4XX are copied to CAN message buffer 9. 
     * Frames with ID 0x5XX are copied to CAN message buffer 10. 
     */
      
    C1CTRL1bits.WIN = 1;                // Acceptance filters and masks
                                        // are in window 1
        
    // dsPIC has 3 masks. We configure mask 0: (0 = Ignore bit, 1 = Match)
    C1RXM0SIDbits.SID = 0x700;          // Setup Acceptance Mask 0 
    C1RXM0SIDbits.MIDE = 1;             // Match standard/extended frames corresponding to EXIDE   
    
    // dsPIC has 16 user-defined acceptance filters. 
    // We configure filter 0:
    C1FMSKSEL1bits.F0MSK = 0;           // Use Acceptance Mask 0
    C1RXF0SIDbits.SID = 0x300;          // Filter 
    C1RXF0SIDbits.EXIDE = 0;            // Match only standard identifiers
    C1BUFPNT1bits.F0BP = 8;             // CAN Acceptance filter 0 to use buffer 8
    C1FEN1bits.FLTEN0 = 1;              // Enable CAN acceptance filter 0
    
    // We don't use FIFO
    //C1BUFPNT1bits.F0BP = 0b1111;      // Use RX FIFO   
    //C1FCTRLbits.FSA = 8;              // FIFO starts at message buffer 8
    //C1FCTRLbits.DMABS = 0b110;        // 32 buffers in device RAM
      
    // We configure filter 1:
    C1FMSKSEL1bits.F1MSK = 0;           // Use Acceptance Mask 0
    C1RXF1SIDbits.SID = 0x400;          // Filter
    C1RXF1SIDbits.EXIDE = 0;            // Match only standard identifiers
    C1BUFPNT1bits.F1BP = 9;             // Use Message Buffer 9
    C1FEN1bits.FLTEN1 = 1;              // Enable CAN acceptance filter 1
    
    // We configure filter 2:
    C1FMSKSEL1bits.F2MSK = 0;           // Use Acceptance Mask 0
    C1RXF2SIDbits.SID = 0x500;          // Filter
    C1RXF2SIDbits.EXIDE = 0;            // Match only standard identifiers
    C1BUFPNT1bits.F2BP = 10;            // Use Message Buffer 10
    C1FEN1bits.FLTEN2 = 1;              // Enable CAN acceptance filter 2
    
    C1CTRL1bits.WIN = 0;                // Window Select Bit
    C1CTRL1bits.REQOP = 0;              // Request normal mode
    while( C1CTRL1bits.OPMODE != 0 );   // Check operation mode
    
    // Set up interrupts
    //IEC2bits.C1IE = 1;                  // Enable ECAN1 Interrupt
    //C1INTEbits.TBIE = 1;                // Enable TX Buffer Interrupt
    //C1INTEbits.RBIE = 1;                // Enable RX Buffer Interrupt
}

void __attribute__((__interrupt__,no_auto_psv)) _DMA1Interrupt(void)
{
    /* 
     * DMA1 (Receive DMA) Interrupt occurs after CAN frame is copied to CAN
     * message buffer.
     * 
     * This example handles CAN message buffers 8 (0x3XX),9 (0x4XX) & 10 
     * (0x5XX) identifiers.
     */
    if (C1RXFUL1bits.RXFUL8) {
        CAN_Print_Frame(&ecan1_msgbuf[8][0]);        
        
        uint16_t SID = (ecan1_msgbuf[8][0] >> 2) & 0x7FF;
        switch (SID) {
            
            case 0x302:
                CAN1_MessageTransmit(SID+1, 5, (uint8_t *)"Hello", 0, CAN_MSG_TX_DATA_FRAME);
                break;
            
            default:
                break;
        }

        C1RXFUL1bits.RXFUL8 = 0;
    }
    
    if (C1RXFUL1bits.RXFUL9) {
        CAN_Print_Frame(&ecan1_msgbuf[9][0]);
        C1RXFUL1bits.RXFUL9 = 0;
    }

    if (C1RXFUL1bits.RXFUL10) {
        CAN_Print_Frame(&ecan1_msgbuf[10][0]);
        C1RXFUL1bits.RXFUL10 = 0;
    }
    
    IFS0bits.DMA1IF = 0;                // Clear the DMA1 Interrupt Flag
}

void CAN_Print_Frame(uint16_t frame[])
{
    uint8_t i;
    
    uint16_t SID = (frame[0] >> 2) & 0x7FF;
    uint8_t DLC = frame[2] & 0xF;
    uint8_t *data = (uint8_t *)&frame[3];
          
    printf("0x%03X [%d] ", SID, DLC);

    if (frame[0] & 0x02) {
        printf("remote request");
    } else {
        for (i = 0; i < DLC; i++)
            printf("%02X ",data[i]);
    }
    
    printf("\r\n");
}

void CAN1_MessageTransmit(uint32_t messageID, uint8_t DLC, uint8_t* message, uint8_t fifoNum, CAN_MSG_TX_ATTRIBUTE msgAttr)
{
    // Populate header of message buffer 0 (msgbuf[0]):
    ecan1_msgbuf[0][0] = (messageID & 0x7FF) << 2;  // SID[10:0] + SRR + IDE
    ecan1_msgbuf[0][1] = 0x0000;                    // EID[17:6]
    ecan1_msgbuf[0][2] = (DLC & 0xF);  
    //if (msgAttr == CAN_MSG_TX_REMOTE_FRAME)

    // Populate payload
    memcpy(&ecan1_msgbuf[0][3], message, DLC);
    
    // Initiate transmission of message buffer 0:
    C1TR01CONbits.TXREQ0 = 0x1;
    while(C1TR01CONbits.TXREQ0 == 1);
}

void __attribute__ ( (interrupt, no_auto_psv) ) _C1Interrupt( void )
{
    if(C1INTFbits.TBIF)
    {
        C1INTFbits.TBIF = 0;
    }

    if(C1INTFbits.RBIF)
    {
        C1INTFbits.RBIF = 0;
    }
    IFS2bits.C1IF = 0;      // Clear interrupt flag
}