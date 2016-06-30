#include <NewPing.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <SPI.h>
#include <Wire.h>
#include <WS2812.h>

#include "var.h"

// auto cycle color / rainbow wheel
//const uint8_t autoCycle = 1; // color cycle auto switch
//uint8_t autoCycleDirection = 1; // current direction of auto color cycle

void refreshRawDistance(); // get raw distance from ultrasonic sensor
void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3]); // convert HSB to RGB color
void setAllWhiteLedsBrightness(uint8_t _newBrightness);

// Can Stuff
uint8_t MODE = 1; //sets Mode to led
uint8_t ADDRESS = 1; 

NewPing sonar(ECHOPIN, ECHOPIN, maximumRange); // NewPing setup of pin and maximum distance.
MCP_CAN CAN0(CANCSPIN);  // initializing CAN BUS


// auto cycle color / rainbow wheel
const uint8_t autoCycle = 1; // color cycle auto switch
uint8_t autoCycleDirection = 1; // current direction of auto color cycle

void setup() {
	LED.setColorOrderRGB();  // RGB color order
	


	if(autoCycle == 1)
		brightness = brightnessMax;
	
	Serial.begin(9600);

	CAN0.begin(CAN_500KBPS, MCP_8MHz);	// init can bus : baudrate = 500k 
	pinMode(CANRXPIN, INPUT);

	while (CAN_OK != CAN0.begin(CAN_500KBPS, MCP_8MHz))              // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println("Init CAN BUS Shield again");
        delay(100);
    }
    Serial.println("CAN BUS Shield init ok!");
    attachInterrupt(0, recvData, FALLING); // start interrupt

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

unsigned char len = 0;
unsigned char buf[8];
uint8_t p_addr[0] = buf[1] >> 4;
uint8_t p_addr[1] = buf[1] & 15;
  
unsigned char barrierbuffer[48];

/**
 * CAN Bus testing stuff
 */

