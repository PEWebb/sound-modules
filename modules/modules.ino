#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

// GUItool: begin automatically generated code
AudioPlaySdRaw           playSdRaw1;     //xy=293,302
AudioInputI2S            i2s1;           //xy=374,58
AudioEffectFlange        flange1;        //xy=449,236
AudioRecordQueue         queue1;         //xy=512,139
AudioOutputI2S           i2s2;           //xy=591,280
AudioConnection          patchCord1(playSdRaw1, flange1);
AudioConnection          patchCord2(i2s1, 1, queue1, 0);
AudioConnection          patchCord3(flange1, 0, i2s2, 0);
AudioConnection          patchCord4(flange1, 0, i2s2, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=458,435
// GUItool: end automatically generated code

//   Record Button: pin 0 to GND
//   Stop Button:   pin 1 to GND
//   Play Button:   pin 2 to GND

#define FLANGE_DELAY_LENGTH (12*AUDIO_BLOCK_SAMPLES)
int s_idx = 3*FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/8;
double s_freq = 0;
short delayline[FLANGE_DELAY_LENGTH];

// Bounce objects to easily and reliably read the buttons
Bounce buttonRecord = Bounce(0, 8);
Bounce buttonStop =   Bounce(1, 8);  // 8 = 8 ms debounce time
Bounce buttonPlay =   Bounce(2, 8);

//const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;
const int flexpin = 0; 

// Remember which mode we're doing
int mode = 0;  // 0=stopped, 1=recording, 2=playing

// The file where data is recorded
File frec;

void setup() {
  // Configure the pushbutton pins
  Serial.begin(9600);
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);

  // Audio connections require memory, and the record queue
  // uses this memory to buffer incoming audio.
  AudioMemory(60);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(1);

  flange1.begin(delayline, FLANGE_DELAY_LENGTH, s_idx, s_depth, s_freq);

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
  buttonStop.update();
  buttonPlay.update();

  int flexposition;
  double mappedFlex;   

  flexposition = analogRead(flexpin);
  //delayTime = map(flexposition, 1023, 930, 100, 0)/100.0;
  //sgtl5000_1.volume(delayTime);
  mappedFlex = map(flexposition, 8, 10, 0, 800)/1000.0;
  flange1.voices(s_idx, s_depth, mappedFlex);

  Serial.print("sensor: ");
  Serial.print(flexposition);
  Serial.print("  ");
  Serial.println(mappedFlex);
  

  // Respond to button presses
  if (buttonRecord.fallingEdge()) {
    Serial.println("Record Button Press");
    if (mode == 2) stopPlaying();
    if (mode == 0) startRecording();
  }
  if (buttonStop.fallingEdge()) {
    Serial.println("Stop Button Press");
    if (mode == 1) stopRecording();
    if (mode == 2) stopPlaying();
  }

  if (buttonPlay.fallingEdge()) {
    Serial.println("Play Button Press");
    if (mode == 1) stopRecording();
    if (mode == 0) startPlaying();
  }

  // If its playing or recording, carry on...
  if (mode == 1) {
    continueRecording();
  }
  if (mode == 2) {
    continuePlaying();
  }

  // when using a microphone, continuously adjust gain
  if (myInput == AUDIO_INPUT_MIC) adjustMicLevel();

}


void startRecording() {
  Serial.println("startRecording");
  if (SD.exists("RECORD.RAW")) {
    // The SD library writes new data to the end of the
    // file, so to start a new recording, the old file
    // must be deleted before new data is written.
    SD.remove("RECORD.RAW");
  }
  frec = SD.open("RECORD.RAW", FILE_WRITE);
  if (frec) {
    queue1.begin();
    mode = 1;
  }
}

void continueRecording() {
  if (queue1.available() >= 2) {
    byte buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer+256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    // write all 512 bytes to the SD card
    elapsedMicros usec = 0;
    frec.write(buffer, 512);
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
  queue1.end();
  if (mode == 1) {
    while (queue1.available() > 0) {
      frec.write((byte*)queue1.readBuffer(), 256);
      queue1.freeBuffer();
    }
    frec.close();
  }
  mode = 0;
}


//this is where I need to add delayTime to control delay: map (0, 400)
void startPlaying() {
  Serial.println("startPlaying");
  playSdRaw1.play("RECORD.RAW");
  mode = 2;
  
}


void continuePlaying() {
  if (!playSdRaw1.isPlaying()) {
    playSdRaw1.stop();
    mode = 0;
  }
}

void stopPlaying() {
  Serial.println("stopPlaying");
  if (mode == 2) playSdRaw1.stop();
  mode = 0;
}

void adjustMicLevel() {
  // TODO: read the peak1 object and adjust sgtl5000_1.micGain()
  
}

