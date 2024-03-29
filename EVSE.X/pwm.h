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

#ifndef XC_HEADER_PWM_H
#define	XC_HEADER_PWM_H

#define HIGH        -1
#define LOW         -2
#define DIGITAL     -3

enum dir { CLOCKWISE, COUNTERCLOCKWISE, BRAKEVCC, BRAKEGND };
void Init_PWM(void);
void CP_Set(int16_t amps);

#endif