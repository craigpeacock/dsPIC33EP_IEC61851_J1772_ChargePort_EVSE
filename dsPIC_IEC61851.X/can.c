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
    __builtin_write_OSCCONL(OSCCON & ~(1<<6));  // Unlock Peripheral Pin Select Registers
    RPINR26bits.C1RXR = 66;                     // Input: Assign CAN1 RX to RP66/RD2
    RPOR1bits.RP67R = 14;                       // Output: Assign CAN1 TX to RP67/RD3
    __builtin_write_OSCCONL(OSCCON | (1<<6));   // Lock Peripheral Pin Select Registers
    
    // ECAN module must be configuration mode to set baud rate registers
    C1CTRL1bits.REQOP = 4;                      // Request configuration mode
    while( C1CTRL1bits.OPMODE != 4 );           // Check operation mode 
       
    // Set ECAN module for 500kbps speed with 10 Tq per bit
    C1CFG1 = 0x47;                              // BRP = 8 SJW = 2 Tq
    C1CFG2 = 0x2D2;
    
    // Transmit
    // Set up DMA
    DMA0CONbits.SIZE = 0x0;             // Data transfer size: word
    DMA0CONbits.DIR = 0x1;              // Direction: Device RAM to peripheral
    DMA0CONbits.AMODE = 0x2;            // Peripheral indirect addressing mode
    DMA0CONbits.MODE = 0x0;             // Continuous, ping-pong disabled
    DMA0REQ = 0b01000110;               // ECAN1 - TX Data Request
    DMA0CNT = 7;                        // 8 words per DMA transfer
    DMA0PAD = (volatile unsigned int)&C1TXD;    // Peripheral register
    DMA0STAL = (unsigned int) &ecan1_msgbuf;    // Device RAM address 
    DMA0STAH = 0;
    DMA0CONbits.CHEN = 0x1;             // Enable
    
    C1TR01CONbits.TXEN0 = 0x1;          // Buffer 0 is a transmit buffer
    C1TR01CONbits.TX0PRI = 0x3;         // Buffer 0 has highest message priority
        
    C1FCTRLbits.FSA = 0b01000;          // FIFO starts at message buffer 8
    C1FCTRLbits.DMABS = 0b110;          // 32 buffers in device RAM

    C1CTRL1bits.WIN = 0;                // Window Select Bit
    C1RXFUL1 = C1RXFUL2 = 0x0000;       // Clear all receive buffer full status bits
    C1RXOVF1 = C1RXOVF2 = 0x0000;       // Clear all receive buffer overflow status bits
    
    // Receive
    // Set up DMA    
    DMA1CONbits.SIZE = 0x0;             // Data transfer size: word
    DMA1CONbits.DIR = 0x0;              // Direction: Peripheral to Device RAM
    DMA1CONbits.AMODE = 0x2;            // Peripheral indirect addressing mode
    DMA1CONbits.MODE = 0x0;             // Continuous, ping-pong disabled
    DMA1REQ = 34;                       // ECAN1 - RX 
    DMA1CNT = 7;                        // 8 words per DMA transfer    
    DMA1PAD = (volatile unsigned int)&C1RXD;    // Peripheral register
    DMA1STAL = (unsigned int) &ecan1_msgbuf;    // Device RAM address 
    DMA1STAH = 0;
    DMA1CONbits.CHEN = 0x1;             // Enable
    
    C1CTRL1bits.WIN = 1;                // Window Select Bit
    // dsPIC has 16 user-defined acceptance filters. We configure filter 0:
    C1FMSKSEL1bits.F0MSK = 0;           // Use Acceptance Mask 0 for Filter 0
    C1RXM0SIDbits.SID = 0x000;          // Setup Acceptance Mask 0 - Accept all
    C1RXM0SIDbits.MIDE = 0x1;           // Match standard/extended frames corresponding to EXIDE      
    
    C1RXF0SIDbits.SID = 0x000;          // Filter 
    C1RXF0SIDbits.EXIDE = 0x0;          // Match only standard identifiers

    C1BUFPNT1bits.F0BP = 8;             // CAN Acceptance filter 0 to use buffer 8
    C1FEN1bits.FLTEN0 = 1;              // Enable CAN acceptance filter 0
    
    C1CTRL1bits.WIN = 0;                // Window Select Bit
    C1CTRL1bits.REQOP = 0;              // Request normal mode
    while( C1CTRL1bits.OPMODE != 0 );   // Check operation mode
    
    // Set up interrupts
    IEC2bits.C1IE = 1;                  // Enable ECAN1 Interrupt
    C1INTEbits.TBIE = 1;                // Enable TX Buffer Interrupt
    C1INTEbits.RBIE = 1;                // Enable RX Buffer Interrupt
}

void CAN1_MessageTransmit(uint32_t messageID, uint8_t DLC, uint8_t* message, uint8_t fifoNum, CAN_MSG_TX_ATTRIBUTE msgAttr)
{
    // Populate header 
    ecan1_msgbuf[0][0] = (messageID & 0x7FF) << 2;  // SID[10:0] + SRR + IDE
    ecan1_msgbuf[0][1] = 0x0000;                    // EID[17:6]
    ecan1_msgbuf[0][2] = (DLC & 0xF);  
    //if (msgAttr == CAN_MSG_TX_REMOTE_FRAME)

    // Populate payload
    memcpy(&ecan1_msgbuf[0][3], message, DLC);
    
    // Initiate transmission
    C1TR01CONbits.TXREQ0 = 0x1;
    while(C1TR01CONbits.TXREQ0 == 1);
}

void __attribute__ ( (interrupt, no_auto_psv) ) _C1Interrupt( void )
{
    IFS2bits.C1IF = 0;      // clear interrupt flag

    if( C1INTFbits.TBIF )
    {
        C1INTFbits.TBIF = 0;
    }

    if( C1INTFbits.RBIF )
    {
        C1INTFbits.RBIF = 0;
    }
}