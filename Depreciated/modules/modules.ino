//--------------------------------------------------------------
// Teensy Sensor Module Test:
// Using potentiometer and single button
//
// Info: This program acts as a test base for creating
//       future sound modules.
//       - Single button for recording (press to start/stop)
//       - Switches to play if potentiometer reaches new value
//       - Currently uses the "Flange" effect preset
//--------------------------------------------------------------

// Including Audioshield libraries
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

// Pitch playback (not implemented)
#include "pitches.h"

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

// For debugging reasons, the Teensy's LED is used:
// HIGH: States 0,2 (Idle/playback)
// LOW: States 1 (Recording)
int ledPin = 13; // Will be off when recording
bool ledState = HIGH;

// Flange parameters
#define FLANGE_DELAY_LENGTH (12*AUDIO_BLOCK_SAMPLES)
int s_idx = 3*FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/8;
double s_freq = 0;
short delayline[FLANGE_DELAY_LENGTH];

// Record Button: pin 0 to GND
// Bounce objects to easily and reliably read the buttons
// 8 = 8 ms debounce time
Bounce buttonRecord = Bounce(0, 8);

// Mic settings
//const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;
const int flexpin = 0; 

// Remember which mode we're doing
int mode = 0;  // 0=stopped, 1=recording, 2=playing

// The file where data is recorded
File frec;

// Moving average sensor data array
double data[3];
double diffThresh = 10; // Threshold for autoplay
//-------------------------[Moving Average Functions]-----------------------
/* This function initializes an array for moving average
 *  @Param: double array_n[]: moving average array
 *  @Param: double val: initial value
 *  @Output: Returns nothing; but updates array
 */
void init_array(double array_n[], int val){
  for(int i = 0; i < sizeof(array_n); i++){
    array_n[i] = val;  
  }
}

/* This function calculates the difference between two sequential timesteps
 *  @Param: double array_n[]: moving average array
 *  @Output: Returns the difference between the moving average of two
 *           sequential timesteps
 */
double get_array_diff(double array_n[]){
  double total = 0;
  for(int i = 1; i < sizeof(array_n); i++){
    total += array_n[i];
  }
  double avg_1 = total/(sizeof(array_n)-1);
  
  total = 0;
  for(int i = 0; i < sizeof(array_n)-1; i++){
    total += array_n[i];
  }
  
  double avg_2 = total/(sizeof(array_n)-1);
  //Serial.println(avg_1);
  //Serial.println(avg_2);
  return avg_2 - avg_1;
}

/* This function populates the moving average array with new data
 *  @Param: double array_n[]: moving average array
 *  @Param: double val: initial value
 *  @Output: Returns nothing; but updates array
 */
void update_array(double array_n[], double val) {
  for(int i = 1; i < sizeof(array_n); i++){
    array_n[i] = array_n[i-1];
  }
  array_n[0] = val;
}

//------------------------------[Initialization]----------------------------
/* This functions initializes the Teensy module
 *  @Output: Returns nothing
 */
void setup() {
  // Set serial port baud rate
  Serial.begin(9600);
  
  // Configure the record pushbutton pin
  pinMode(0, INPUT_PULLUP);

  // Configure the LED on the Teensy
  pinMode(ledPin, OUTPUT);

  // Audio connections require memory, and the record queue
  // uses this memory to buffer incoming audio.
  AudioMemory(60);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(1);

  // Initialize flange
  flange1.begin(delayline, FLANGE_DELAY_LENGTH, s_idx, s_depth, s_freq);

  // Initialize data array
  int flexposition = analogRead(flexpin);
  double mappedFlex = map(flexposition, 8, 10, 0, 800)/1000.0;
  init_array(data, mappedFlex);

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

//-----------------------------------[Main]---------------------------------
/* This is the main function
 *  @Output: Returns nothing
 */
void loop() {

  // Define variables to store data
  int flexposition;
  double mappedFlex;   

  // Read data from potentiometer
  flexposition = analogRead(flexpin);
  //delayTime = map(flexposition, 1023, 930, 100, 0)/100.0;
  //sgtl5000_1.volume(delayTime);
  mappedFlex = map(flexposition, 8, 10, 0, 800)/1000.0;

  // Apply flange
  flange1.voices(s_idx, s_depth, mappedFlex);
  
  // Debugging print statements
//  Serial.print("sensor: ");
//  Serial.print(flexposition);
//  Serial.print("  ");
//  Serial.println(mappedFlex);

  // Update data array
  update_array(data, mappedFlex);

  // Get difference from this and last time step
  double diff = abs(get_array_diff(data));
  
  // when using a microphone, continuously adjust gain
  if (myInput == AUDIO_INPUT_MIC) adjustMicLevel();
  
  // First, read the buttons
  buttonRecord.update();

  // If potentiometer is changed enough
  if (diff > diffThresh) {
    
    Serial.println("Play");
    if (mode == 1) stopRecording();
    if (mode == 0) startPlaying();
  }
  
  // Respond to button presses
  // If record button is pressed
  else if (buttonRecord.fallingEdge()) {
    Serial.println("Record Button Press");

    // If currently playing something: stop
    if (mode == 2) stopPlaying();

    // If currently recording: stop
    if (mode == 1) stopRecording();

    // Else (idle): start recording
    else if (mode == 0) startRecording();
  }
  
  // If its playing or recording, carry on...
  else if (mode == 1) {
    continueRecording();
  }
  else if (mode == 2) {
    continuePlaying();
  }

  // Update LED state
  Serial.println(mode);
//  digitalWrite(ledPin, ledState); // updates LED

}

//-------------------------------[Record/Play]------------------------------
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

    ledState = HIGH;
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

    ledState = HIGH;
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

