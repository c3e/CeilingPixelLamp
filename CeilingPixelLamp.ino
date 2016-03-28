#include <WS2812.h>
#include <Metro.h>

const uint8_t LEDPin = 4;  	// Digital output pin (default: 7)
const uint8_t LEDCount = 4;	// Number of LEDs to drive (default: 9)
WS2812 LED(LEDCount); 
cRGB ledValue;
const uint8_t saturation = 255; 	// color saturation
uint8_t brightness = 0; 			// current brightness
const uint8_t brightnessMin = 0;	// lowest limit of led brightness
const uint8_t brightnessMax = 64;	// highest limit of led brightness
uint8_t rgbColor[3] = {0,0,0}; 		// Default RGB values
const uint16_t hueRange = 768;		// Hue range (3x255 from RGB)
uint16_t hue = 0;					// current Hue color

const uint8_t autoCycle = 0; // color cycle auto switch
uint8_t sign = 1; // current direction of auto color cycle

const uint8_t PROBENUMBER = 5; // number of distance probes (0-254) 
uint8_t probes[PROBENUMBER];
uint8_t currentProbe = 0;

const uint8_t echoPin = 7; // Echo Pin
const uint8_t trigPin = 8; // Trigger Pin
const uint8_t minimumRange = 0; // Minimum range needed in cm
const uint8_t maximumRange = 60; // Maximum range needed in cm
long duration = 0;
long distance = 0;
long distancesmooth = 0; // Duration used to calculate distance
Metro sonicMetro = Metro(50); // Metro based refresh in milliseconds

uint8_t noEvent = 1;
Metro sonicLastChange = Metro(500); // inactive timer
Metro fadeMetro = Metro(25); // fade in and out refresh metro timer

void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3]); // Instantiate hsb2rgb and it's variables  a.k.a  Hue to RGB

void setup() {
	// supersonic sensor pin setup
	pinMode(trigPin, OUTPUT);
	pinMode(echoPin, INPUT);

	// LED pin and color order setup
	LED.setOutput(LEDPin); // Digital Pin 4
	LED.setColorOrderRGB();  // Uncomment for RGB color order
	//LED.setColorOrderBRG();  // Uncomment for BRG color order
	//LED.setColorOrderGRB();  // Uncomment for GRB color order (Default; will be used if none other is defined.)

	// fill probes with zeros at startup
	for(uint8_t i=0; i< PROBENUMBER; ++i) {
	    probes[i] = 0;
	}
}

void loop() {
	if(autoCycle) {
		if (sign) {
			++hue;
			if (hue == hueRange) 
				sign = 0; 
		}
		else {
			--hue;
			if (hue == 0)
				sign = 1;
		}
	}
	else {
		if( sonicMetro.check() ) {
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

			if (distance >= maximumRange || distance <= minimumRange){
				// "out of range" logic
				// nothing to do right now :(
			}
			else {
				// "inside of range" logic
				if(currentProbe < PROBENUMBER) {
					probes[currentProbe] = distance;	
					++currentProbe;
				} else {
					currentProbe = 0;
				}
				sonicLastChange.reset(); // reset last change timer
				noEvent = 0;
			}
		} // end if (sonicMetro.check())

	    distancesmooth = 0; // reset distance smooth value
		for(uint8_t i=0; i < PROBENUMBER; ++i) {
			distancesmooth += probes[i];
		}
		distancesmooth = distancesmooth / PROBENUMBER;
		// map distance of sensor to color range
		hue = map(distancesmooth, minimumRange, maximumRange, 0, hueRange);
	}


	if(sonicLastChange.check() && !noEvent) {
		noEvent = 1;
	}

	if(noEvent) {
		if (brightness > brightnessMin) {
			if(fadeMetro.check()) {
				--brightness;
				fadeMetro.reset();
			}
		}
	}
	else {
		if (brightness < brightnessMax) {
			if(fadeMetro.check()) {
				++brightness;
				fadeMetro.reset();
			}
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
	//delay(10); // Wait (ms) to breath
}

void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3]) {
	uint16_t r_temp, g_temp, b_temp;
	uint8_t index_mod;
	uint8_t inverse_sat = (sat ^ 255);

	index = index % 768;
	index_mod = index % 256;

	if (index < 256) {
		r_temp = index_mod ^ 255;
		g_temp = index_mod;
		b_temp = 0;
	}
	else if (index < 512) {
		r_temp = 0;
		g_temp = index_mod ^ 255;
		b_temp = index_mod;
	}
	else if ( index < 768) {
		r_temp = index_mod;
		g_temp = 0;
		b_temp = index_mod ^ 255;
	}
	else {
		r_temp = 0;
		g_temp = 0;
		b_temp = 0;
	}
	// calculate saturation
	r_temp = ((r_temp * sat) / 255) + inverse_sat;
	g_temp = ((g_temp * sat) / 255) + inverse_sat;
	b_temp = ((b_temp * sat) / 255) + inverse_sat;
	// calculate brightness
	r_temp = (r_temp * bright) / 255;
	g_temp = (g_temp * bright) / 255;
	b_temp = (b_temp * bright) / 255;

	rgbColor[0] = (uint8_t)r_temp;
	rgbColor[1] = (uint8_t)g_temp;
	rgbColor[2] = (uint8_t)b_temp;
}