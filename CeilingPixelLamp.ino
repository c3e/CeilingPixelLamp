#include <WS2812.h>
#include <Metro.h>

const uint8_t LEDPin = 4;  	// Digital output pin (default: 7)
const uint8_t LEDCount = 16;	// Number of LEDs to drive (default: 9)
WS2812 LED(LEDCount); 
cRGB ledValue;
const uint8_t saturation = 255; 	// color saturation
uint8_t brightness = 0; 			// start / current brightness
const uint8_t brightnessMin = 0;	// lowest limit of led brightness
const uint8_t brightnessMax = 64;	// highest limit of led brightness
uint8_t rgbColor[3] = {0,0,0}; 		// inital RGB values
const uint16_t hueRange = 768;		// cue color range (3x255 from RGB)
uint16_t hue = 0;					// current Hue color

const uint8_t autoCycle = 0; // color cycle auto switch
uint8_t autoCycleDirection = 1; // current direction of auto color cycle

const uint8_t echoPin = 7; // Echo Pin
const uint8_t trigPin = 8; // Trigger Pin
const uint8_t minimumRange = 0; // Minimum range needed in cm
const uint8_t maximumRange = 40; // Maximum range needed in cm
long duration = 0; // ultra sonic sensor echo duration
long distance = 0; // ultra sonic sensor distance
Metro sonicMetro = Metro(150); // Metro based refresh in milliseconds

uint8_t getNewColor = 0; // trigger flag for new color generation based on distance
uint8_t FadingIn = 0; // trigger flag for automatic fade in
uint8_t FadingOut = 0; // trigger flag for automatic fade out
Metro sonicLastChange = Metro(1000); // inactive timer
Metro fadeMetro = Metro(25); // fade in and out refresh metro timer

void refreshRawDistance(); // get raw distance from ultra sonic sensor
void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3]); // convert HSB to RGB color

void setup() {
	// supersonic sensor pin setup
	pinMode(trigPin, OUTPUT); // set ultrasonic sensor trigger pin to output
	pinMode(echoPin, INPUT); // set ultrasonic sensor echo pin to input

	// LED pin and color order setup
	LED.setOutput(LEDPin); // set WS2812B pin for library
	LED.setColorOrderRGB();  // RGB color order
	//LED.setColorOrderBRG();  // BRG color order
	//LED.setColorOrderGRB();  // GRB color order (Default; will be used if none other is defined.)
}

void loop() {
	if(FadingIn == 1) { // still fading in?
		if (brightness < brightnessMax) { // current brightness still under max brightness?
			if(fadeMetro.check()) { // is it time to fade?
				++brightness; // increase brightness by one
				fadeMetro.reset(); // reset fade wait timer for next round
			}
		}
		else if(brightness == brightnessMax) { // current brightness reached max brightness?
			FadingIn = 0; // stop fading in
		}
	}
	else if(FadingOut == 1) { // still fading out?
		if (brightness > brightnessMin) { // current brightness still over min brightness?
			if(fadeMetro.check()) { // is it time to fade?
				--brightness; // decrease brightness by one
				fadeMetro.reset(); // reset fade wait timer for next round
			}
		}
		else if(brightness == brightnessMin) { // current brightness reached min brightness?
			FadingOut = 0; // stop fading out
		}
	}
	else if(FadingOut == 1 && FadingIn == 1) { // something got wrong, better reset both fades
		FadingIn = 0; // stop fading in
		FadingOut = 0; // stop fading out
	}

	if(autoCycle == 1) {
		if (autoCycleDirection == 1) {
			++hue;
			if (hue == hueRange) 
				autoCycleDirection = 0; 
		}
		else {
			--hue;
			if (hue == 0)
				autoCycleDirection = 1;
		}
	}
	else {
		if(sonicMetro.check() ) { // check if ultra-sonic-sensor refresh timer breached
			refreshRawDistance(); // get current distance from ultra-sonic-sensor
			// "inside of range" logic
			if (distance > minimumRange && distance < maximumRange) {
				sonicLastChange.reset(); // reset last change timer to keep current color active

				if(getNewColor == 1) {
					hue = map(distance, minimumRange, maximumRange, 0, hueRange); //
					getNewColor = 0; // queue new color request
					FadingIn = 1; // queue fade in effect
				}
			}
			sonicMetro.reset(); // restart ultra-sonic-sensor request timer
		} // end if (sonicMetro.check())	

		// check if inactive timer ended
		if(sonicLastChange.check()) {
			getNewColor = 1; // queue new color request
			FadingOut = 1; // queue fade out effect
		}
	}

	// convert hsb 2 rgb colors (hue, saturation, brightness, colorarray[r,g,b])
	hsb2rgb(hue, saturation, brightness, rgbColor);
	// push r,g,b values in LED buffer
	for(uint8_t i=0; i < LEDCount; ++i) {
		ledValue.b = rgbColor[0];
		ledValue.g = rgbColor[1];
		ledValue.r = rgbColor[2];
		LED.set_crgb_at(i, ledValue); // Set values at LED found at index i
	}

	LED.sync(); // Sends the data to the LED strip
}

void refreshRawDistance(){
	/* The following trigPin/echoPin cycle is used to determine the
		distance of the nearest object by bouncing soundwaves off of it. */ 
	digitalWrite(trigPin, LOW); 
	delayMicroseconds(2);
	digitalWrite(trigPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(trigPin, LOW);
	duration = pulseIn(echoPin, HIGH);
	// Calculate the distance (in cm) based on the speed of sound.
	distance = duration / 58.2;
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
