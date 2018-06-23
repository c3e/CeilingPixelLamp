// a lesson was learned but the damage is irreversible
#define NUM_LINES 3
#define NUM_LEDS_PER_LINE (3*64)

uint8_t gammaCorrection[] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
	2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
	5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
	10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
	17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
	25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
	37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
	51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
	69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
	90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
	115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
	144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
	177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
	215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

uint8_t whiteStart = 0;


uint8_t pixelbuffer[NUM_LEDS_PER_LINE*NUM_LINES*4];
uint8_t pixelbuffer_reswizzle[NUM_LEDS_PER_LINE*32];
//#define CYCLES_800_T0H  (F_CPU / 4000000)
//#define CYCLES_800_T1H  (F_CPU / 1250000)
//#define CYCLES_800      (F_CPU /  800000)

#define CYCLES_800_T0H  25
#define CYCLES_800_T1H  64
#define CYCLES_800      90

// ARM MCUs -- Teensy 3.0, 3.1, LC, Arduino Due ---------------------------
void WriteBytes(uint8_t* pixels, int numBytes){
	// T0H < T1H

	// clear all interrupts before changing control register values
	cli();

	uint8_t	 *p   = pixels;
	uint8_t	 *end = p + numBytes;
	uint8_t   pix;
	uint32_t  cyc;

	// Control Registers used:
	//	DWT 			Data and Address Watchpoints 
	//				is a debug unit
	//				consiting of four comparators
	//	ARM_DWT_CYCCNT 		the DWTs CYCle-CouNT register
	//	ARM_DWT_CTRL_CYCCNTENA 	enable the DWTs cycle counter
	//
	//	DEMCR			Debug Exception and Monitor Control Register
	//	ARM_DEMCR_TRCENA 	Set to 1 to enable use of the trace and debug
	//				blocks (DWT, ITM, ETM, TPIU)

	ARM_DEMCR    |= ARM_DEMCR_TRCENA;
	ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
	// What happens when ARM_DWT_CYCCNT overflows?
	// An event is emitted (Reference Manual p 203)

	
	/*
	  Data Transmission Timing (according to official documentation)
	  
	  T0H | 0 code, high level time    |  0.3 us | +/- 0.15us
	  T0L | 0 code, low  level time    |  0.9 us | +/- 0.15us
	  T1H | 1 code, high level time    |  0.6 us | +/- 0.15us
	  T1L | 1 code, low  level time    |  0.6 us | +/- 0.15us
	  Trst| Reset code, low level time | 80   us
	  
	  Input code
	  
	        +-----------+                        |
	  0     |           | <-        T0L       -> |
	        | <- T0H -> |________________________|
	
	        +-----------------+                  |
	  1     |                 | <-    T1L     -> |
	        | <-    T1H    -> |__________________|
	
	        |                   / /              |
	  Reset | <-     Trst      / /            -> |
	        |________________ / /________________|
	
	
	  Data Transmission Timing (according to https://cpldcpu.wordpress.com/2014/01/19/light_ws2812-library-v2-0/)
	  
	  Input code
	  
	        t0          t1    t2                 t3
	        *           *     *                  *
	        *           *     *                  *
	        *           *     *                  *
	        +-----------+     *                  |
	  0     |           | <-  *     T0L       -> |
	        | <- T0H -> |_____*__________________|
	        *           *     *                  *
	        +-----------------+                  |
	  1     |           *     | <-    T1L     -> |
	        | <-    T1H *  -> |__________________|
	
	  CYCLES_800_T0H  ~ 0.25 us	(based on F_CPU / 4 000 000) ~ 0.35 us (based on 25 cycles)
	  CYCLES_800_T1H  ~ 0.8  us	(based on F_CPU / 1 250 000) ~ 0.88 us (based on 64 cycles)
	  CYCLES_800	  ~ 1.25 us	(based on F_CPU /   800 000) ~ 1.25 us (based on 90 cycles)	=> 800 kHz
	*/
	
	cyc = ARM_DWT_CYCCNT + CYCLES_800;
	
	while(p < end) {
		pix = *p++;
		while(ARM_DWT_CYCCNT - cyc < CYCLES_800){
			// wait for cycle-sync (the remaining time from t2 - t3)
		}
		// Store current cycle count as start
		cyc  = ARM_DWT_CYCCNT;
		// Set Port D high (because both symbols start with a high signal)
		GPIOD_PDOR = 0xff;
		
		while(ARM_DWT_CYCCNT - cyc < CYCLES_800_T0H){
			// both symbols share their high signal from t0 - t1
		}
		
		//change signal from high to low if necessary (to transmit a 0 signal)
		GPIOD_PDOR = pix;
		
		while(ARM_DWT_CYCCNT - cyc < CYCLES_800_T1H){
			// wait from t1 - t2 
		}
		
		// Set Port D low again, because both symbols end with a low signal
		GPIOD_PDOR = 0;
	}
	while(ARM_DWT_CYCCNT - cyc < CYCLES_800)
	{
		// generate reset signal by waiting another cycle
	}
	
	// set all interrupts after setting control registers 
	// to reenable the programs functionality
	sei();
}

