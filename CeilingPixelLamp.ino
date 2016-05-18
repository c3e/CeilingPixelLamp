#include <can.h>

#include <Wire.h>
#include <WS2812.h>

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Code in here will only be compiled if an Arduino Uno (or older) is used.
#define LEDRGBPIN 4 // WS2812b, digital
#define LEDWHITEPIN 5 // white led, PWM
#define ECHOPIN 7 // Echo pin, digital
#define TRIGPIN 8 // Trigger pin, digital
#define I2CSDAPIN 18 // I2C SDA, Analog in
#define I2CSCLPIN 19 // I2C SCL, Analog in
#endif

#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
//Code in here will only be compiled if an Arduino Leonardo is used.
#define LEDRGBPIN 4 // WS2812b, Analog In
#define LEDWHITEPIN 5 // white led, PWM
#define ECHOPIN 7 // Echo pin, digital
#define TRIGPIN 8 // Trigger pin, Analog In
#define I2CSDAPIN 2 // I2C SDA, Analog in
#define I2CSCLPIN 3 // I2C SCL, PWM
#endif

#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__)
//Code in here will only be compiled if an Arduino Tiny45/85 is used.
#define LEDRGBPIN 3 // WS2812b, , PWM (LED on Model A)
#define LEDWHITEPIN 5 // white led, PWM, Analog (also used for USB- when USB is in use)
#define ECHOPIN 1 // ultrasonic echo, Analog In (also used for USB+ when USB is in use)
#define TRIGPIN 4 // ultrasonic trigger, Analog in
#define I2CSDAPIN 0 // I2C SDA, PWM (LED on Model B)
#define I2CSCLPIN 2 // I2C SCL, Analog in
#endif

// LED related variables
const uint8_t LEDCount = 16; // Number of LEDs to drive (default: 9)
WS2812 LED(LEDCount); // init the WS2812 LED libary with X LED's
cRGB ledValue; // holds the RGB color values
const uint8_t saturation = 255; 	// color saturation
uint8_t brightness = 0; 			// start / current brightness
const uint8_t brightnessMin = 0;	// lowest limit of led brightness
const uint8_t brightnessMax = 32;	// highest limit of led brightness
uint8_t rgbColor[3] = {0,0,0}; 		// inital RGB values buffer array
const uint16_t hueRange = 768;		// hue color range (3x255 from RGB)
uint16_t hue = 0;					// current Hue color

// auto cycle color / rainbow wheel
const uint8_t autoCycle = 0; // color cycle auto switch
uint8_t autoCycleDirection = 1; // current direction of auto color cycle

// ultra sonic sensor
const uint8_t minimumRange = 60; // Minimum range needed in cm
const uint8_t maximumRange = 120; // Maximum range needed in cm
uint32_t duration = 0; // ultra sonic sensor echo duration
uint32_t distance = 0; // ultra sonic sensor distance
//Metro sonicMetro = Metro(50); // refresh time in ms for sensor
uint32_t sonicPrevMillis = 0; // previous ultrasonic sensor polling millis
const uint32_t sonicInterval = 100; // ultrasonic sensor polling interval

// program logic related variables
uint8_t getNewColor = 0; // trigger flag for new color generation based on distance
uint8_t FadingIn = 0; // trigger flag for automatic fade in
uint8_t FadingOut = 0; // trigger flag for automatic fade out
uint32_t fadePrevMillis = 0; // previous fade millis
const uint32_t fadeInterval = 20; // fade in and out refresh interval (ms till next increase)
uint32_t sonicLastPrevMillis = 0; // previous ultrasonic sensor inactive millis
const uint32_t sonicLastInterval = 1000; // ultrasonic sensor inactive interval


void refreshRawDistance(); // get raw distance from ultrasonic sensor
void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3]); // convert HSB to RGB color

/*
Autocious Management
MODE 0: distance sensor
MODE 1: 12 cycles a slave ...ahem get led colors from i2c master

*/
uint8_t MODE = 0; //sets Mode to do nothing 
//
//
void setup() {
	// supersonic sensor pin setup
	pinMode(ECHOPIN, INPUT); // set ultrasonic sensor echo pin to input
	pinMode(TRIGPIN, OUTPUT); // set ultrasonic sensor trigger pin to output

	// LED pin and color order setup
	//LED.setOutput(LEDRGBPIN); // set WS2812B pin for library
	//LED.setColorOrderRGB();  // RGB color order
	//LED.setColorOrderBRG();  // BRG color order
	//LED.setColorOrderGRB();  // GRB color order (Default; will be used if none other is defined.)

	if(autoCycle == 1)
		brightness = brightnessMax;
  Serial.begin(9600);
  Wire.begin(60);
  Wire.onReceive(recvData);
  Serial.print("Setup complete");
}

void log3( uint8_t a,uint8_t b,uint8_t c){
  Serial.print("[");
  Serial.print(a);
  Serial.print(",");
  Serial.print(b);
  Serial.print(",");
  Serial.print(c);
  Serial.print("]");

}

