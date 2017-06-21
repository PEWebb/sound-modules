#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <cfft256.h>

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;
  
// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputI2S        audioInput;         // audio shield: mic or line-in
AudioPlaySdWav       playSdWavFile;
AudioCFFT256         cfft;
AudioOutputI2S       audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
AudioConnection c1(playSdWavFile, 0, cfft, 0);
AudioConnection c2(cfft, 0, audioOutput, 0);
//AudioConnection c3(playSdWavFile, 0, audioOutput, 0);

// Create an object to control the audio shield.
AudioControlSGTL5000 audioShield;          

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(.5);

  cfft.windowFunction(AudioWindowHanning256);

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
  
  startPlaying();
  Serial.println("Initialization Complete");
}

void loop() {
  if (cfft.available()) {
    for(int i = 0; i < 20; i++) {
//      Serial.print(cfft.block->data[i]);
//      Serial.print(",");
      Serial.print(cfft.buffer[i]);
      Serial.print(" | ");
    }
    Serial.println();
  }
}

void startPlaying() {
  Serial.println("startPlaying");
  playSdWavFile.play("river16.wav");
}
