https://wiki.wemos.cc/products:d1:d1_mini

# 2.3.0 #

## Technical specs ##

Microcontroller | ESP-8266EX
--- | ---
Operating Voltage | 3.3V
Digital I/O Pins | 11
Analog Input Pins | 1 (Max input: 3.2V)
Clock Speed | 80MHz/160MHz
Flash | 4M bytes
Length | 34.2mm
Width | 25.6mm
Weight | 10g

## Pinout

Pin | Function | ESP-8266 Pin |
--- | --- | --- |
TX | TXD | TXD |
RX | RXD | RXD |
A0 | Analog input, max 3.3V input | A0
D0 | IO | GPIO16
D1 | IO, SCL | GPIO5
D2 | IO, SDA | GPIO4
D3 | IO, 10k Pull-up | GPIO0
D4 | IO, 10k Pull-up, BUILTIN_LED | GPIO2
D5 | IO, SCK | GPIO14
D6 | IO, MISO | GPIO12
D7 | IO, MOSI | GPIO13
D8 | IO, 10k Pull-down, SS  |GPIO15
G | Ground  |GND
5V | 5V  |-
3V3 | 3.3V  |3.3V
RST | Reset | RST

All of the IO pins have interrupt/pwm/I2C/one-wire support except D0.
All of the IO pins run at 3.3V.

## Reset 

reset causes:
- 0: 
- 1: normal boot
- 2: reset pin
- 3: software reset
- 4: watchdog reset

boot device:
- 0:
- 1: ram
- 3: flash

## Boot Mode

N | G15/D8 | G0/D3 | G2/D4 | Boot Mode
--- | --- | --- | --- | ---
0 |   0    |   0   |   0   | INV
1 |   0    |   0   |  3.3  | UART
2 |   0    |  3.3  |   0   | INV
3 |   0    |  3.3  |  3.3  | Flash 
4 |  3.3   |   0   |   0   | SDIO
5 |  3.3   |   0   |  3.3  | SDIO
6 |  3.3   |  3.3  |   0   | SDIO
7 |  3.3   |  3.3  |  3.3  | SDIO
