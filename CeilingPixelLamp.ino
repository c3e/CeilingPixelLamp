#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <SPI.h>
//#include <can.h>
#include <Wire.h>
#include <WS2812.h>
#include <NewPing.h>


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
const uint8_t brightnessMax = 255;	// highest limit of led brightness
uint8_t rgbColor[3] = {0,0,0}; 		// inital RGB values buffer array
const uint16_t hueRange = 765;		// hue color range (3x255 from RGB)
uint16_t hue = 0;					// current Hue color

// auto cycle color / rainbow wheel
const uint8_t autoCycle = 0; // color cycle auto switch
uint8_t autoCycleDirection = 1; // current direction of auto color cycle

// white led stuff
const uint8_t autoWhite = 1;
uint8_t WhiteFadingIn = 0;
uint8_t WhiteFadingOut = 1;
uint32_t WhitefadePrevMillis = 0; // previous fade millis
const uint32_t WhiteFadeInterval = 5; // fade in and out refresh interval (ms till next increase)
uint8_t Whitebrightness = 0;
uint8_t WhiteBrightnessMin = 0;
uint8_t WhiteBrightnessMax = 255;

// ultra sonic sensor
const uint8_t minimumRange = 60; // Minimum range needed in cm
const uint8_t maximumRange = 120; // Maximum range needed in cm
//const uint8_t minimumRange = 10; // Minimum range needed in cm
//const uint8_t maximumRange = 80; // Maximum range needed in cm

uint32_t distance = 0; // ultra sonic sensor distance
//Metro sonicMetro = Metro(50); // refresh time in ms for sensor
uint32_t sonicPrevMillis = 0; // previous ultrasonic sensor polling millis
const uint32_t sonicInterval = 40; // ultrasonic sensor polling interval
uint32_t sonicLastPrevMillis = 0; // previous ultrasonic sensor inactive millis
const uint32_t sonicLastInterval = 500; // ultrasonic sensor inactive interval
NewPing sonar(ECHOPIN, ECHOPIN, maximumRange); // NewPing setup of pin and maximum distance.

// program logic related variables
uint8_t getNewColor = 0; // trigger flag for new color generation based on distance
uint8_t FadingIn = 0; // trigger flag for automatic fade in
uint8_t FadingOut = 0; // trigger flag for automatic fade out
uint32_t fadePrevMillis = 0; // previous fade millis
const uint32_t fadeInterval = 5; // fade in and out refresh interval (ms till next increase)



void refreshRawDistance(); // get raw distance from ultrasonic sensor
void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3]); // convert HSB to RGB color

/*
Autocious Management
MODE 0: distance sensor
MODE 1: 12 cycles a slave ...ahem get led colors from i2c master

*/
uint8_t MODE = 0; //sets Mode to do nothing
uint8_t ADDRESS = 1; 
//
//

MCP_CAN CAN0(10);


void setup() {
	// supersonic sensor pin setup
	//pinMode(ECHOPIN, INPUT); // set ultrasonic sensor echo pin to input
	//pinMode(TRIGPIN, OUTPUT); // set ultrasonic sensor trigger pin to output

	// LED pin and color order setup
	//LED.setOutput(LEDRGBPIN); // set WS2812B pin for library
	//LED.setColorOrderRGB();  // RGB color order
	//LED.setColorOrderBRG();  // BRG color order
	//LED.setColorOrderGRB();  // GRB color order (Default; will be used if none other is defined.)

	if(autoCycle == 1) {
		//brightness = 0;
		brightness = brightnessMax;
<<<<<<< HEAD
  
  CAN0.begin(CAN_500KBPS);                       // init can bus : baudrate = 500k 
  pinMode(5, INPUT); 
  Serial.begin(9600);
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
=======
	}
}

