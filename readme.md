# dsPIC33EP IEC61851/SAE J1772 Demo Code

Source code intended for IEC61851/J1772 ChargePort Controller:

https://circuitmaker.com/Projects/Details/Craig-Peacock-4/J1772-Interface

https://circuitmaker.com/Projects/Details/Craig-Peacock-4/IEC61851-J1772-EVSE-Interface


## Example output connected to Tesla UMC.  

```
dsPIC33EP128GS804 IEC61851/SAE J1772 Demo Code
00000.14 PP: 3.30V (ADC_4095) Port unplugged
00002.57 PP: 1.90V (ADC_2363) 32A detachable cable detected
00002.80 CP: No signal present
00002.92 CP: Duty cycle = 4.8% @ 1000.1Hz
00002.95 Requesting digital communication
00005.98 CP: No signal present
00008.90 CP: Duty cycle = 13.1% @ 1000.1Hz
00008.93 Locking Charge Port
00009.14 Maximum Charge Rate 7.86A
00009.26 CP: Duty cycle = 13.1% @ 1000.8Hz
00009.29 Locking Charge Port
00009.51 Maximum Charge Rate 7.80A
00052.89 CP: No signal present
00053.61 PP: 3.30V (ADC_4095) Port unplugged
```

Connected using SWCAN Mode:
```
dsPIC33EP128GS804 IEC61851/SAE J1772 Demo Code
00000.14 PP: 3.30V (ADC_4095) Port unplugged
00005.11 PP: 1.90V (ADC_2361) 32A detachable cable detected
00005.30 CP: No signal present
00005.43 CP: Duty cycle = 4.8% @ 1000.1Hz
00005.46 Requesting digital communication
00005.58 Enabling SWCAN Interface
00005.60 0x505 [8] 0B 86 00 00 00 00 7D 08
00005.62 0x32C [8] 00 01 BF DC C0 C0 1C 05
00005.62 0x47C [8] 00 00 00 00 00 00 00 00
00005.62 0x505 [8] 1F EC 7F B8 08 42 08 00
00005.65 0x31C [8] 10 05 F6 78 FE FF 07 00
00005.65 0x505 [8] 00 03 CB 2D 2D 09 04 00
00005.72 0x31C [8] 10 05 F7 78 FE FF 07 00
00005.72 0x505 [8] 0B 86 00 00 00 00 7D 08
00005.79 0x31C [8] 10 05 F6 78 FE FF 07 00
00005.79 0x505 [8] 1F EC 7F B8 08 42 08 00
00005.86 0x31C [8] 10 05 F7 78 FE FF 07 00
00005.86 0x505 [8] 20 A4 11 F7 6C 04 00 00
00005.93 0x31C [8] 10 05 F7 78 FE FF 07 00
00005.93 0x505 [8] 00 03 CB 2D 2D 09 04 00
00006.00 0x31C [8] 10 05 F7 78 FE FF 07 00
00006.00 0x47C [8] 00 00 00 00 00 00 00 00
00006.00 0x505 [8] 0B 86 00 00 00 00 7D 08
00006.05 0x32C [8] 00 01 BF DC C0 C0 1C 05
00006.05 0x505 [8] 1F EC 7F B8 08 42 08 00
00006.10 0x31C [8] 10 05 F7 78 FE FF 07 00
00006.10 0x505 [8] 20 A4 11 F7 6C 04 00 00
00006.17 0x31C [8] 10 05 F6 78 FE FF 07 00
00006.17 0x505 [8] 00 03 CB 2D 2D 09 04 00
00008.13 PP: 3.30V (ADC_4095) Port unplugged
```
