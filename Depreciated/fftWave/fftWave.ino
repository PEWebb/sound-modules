#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioSynthWaveformSine sinewave;

// GUItool: begin automatically generated code
AudioOutputI2S           audioOutput;           //xy=171,649
AudioPlaySdRaw           playSdRawFile;  //xy=176,542
AudioAnalyzeFFT256       fftSound;       //xy=348,542
//AudioConnection          patchCord1(playSdRawFile, fftSound);
AudioConnection          patchCord1(sinewave, fftSound);
AudioControlSGTL5000     audioShield;     //xy=153,598
// GUItool: end automatically generated code

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.volume(0.5);

  // Configure the window algorithm to use
  fftSound.windowFunction(AudioWindowHanning256);

  sinewave.amplitude(0.8);
  sinewave.frequency(1034.007);
  
}

void loop() {
  float n;
  int i;

  if (fftSound.available()) {
    // each time new FFT data is available
    // print it all to the Arduino Serial Monitor
    Serial.print("FFT: ");
    for (i=0; i<40; i++) {
      n = fftSound.read(i);
      if (n >= 0.01) {
        Serial.print(n);
        Serial.print(" ");
      } else {
        Serial.print("  -  "); // don't print "0.00"
      }
    }
    Serial.println();
  }
}
