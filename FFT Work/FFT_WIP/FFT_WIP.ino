#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2sIn;           //xy=173.1999969482422,160.1999969482422
AudioPlaySdRaw           playSdRawFile;     //xy=179.11112213134766,311.3333215713501
AudioOutputI2S           i2sOut;           //xy=521.8666572570801,303.08887481689453
AudioAnalyzePeak         micPeak;          //xy=525.7777366638184,100.2222146987915
AudioRecordQueue         micQueue;         //xy=529.4222183227539,154.1999912261963
AudioConnection          patchCord1(i2sIn, 0, micQueue, 0);
AudioConnection          patchCord2(i2sIn, 0, micPeak, 0);
AudioConnection          patchCord3(playSdRawFile, flangeEffect);
AudioConnection          patchCord4(flangeEffect, 0, i2sOut, 1);
AudioConnection          patchCord5(flangeEffect, 0, i2sOut, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=408,471.3333148956299
// GUItool: end automatically generated code

// Flange parameters
#define FLANGE_DELAY_LENGTH (12*AUDIO_BLOCK_SAMPLES)
int s_idx = 3*FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/8;
double s_freq = 0.5;
short delayline[FLANGE_DELAY_LENGTH];

const int flexpin = 2;

// Bounce objects to easily and reliably read the buttons
Bounce buttonRecord = Bounce(0, 8);
Bounce buttonPlay   = Bounce(1, 8);  // 8 = 8 ms debounce time

// Which is the input medium
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// Remember which mode we're doing
int mode = 0;  // 0=stopped, 1=recording, 2=playing

// The file where data is recorded
File fRec;

void setup() {
  // Configure the pushbutton pins
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);

  // Audio connections require memory, and the record queue
  // uses this memory to buffer incoming audio.
  AudioMemory(60);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(1.0);
  
  // Initialize the SD card
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    // stop here if no SD card, but print a message
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
}

void loop() {
    
}
