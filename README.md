# CeilingPixelLamp
Arduino based ceiling pixel lamp

Description:
This Project is a Ceiling RGB Pixel Lamp.
Currently it just supports the standalone mode,
but there is a slave mode planned for operating as a RGB pixel matrix later on.

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
- Digispark (ATTiny85su20) - https://digistump.com/wiki/digispark
- HC-SR04 Ultrasonic Distance Sensor
- WS2812B LEDs
- Warm white LEDs
- P82B715: I2C-bus extender - http://www.nxp.com/products/interface-and-connectivity/interface-and-system-management/i2c/i2c-bus-repeaters-hubs-extenders/i2c-bus-extender:P82B715
- or PCA9600: Dual bidirectional bus buffer -  http://www.nxp.com/products/interface-and-connectivity/interface-and-system-management/i2c/i2c-voltage-level-translators/dual-bidirectional-bus-buffer:PCA9600

Because of the limited number of useable / available pins (and some other hardware limitations) on a ATTiny85,
we are testing right now the use of SPI or I2C for communication between the single nodes.

External Libraries:
- Light_WS2812 - https://github.com/cpldcpu/light_ws2812
- TinyWireM - https://github.com/adafruit/TinyWireM
