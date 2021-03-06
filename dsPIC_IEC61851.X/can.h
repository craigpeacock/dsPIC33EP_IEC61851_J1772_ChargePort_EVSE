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

#ifndef XC_HEADER_CAN_H
#define	XC_HEADER_CAN_H

typedef enum
{
    CAN_MSG_TX_DATA_FRAME = 0,
    CAN_MSG_TX_REMOTE_FRAME
} CAN_MSG_TX_ATTRIBUTE;

#define NUM_OF_CAN_BUFFERS 32

#define TX_MSG_HIGH_PRIORITY                0b11
#define TX_MSG_INTERMEDIATE_HIGH_PRIORITY   0b10
#define TX_MSG_INTERMEDIATE_LOW_PRIORITY    0b01
#define TX_MSG_LOW_PRIORITY                 0b00

extern unsigned int ecan1_msgbuf[NUM_OF_CAN_BUFFERS][8] __attribute__((aligned(NUM_OF_CAN_BUFFERS * 16)));

void Init_CAN1(void);
void Init_SWCAN2(void);
void CAN_Print_Frame(uint16_t frame[]);
void CAN1_MessageTransmit(uint32_t messageID, uint8_t DLC, uint8_t* message, uint8_t fifoNum, CAN_MSG_TX_ATTRIBUTE msgAttr);
void CAN2_MessageTransmit(uint32_t messageID, uint8_t DLC, uint8_t* message, uint8_t fifoNum, CAN_MSG_TX_ATTRIBUTE msgAttr);

#endif