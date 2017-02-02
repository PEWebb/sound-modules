// Record sound to mic and playback upon button press
// Alan Cheng
// 2/2/2017

// Adapted from code by by PaulStoffregen: 
// https://github.com/PaulStoffregen/Audio/blob/master/examples/Recorder/Recorder.ino#L181

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2sIn;           //xy=173.1999969482422,160.1999969482422
AudioPlaySdRaw           playSdRawFile;     //xy=179.11112213134766,311.3333215713501
AudioEffectFlange        flangeEffect;        //xy=359.1111068725586,311.3333396911621
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
double s_freq = 0;
short delayline[FLANGE_DELAY_LENGTH];

const int flexpin = 0;

// Bounce objects to easily and reliably read the buttons
Bounce buttonRecord = Bounce(0, 8);
Bounce buttonPlay   = Bounce(1, 8);  // 8 = 8 ms debounce time

// Which is the input medium
//const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;

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
  sgtl5000_1.volume(0.5);

  // Initialize flange
  flangeEffect.begin(delayline, FLANGE_DELAY_LENGTH, s_idx, s_depth, s_freq);
  

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
  // First, read the buttons
  buttonRecord.update();
  buttonPlay.update();

  int flexposition = analogRead(flexpin);
  double mappedFlexUncap = map(flexposition, 3, 30, 0, 10);
  double mappedFlex = constrain(mappedFlexUncap,0,10)/10;

  Serial.print("Flex: ");
  Serial.print(flexposition);
  Serial.print(", Mapped: ");
  Serial.print(mappedFlex);
  Serial.print("\n");

  flangeEffect.voices(s_idx, s_depth, mappedFlex);

  // Respond to button presses
  if (buttonRecord.fallingEdge()) {
    Serial.println("Record Button Press");

    // If currently playing something: stop
    if (mode == 2) stopPlaying();

    // If currently recording: stop
    if (mode == 1) stopRecording();

    // Else (idle): start recording
    else if (mode == 0) startRecording();
  }

  if (buttonPlay.fallingEdge()) {
    Serial.println("Play Button Press");
    if (mode == 1) stopRecording();
    if (mode == 0) startPlaying();
  }

  // If we're playing or recording, carry on...
  if (mode == 1) {
    continueRecording();
  }
  if (mode == 2) {
    continuePlaying();
  }

  // when using a microphone, continuously adjust gain
//  if (myInput == AUDIO_INPUT_MIC) adjustMicLevel();
  
}

void startRecording() {
  Serial.println("startRecording");
  if (SD.exists("RECORD.RAW")) {
    // The SD library writes new data to the end of the
    // file, so to start a new recording, the old file
    // must be deleted before new data is written.
    SD.remove("RECORD.RAW");
  }
  fRec = SD.open("RECORD.RAW", FILE_WRITE);
  if (fRec) {
    micQueue.begin();
    mode = 1;
  }
}

void continueRecording() {
  if (micQueue.available() >= 2) {
    byte buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, micQueue.readBuffer(), 256);
    micQueue.freeBuffer();
    memcpy(buffer+256, micQueue.readBuffer(), 256);
    micQueue.freeBuffer();
    // write all 512 bytes to the SD card
    elapsedMicros usec = 0;
    fRec.write(buffer, 512);
    // Uncomment these lines to see how long SD writes
    // are taking.  A pair of audio blocks arrives every
    // 5802 microseconds, so hopefully most of the writes
    // take well under 5802 us.  Some will take more, as
    // the SD library also must write to the FAT tables
    // and the SD card controller manages media erase and
    // wear leveling.  The queue1 object can buffer
    // approximately 301700 us of audio, to allow time
    // for occasional high SD card latency, as long as
    // the average write time is under 5802 us.
    //Serial.print("SD write, us=");
    //Serial.println(usec);
  }
}

void stopRecording() {
  Serial.println("stopRecording");
  micQueue.end();
  if (mode == 1) {
    while (micQueue.available() > 0) {
      fRec.write((byte*)micQueue.readBuffer(), 256);
      micQueue.freeBuffer();
    }
    fRec.close();
  }
  mode = 0;
}


void startPlaying() {
  Serial.println("startPlaying");
  playSdRawFile.play("RECORD.RAW");
  mode = 2;
}

void continuePlaying() {
  if (!playSdRawFile.isPlaying()) {
    playSdRawFile.stop();
    mode = 0;
  }
}

void stopPlaying() {
  Serial.println("stopPlaying");
  if (mode == 2)
    playSdRawFile.stop();
  mode = 0;
}