// GPIO_CONFIG points to System Clock Gating Control Register 5 (SIM_SCGC5)
#define GPIO_CONFIG  (*(volatile unsigned short *)0x40048038)

void setup() {
	//Serial.begin(115200);
	Serial.begin(1000000); // 1mbit glediator
	
	// Switch on orange on-board LED
	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH);

	//GPIO_CONFIG = ((unsigned short)0x00043F82); // 0b1000011111110000010


	GPIO_CONFIG |= 		(1 << 9)	// Enable Clock on PORT A
				 	  | (1 << 10)	// Enable Clock on PORT B
				 	  | (1 << 11)	// Enable Clock on PORT C
				 	  | (1 << 12)	// Enable Clock on PORT D
				 	  | (1 << 13);	// Enable Clock on PORT E

	// Pin 0 on Port D -> Pin "2"
	// PCR Pin Control Register
	// configure pins 0 - 4 as GPIO pin (via MUX-register value 001)
	PORTD_PCR0 = PORT_PCR_MUX(0x1);
	PORTD_PCR1 = PORT_PCR_MUX(0x1);
	PORTD_PCR2 = PORT_PCR_MUX(0x1);
	PORTD_PCR3 = PORT_PCR_MUX(0x1);
	PORTD_PCR4 = PORT_PCR_MUX(0x1);
	// Set Pin 0 - 4 as Output
	// PDDR Port Data Direction Register
	GPIOD_PDDR |= 		(1 << 4)	// Set Pin 5 on PORT D as Output
					  | (1 << 3)	// Set Pin 4 on PORT D as Output
					  | (1 << 2)	// Set Pin 3 on PORT D as Output
					  | (1 << 1)	// Set Pin 2 on PORT D as Output
					  | (1 << 0);	// Set Pin 1 on PORT D as Output
	
	// Set Pin 2 High
	// PDOR Port Data Output Register
	// Set logic level to 1
	GPIOD_PDOR = (1 << 0);

	for(int i = 0; i != 32*NUM_LEDS_PER_LINE; i++){
		pixelbuffer_reswizzle[i] = 0;
	}
	WriteBytes(pixelbuffer_reswizzle, 32*NUM_LEDS_PER_LINE);
}


int serialGlediator() {
	while (!Serial.available()) {
		; // do nothing :P
	}
	return Serial.read();
}


uint8_t ledColor[3] = {0, 0, 0};


void loop() {
	while (serialGlediator() != 1) {
		// wait for it
	} 

	int idx = 0;
	for (size_t i = 0; i < NUM_LINES*NUM_LEDS_PER_LINE; i++) {
		pixelbuffer[idx++] = gammaCorrection[serialGlediator()]; // G
		pixelbuffer[idx++] = gammaCorrection[serialGlediator()]; // R
		pixelbuffer[idx++] = gammaCorrection[serialGlediator()]; // B
		pixelbuffer[idx++] = gammaCorrection[whiteStart];        // W

	}

	for(int i = 0; i != 32*NUM_LEDS_PER_LINE; i++){
		pixelbuffer_reswizzle[i] = 0;
	}

	// Map pixel buffer 
	for(size_t line = 0; line != NUM_LINES; line++){
		for(size_t pixel = 0; pixel < NUM_LEDS_PER_LINE; pixel++){
			for(int channel = 0; channel != 4; channel++){
				uint8_t data = pixelbuffer[pixel*4+channel+4*NUM_LEDS_PER_LINE*line];
				size_t offset = 8*channel+32*pixel;
				pixelbuffer_reswizzle[7+offset]|=(data&1)<<line;
				data >>= 1;
				pixelbuffer_reswizzle[6+offset]|=(data&1)<<line;
				data >>= 1;
				pixelbuffer_reswizzle[5+offset]|=(data&1)<<line;
				data >>= 1;
				pixelbuffer_reswizzle[4+offset]|=(data&1)<<line;
				data >>= 1;
				pixelbuffer_reswizzle[3+offset]|=(data&1)<<line;
				data >>= 1;
				pixelbuffer_reswizzle[2+offset]|=(data&1)<<line;
				data >>= 1;
				pixelbuffer_reswizzle[1+offset]|=(data&1)<<line;
				data >>= 1;
				pixelbuffer_reswizzle[0+offset]|=(data&1)<<line;
				data >>= 1;
			}
		}
	}

	WriteBytes(pixelbuffer_reswizzle, 32*NUM_LEDS_PER_LINE);
}
