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

//CAN Variables
#define CAN_RX_PIN 5