/*
 * PacketLayout:
 * 0 - 47 byte: 16 * 3 color values
 * 48 - 51 byte: color pwm
 * 52 byte: white pwm
 * 53 byte: 
 *  bit 1: 
 *    1: activate remote control
 *    0: deactivate and switch to autacious mode
 * 
 * 
*/
void recvData (int count){
  Serial.print("Got Values: ");
  for ( uint8_t i = 0; i < count ; i++ ){
    ledValue.r = Wire.read();
    ledValue.b = Wire.read();
    ledValue.g = Wire.read();
    log3( ledValue.r,ledValue.g,ledValue.b);
    Serial.print(", ");
    LED.set_crgb_at(i/3, ledValue); // Set values at LED found at index i
  }
  LED.sync();
  if ( count == 54 ) {
   uint8_t x = Wire.read();
   if ( x >> 7 == 1 ) {
    MODE = 0;
   } else {
    MODE = 1;
   }
  }
}

int ledDistance () {
  if(autoCycle == 1) { // cycle through hue color wheel?
    if (autoCycleDirection == 1) { // cycle forward ...
      ++hue; // increase hue by one
      if (hue == hueRange) { // reached the end limit of hue?
        autoCycleDirection = 0; // change direction to backward
      }
    }
    else { // ... or backwards ?
      --hue; // decrease hue by one
      if (hue == 0) { // reached start limit of hue ?
        autoCycleDirection = 1; // change direction to forward
      }
    }
  }
  else if(FadingIn == 1) { // still fading in?
    if (brightness < brightnessMax) { // current brightness still under max brightness ?
        if (millis() - fadePrevMillis > fadeInterval) { // is it time to fade ?
        fadePrevMillis = millis(); // save the last time the hue faded one step
        ++brightness; // increase brightness by one
      }
    }
    else if(brightness == brightnessMax) { // current brightness reached max brightness ?
      FadingIn = 0; // stop fading in
    }
  }
  else if(FadingOut == 1) { // still fading out?
    if (brightness > brightnessMin) { // current brightness still over min brightness?
      if (millis() - fadePrevMillis > fadeInterval) { // is it time to fade?
        fadePrevMillis = millis(); // save the last time the hue faded one step
        --brightness; // decrease brightness by one
      }
    }
    else if(brightness == brightnessMin) { // current brightness reached min brightness ?
      FadingOut = 0; // stop fading out
    }
  }
  else {
    // check if ultra-sonic-sensor refresh interval breached
    if (millis() - sonicPrevMillis > sonicInterval) {
      sonicPrevMillis = millis(); // save current ultrasonic sensor check time
      refreshRawDistance(); // get current distance from ultrasonic sensor
      // "inside of range" logic
      if (distance > minimumRange && distance < maximumRange) {
        sonicLastPrevMillis = millis(); // save millis for distance inside limit
        if(getNewColor == 1) {
          // map distance 
          hue = map(distance, minimumRange, maximumRange, 0, hueRange);
          getNewColor = 0; // queue new color request
          FadingIn = 1; // queue fade in effect
        }
      }
    } // end if (currentMillis - sonicPrevMillis > sonicInterval)
    // check last ultrasonic sensor event interval
    if (millis() - sonicLastPrevMillis > sonicLastInterval) {
      getNewColor = 1; // queue new color request
      FadingOut = 1; // queue fade out effect
    }
  }

  // convert hsb 2 rgb colors (hue, saturation, brightness, colorarray[r,g,b])
  hsb2rgb(hue, saturation, brightness, rgbColor);
  // push r,g,b values in LED buffer
  ledValue.b = rgbColor[0];
  ledValue.g = rgbColor[1];
  ledValue.r = rgbColor[2];
    
  for(uint8_t i=0; i < LEDCount; ++i) {
    LED.set_crgb_at(i, ledValue); // Set values at LED found at index i
  }
  LED.sync(); // Sends the data to the LED strip
}

void loop() {
	if ( MODE == 1 ) {
	  ledDistance();
	} else {
    delay (100);
	}
}

void refreshRawDistance(){
	/* The following TRIGPIN/ECHOPIN cycle is used to determine the
		distance of the nearest object by bouncing soundwaves off of it. */ 
	digitalWrite(TRIGPIN, LOW); 
	delayMicroseconds(2);
	digitalWrite(TRIGPIN, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIGPIN, LOW);
	duration = pulseIn(ECHOPIN, HIGH); // wait for echo wave and return the duration
	// Calculate the distance (in centimeter or inches) based on the speed of sound.
	//distance = duration / 74 / 2; // calculate inches
	distance = duration / 29 / 2; // calculate centimeter
}

void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3]) {
    uint8_t temp[5], n = (index >> 8) % 3;
	// %3 not needed if input is constrained, but may be useful for color cycling and/or if modulo constant is fast
    uint8_t x = ((((index & 255) * sat) >> 8) * bright) >> 8;
	// shifts may be added for added speed and precision at the end if fast 32 bit calculation is available
    uint8_t s = ((256 - sat) * bright) >> 8;
    temp[0] = temp[3] = s;
    temp[1] = temp[4] = x + s;
    temp[2] = bright - x;
    color[0] = temp[n + 2];
    color[1] = temp[n + 1];
    color[2] = temp[n];
}
