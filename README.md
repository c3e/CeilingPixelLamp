# CeilingPixelLamp
Arduino based ceiling pixel lamp

Description:
This Project is a Ceiling RGB Pixel Lamp.
Currently it just supports the standalone mode,
but the slave mode is in development for operating as a RGB pixel matrix later on.

Standalone Mode:
In standalone mode, 
the attiny is constantly polling the ultra sonic sensor to measure the current distance in centimeters.
(You can set a min and max-range for the distance check, to limit the "active" range.)
If the current distance is inside the limits (a person enters),
the RGB leds start to "fadein" until the max brightness is reached and there color will change based on the current distance.
If the current distance is outside the limits (a person left),
the RGB leds start to "fadeout" until the min brightness is reached.

Slave Mode:
still in development...
But the basic idea is to drive multiple ceiling lamps over a bus connection,
to get a pixel matrix for displaying content. (games, ambilight, etc)

Hardware:
- Raspberry Pi as Master
- Arduino Nano v3.0 (ATmega328) as Slave
- HC-SR04 Ultrasonic Distance Sensor
- WS2812B LEDs
- Warm white LEDs
- MCP2515 CAN Bus Module Board TJA1050

External Libraries:
- Light_WS2812 - https://github.com/cpldcpu/light_ws2812
- New Ping - https://bitbucket.org/teckel12/arduino-new-ping/wiki/Home
- CAN Bus Shield