void recvData (){
	// Serial.print("Got Values: ");
	CAN0.readMsgBuf(&len, buf);
	uint32_t id = CAN0.getCanId();
	uint8_t control_bits = id & 7;
	uint8_t p_addr[4];
	uint8_t rgb[12];
	if ( control_bits > 3){
		for ( size_t i = 0;  i< 4 ; i++){
				rgb[i*3]   = buf[0] >> 3 ;
				rgb[i*3+1] = ( buf[0] & 7 ) << 3 + buf[1] >> 5;
				rgb[i*3+2] = buf[1] & 31  ;
		}
		p_addr[3] = (id >> 13) && 15;
	}
	p_addr[0] = (id >> 25) && 15;
	p_addr[1] = (id >> 21) && 15;
	p_addr[2] = (id >> 17) && 15;
	
	if ( id >> 3 & 127 == ADDRESS && control_bits != 3){
		
		switch (control_bits) {
			
			case 0: //sync 
		        
				LED.set_crgb_at( p_addr[0] , { ( id >> 9 ) & 0xFF,buf[0],buf[1] } );
				if (len > 2)
					LED.set_crgb_at( p_addr[1] , { buf[2],buf[3],buf[4] } );
				if ( len > 5)
		        	LED.set_crgb_at( p_addr[2] , { buf[5],buf[6],buf[7] } );
		        LED.sync();
				break;
		
			case 1: //buffer until barrier
				barrierbuffer[ p_addr[0] * 3 ] 	   = ( id >> 9 ) & 0xFF;
				barrierbuffer[ p_addr[0] * 3 + 1 ] = buf[0];
				barrierbuffer[ p_addr[0] * 3 + 2 ] = buf[1];
				if ( len > 2){
					barrierbuffer[ p_addr[1] * 3 ] 	   = buf[2];
					barrierbuffer[ p_addr[1] * 3 + 1 ] = buf[3];
					barrierbuffer[ p_addr[1] * 3 + 2 ] = buf[4];
				}	
				if ( len > 5){
		        	barrierbuffer[ p_addr[2] * 3 ] 	   = buf[5];
		       		barrierbuffer[ p_addr[2] * 3 + 1 ] = buf[6];
		        	barrierbuffer[ p_addr[2] * 3 + 2 ] = buf[7];
				}
				break;
			
			case 2: //barrier signal + payload
		        barrierbuffer[ p_addr[0] * 3 ] 	   = (id >> 9) & 0xFF;
				barrierbuffer[ p_addr[0] * 3 + 1 ] = buf[0];
				barrierbuffer[ p_addr[0] * 3 + 2 ] = buf[1];
				if ( len > 2){
					barrierbuffer[ p_addr[1] * 3 ] 	   = buf[2];
					barrierbuffer[ p_addr[1] * 3 + 1 ] = buf[3];
					barrierbuffer[ p_addr[1] * 3 + 2 ] = buf[4];
				}	
				if ( len > 5) {
		        	barrierbuffer[ p_addr[2] * 3 ] 	   = buf[5];
		        	barrierbuffer[ p_addr[2] * 3 + 1 ] = buf[6];
		        	barrierbuffer[ p_addr[2] * 3 + 2 ] = buf[7];
        		}

				for ( uint8_t i = 0; i < 16; i++ ){
					LED.set_crgb_at( i, { barrierbuffer[i*3], barrierbuffer[i*3+1], barrierbuffer[i*3+2] } );
				}
				memset(barrierbuffer, 0, 48);
				LED.sync();
				break;

			case 4: //sync new layout
				for ( size_t i = 0; i<4; i++)
					LED.set_crgb_at( p_addr[i] , { rgb[i*3],rgb[i*3+1],rgb[i+3+2] } );
				LED.sync();
				break;

			case 5:
				for ( size_t i = 0; i<4; i++){
					barrierbuffer[p_addr[i]] = rgb[i*3];
					barrierbuffer[p_addr[i]+1] = rgb[i*3+1];
					barrierbuffer[p_addr[i]+2] = rgb[i*3+2];
				}
				break;

			case 6:
				case 5:
				for ( size_t i = 0; i<4; i++){
					barrierbuffer[p_addr[i]] = rgb[i*3];
					barrierbuffer[p_addr[i]+1] = rgb[i*3+1];
					barrierbuffer[p_addr[i]+2] = rgb[i*3+2];
				}

				for ( uint8_t i = 0; i < 16; i++ ){
					LED.set_crgb_at( i, { barrierbuffer[i*3], barrierbuffer[i*3+1], barrierbuffer[i*3+2] } );
				}
				memset(barrierbuffer, 0, 48);
				LED.sync();
				break;

			case 7: //sync one/two
				LED.set_crgb_at( p_addr[0] , { id && 0xFF,buf[0],buf[1] } );

				break;

		}
	}

	//
	//	Whiting Out
	//	6 panels with one packet, so there are 6 addresses 
	//	and a 8bit value for each address
	//  
	//	Also there is a bit next to id, to set panels to autacious mode 
	//

	if ( control_bits == 3 ) { 
		if (id & 8 == 8 ) {
			 MODE = 1;
		} else if ( id & 8 == 0 ) {
			 MODE = 0;
		} else if ( buf[1] >> 2  == ADDRESS){
			setAllWhiteLedsBrightness(buf[4]);
		} else if ( ( ( buf[1] & 3) << 4 ) + ( buf[2] >> 4 ) == ADDRESS ){
			setAllWhiteLedsBrightness(buf[5]);
		} else if ( ( ( buf[2] & 15) << 2 ) + ( ( buf[3] & 192 ) >> 6 ) == ADDRESS ){
			setAllWhiteLedsBrightness(buf[6]);
		} else if ( buf[3] & 127 == ADDRESS ){
			setAllWhiteLedsBrightness(buf[7]);
		} else if ( (id >> 9) & 127 == ADDRESS) {
			setAllWhiteLedsBrightness(buf[0]);
		} else if ( (id >> 15) & 127 == ADDRESS ) {
			setAllWhiteLedsBrightness(id >> 21);
		}
	}
}

void setAllWhiteLedsBrightness(uint8_t _newBrightness=0) {
	analogWrite(LEDWHITEPIN1, _newBrightness);
	analogWrite(LEDWHITEPIN2, _newBrightness);
	analogWrite(LEDWHITEPIN3, _newBrightness);
	analogWrite(LEDWHITEPIN4, _newBrightness);
}

int ledDistance () {
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
			setAllWhiteLedsBrightness(Whitebrightness);
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

		//delay(20);

		if(FadingIn == 1) { // still fading in?
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
}

void refreshRawDistance(){
	/* The following TRIGPIN/ECHOPIN cycle is used to determine the
		distance of the nearest object by bouncing soundwaves off of it. */
	// Calculate the distance (in centimeter or inches) based on the speed of sound.
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
