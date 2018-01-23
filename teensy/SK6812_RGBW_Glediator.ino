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
#define CYCLES_800_T0H  (F_CPU / 4000000)
#define CYCLES_800_T1H  (F_CPU / 1250000)
#define CYCLES_800      (F_CPU /  800000)

#define CYCLES_800_T0H  25
#define CYCLES_800_T1H  64
#define CYCLES_800      90

// ARM MCUs -- Teensy 3.0, 3.1, LC, Arduino Due ---------------------------
void WriteBytes(uint8_t* pixels, int numBytes){
  // T0H < T1H
  cli();
  uint8_t          *p   = pixels,
                   *end = p + numBytes, pix;
  uint32_t          cyc;

  ARM_DEMCR    |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
  // What happens when ARM_DWT_CYCCNT overflows?
    cyc = ARM_DWT_CYCCNT + CYCLES_800;

    while(p < end) {
      pix = *p++;
        while(ARM_DWT_CYCCNT - cyc < CYCLES_800);
        cyc  = ARM_DWT_CYCCNT;
        GPIOD_PDOR = 0xff;
        while(ARM_DWT_CYCCNT - cyc < CYCLES_800_T0H);
        GPIOD_PDOR = pix;
        while(ARM_DWT_CYCCNT - cyc < CYCLES_800_T1H);
        GPIOD_PDOR = 0;
    }
    while(ARM_DWT_CYCCNT - cyc < CYCLES_800);
    sei();
}

#define GPIO_CONFIG  (*(volatile unsigned short *)0x40048038)

void setup() {
  //Serial.begin(115200);
  Serial.begin(1000000); // 1mbit glediator
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  GPIO_CONFIG = ((unsigned short)0x00043F82); // 0b1000011111110000010
  
  // Pin 0 on Port D -> Pin "2"
  PORTD_PCR0 = PORT_PCR_MUX(0x1);
  PORTD_PCR1 = PORT_PCR_MUX(0x1);
  PORTD_PCR2 = PORT_PCR_MUX(0x1);
  PORTD_PCR3 = PORT_PCR_MUX(0x1);
  PORTD_PCR4 = PORT_PCR_MUX(0x1);
  // Set Pin 0 as Output
  GPIOD_PDDR = 31<< 0;
  // Set Pin 2 High
  GPIOD_PDOR = 1 << 0;

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
