#HTTP Api

## Pixel

Set the pixel at [x],[y] of panel [id] to color [<r\>,<g\>,<b\>].

>POST http://[host]:5555/api/set/pixel/[id]/[x]/[y]

>DATA "[<r\>,<g\>,<b\>]"

## Panel

Set the panel [id] with the array of pixel colors. Each given in an RGB encoded Number (eg. 38127321) (not implemented: or Hex-Number (eg. xDEADBEEF)).

For Example: the value x0055FF would mean, that the pixel will assume r:0x00, g:0x55, b:0xFF.

>POST http://[host]:5555/api/set/panel/[id]

>DATA [<rgb_0>,<rgb_1>,...,<rgb_15>]

## On/Off

Switching panels on and off works by invoking the following keys:

Switch panel [id] ON:

>http://[host]:5555/api/set/on/[id]

Switch panel [id] OFF:

>http://[host]:5555/api/set/off/[id]

## White

Turn on the white LEDs of panel [id] with the brightness value of [pwm]

>http://[host]:5555/api/set/white/[id]/[pwm]

## Orientation

When fitting the panel you usually don't really take care of the orientation,
so you have to change the orientation afterwards to make the pixels appear in the right order:

>http://[host]:5555/api/set/orient/[id]/[order]

[order] := 1,2,3,4

## Panel state

Querying the state of panel [id] works by invoking

>http://[host]:5555/api/get/[id]

You will receive an array of 16 hex values

## MQTT

Hardcoded MQTT Server at docker.chaospott.de:9001 (no ssl)

# Update

Updating a Panel via MQTT with following JSON struct:

```
items: [
	"x": 3,
	"y": 5,
	"pixels":[
		0: xABFF09,
		1: xFB341E,
		3: xA12211,
		...
	]
]
´´´

This updates the Pixel 0,1,3 with the respective values of a Panel at coordinates (3,5)

# Propagate

The same struct is prapagated back when updated.