void loop() {
	if(autoWhite == 1){
		if(WhiteFadingIn == 1) { // still fading in?
			if (Whitebrightness < WhiteBrightnessMax) { // current Whitebrightness still under max Whitebrightness ?
	  			if (millis() - WhitefadePrevMillis > WhiteFadeInterval) { // is it time to fade ?
					WhitefadePrevMillis = millis(); // save the last time the hue faded one step
					++Whitebrightness; // increase Whitebrightness by one
				}
			}
			else if(Whitebrightness == WhiteBrightnessMax) { // current Whitebrightness reached max Whitebrightness ?
				WhiteFadingIn = 0; // stop fading in
			}
		}
		else if(WhiteFadingOut == 1) { // still fading out?
			if (Whitebrightness > WhiteBrightnessMin) { // current Whitebrightness still over min Whitebrightness?
				if (millis() - WhitefadePrevMillis > WhiteFadeInterval) { // is it time to fade?
					WhitefadePrevMillis = millis(); // save the last time the hue faded one step
					--Whitebrightness; // decrease Whitebrightness by one
				}
			}
			else if(Whitebrightness == WhiteBrightnessMin) { // current Whitebrightness reached min Whitebrightness ?
				WhiteFadingOut = 0; // stop fading out
			}
		}
		if(WhiteFadingIn == 1 || WhiteFadingOut == 1) {
			analogWrite(LEDWHITEPIN, Whitebrightness);
		}
		/*
		else {
			analogWrite(LEDWHITEPIN, 0);
		}
		*/
	}


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
		delay(20);
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
					WhiteFadingOut = 1;
				}
			}
		} // end if (currentMillis - sonicPrevMillis > sonicInterval)
		// check last ultrasonic sensor event interval
		if (millis() - sonicLastPrevMillis > sonicLastInterval) {
			getNewColor = 1; // queue new color request
			FadingOut = 1; // queue fade out effect
			WhiteFadingIn = 1;
		}
	}
>>>>>>> 914c4a4df7a4b3c03db2bfe284934364ac19a602

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

unsigned char len = 0;
unsigned char buf[8];
unsigned char barrierbuffer[48];


void recvData (){
  // Serial.print("Got Values: ");
  
  CAN0.readMsgBuf(&len, buf);
  uint8_t control_bits = buf[0] & 3;
  
  if ( buf[0] >> 2  == ADDRESS && len == 8 && control_bits != 3){
    
    switch (control_bits) {
      
      case 0: //sync two
        LED.set_crgb_at(buf[1] >> 4 , { buf[2],buf[3],buf[4] } );
        LED.set_crgb_at( ( buf[1] & 15 ) >> 4 , { buf[5],buf[6],buf[7] } );
        break;
    
      case 1: //wait for sync signal
        barrierbuffer[ ( buf[1] >> 4 ) * 3 ] = buf[2];
        barrierbuffer[ ( buf[1] >> 4 ) * 3 + 1 ] = buf[3];
        barrierbuffer[ ( buf[1] >> 4 ) * 3 + 2 ] = buf[4];
        barrierbuffer[ ( buf[1] & 15 ) * 3 ] = buf[5];
        barrierbuffer[ ( buf[1] & 15 ) * 3 + 1 ] = buf[6];
        barrierbuffer[ ( buf[1] & 15 ) * 3 + 2 ] = buf[7];
        break;
      
      case 2: //sync
        barrierbuffer[ ( buf[1] >> 4 ) * 3 ] = buf[2];
        barrierbuffer[ ( buf[1] >> 4 ) * 3 + 1 ] = buf[3];
        barrierbuffer[ ( buf[1] >> 4 ) * 3 + 2 ] = buf[4];
        barrierbuffer[ ( buf[1] & 15 ) * 3 ] = buf[5];
        barrierbuffer[ ( buf[1] & 15 ) * 3 + 1 ] = buf[6];
        barrierbuffer[ ( buf[1] & 15 ) * 3 + 2 ] = buf[7];
        for ( uint8_t i = 0; i < 16; i++ ){
          LED.set_crgb_at( i, { barrierbuffer[i*3], barrierbuffer[i*3+1], barrierbuffer[i*3+2] } );
        }
        memset(barrierbuffer, 0, 48);
        break;
    }
    LED.sync();
  }
  if ( control_bits == 3 ) {
    if (buf[0] & 4 == 4 ) {
       MODE = 1;
    } else if ( buf[0] & 8 == 8 ) {
       MODE = 0;
    } else
    
    if ( buf[1] >> 2  == ADDRESS){
      analogWrite(LEDWHITEPIN, buf[4]);
    } else if ( ( ( buf[1] & 252) << 4 ) + ( buf[2] >> 4 ) == ADDRESS ){
      analogWrite(LEDWHITEPIN, buf[5]);
    } else if ( ( ( buf[2] & 15) << 2 ) + ( ( buf[3] & 192 ) >> 6 ) == ADDRESS ){
      analogWrite(LEDWHITEPIN, buf[6]);
    } else if ( buf[3] & 127 == ADDRESS ){
      analogWrite(LEDWHITEPIN, buf[7]);
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
	}
  if (!digitalRead(5) && MODE == 0){
    recvData();
  }
}

void refreshRawDistance() {
	unsigned int uS = sonar.ping(); // Send ping, get ping time in microseconds (uS).
	distance = uS / US_ROUNDTRIP_CM; // Convert ping time to distance (0 = outside set distance range, no ping echo)
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
