# Makita OBI Arduino Code

Standalone Arduino-based Makita LXT battery diagnostic and study tool using an **ATmega328P**, **ST7567 LCD**, push buttons, and a one-wire style communication interface.

This project was created for learning, diagnostics, and battery communication study. It is not intended as a commercial repair product.

## Project Status

Current working version:

```text
Makita_OBI_ST7567_Reset_Working_v14
```

Main features are working:

```text
Read battery model
Read pack voltage
Read individual cell voltages
Read cell voltage difference
Read battery temperature
Read lock/unlock status
Read capacity and battery type
LED test command
Guarded clear-errors/reset command
ST7567 LCD menu system
3-button user interface
```

## Hardware Used

```text
MCU: ATmega328P small board / Arduino-compatible board
Display: ST7567 132x32 LCD
Battery: Makita LXT 18V battery
Communication: Makita battery one-wire style data line
Buttons: UP, DOWN, OK
```

## Button Controls

```text
UP button    = Previous page
DOWN button  = Next page
OK button    = Refresh current page
```

Special pages:

```text
Reset Menu:
First long OK press  = Safety check
Second long OK press = Guarded clear-errors/reset attempt

LED Test:
Short OK press = LED ON command
Long OK press  = LED OFF command
```

## LCD Pages

```text
Page 0: Summary
Page 1: Battery model
Page 2: Cell voltages
Page 3: Temperature
Page 4: Status
Page 5: Reset menu
Page 6: About page
Page 7: LED test
```

## Working Reset / Clear Errors Sequence

The working sequence was found by comparing the standalone Arduino code with OBI.exe debug output.

```text
33 AA 00        Read status
CC DC 0C        Read model
33 D9 96 A5     Enter test mode
33 DA 04        Clear errors
```

The important ACK byte is:

```cpp
rsp[8]
```

Expected ACK:

```text
0x06
```

## Working LED ON Sequence

The LED ON command also uses the test mode sequence.

```text
33 D9 96 A5     Enter test mode
33 DA 31        LED ON
```

Expected ACK:

```text
0x06
```

## Safety Notes

This project is for learning and diagnostics only.

Do not use this project to return unsafe battery packs to normal service. Always check the cell voltages, cell balance, temperature, and physical battery condition before doing any test.

The firmware includes safety checks before attempting the guarded clear-errors/reset command:

```text
Battery must be locked
Cell voltage difference must be acceptable
Cell voltage must not be too low
Temperature must be within a safe range
Two long OK presses are required
```

## Important Notes

Makita battery communication is timing-sensitive. Small differences in delay timing can affect whether commands work.

The project currently uses Arduino-style functions such as:

```cpp
delay()
delayMicroseconds()
digitalWrite()
```

When porting to MPLAB X IDE or STM32CubeIDE, the most important part will be replacing the one-wire timing functions carefully.

## Future Plans

```text
Clean code structure further
Add clearer debug pages
Improve LED OFF testing
Port to MPLAB X IDE
Port to STM32CubeIDE
Create a proper PCB version
```

## Credits

This project was developed by studying Makita LXT battery communication behavior and comparing results with OBI / Open Battery Information style command sequences.

Repository by:

```text
Chaminda Hetti Arachchi
```
