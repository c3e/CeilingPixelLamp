# CeilingPixelLamp
Arduino code based rgbw ceiling pixel lamp

## Description:
This project is like a disco floor, but on a ceiling, with handmade wooden frames, RGB LEDs and cotton fabric cover as a diffuser.
Currently it just supports the usage as RGB Matrix over a USB to Virtual Serial Port connection
and can be controlled with software like Glediator (java, cross platform) or Jinx (Windows only).

## Modes:

### USB 2 Serial:
In this mode, we use the native virtual serial port over USB, provided by the Teensy.
You "just" need to send a string over the serial port, 
starting with a sync character (1-3) and followed by the color values (0-255), 
based on the pixel order and the chosen sync character:
- sync char, color values
- RGB only:
  - 1,R,G,B,R,G,B,...
- Warm white only (not supported right now)
  - 2,W,W,W,W,...
- RGBWW (not supported right now)
  - 3,R,G,B,W,R,G,B,W,...

### Standalone:
In standalone mode, 
there will be some Templates to chose from, for idle Animations, fixed light presets and so on.

### MQTT Server:
still under development...

## Hardware:
- Raspberry Pi as Master
- Teensy 3.2
- SK6812 RGBWW Led strip
- 5x 5V 60A (300W) Powersupply

### Arduino + CAN Version:
```
We stopped the development of the Arduino based version, with CAN bus transceiver and WS2812 leds.
That been said, the code provided should still work.
```
- Hardware:
  - Arduino Nano v3.0 (ATmega328)
  - HC-SR04 Ultrasonic Distance Sensor
  - WS2812B LEDs
  - Warm white LEDs
  - MCP2515 CAN Bus Module Board TJA1050

## External Libraries:
- Teensy:
  - no external libs right now
- Arduino:
  - Light_WS2812 
    - https://github.com/cpldcpu/light_ws2812
  - New Ping
    - https://bitbucket.org/teckel12/arduino-new-ping/wiki/Home
  - CAN Bus
    - https://github.com/autowp/arduino-mcp2515
