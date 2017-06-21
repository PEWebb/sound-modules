//https://forum.pjrc.com/threads/33624-AudioOctoWiz
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <OctoWS2811.h>
#include <Encoder.h>
#include <Bounce.h>

const int EncoderOneSwitch = 24;
const int EncoderOneA = 31;
const int EncoderOneB = 25;
const int EncoderTwoSwitch = 28;
const int EncoderTwoA = 26;
const int EncoderTwoB = 27;

const int analogPotOne = A12;
const int analogPotTwo = A13;
const int LED = 32;

const int ledsPerStrip = 64;
// The display size and color to use
const unsigned int matrix_width = 32;
const unsigned int matrix_height = 8;

// These parameters adjust the vertical thresholds
const float maxLevel = 0.4;      // 1.0 = max, lower is more "sensitive"
const float dynamicRange = 40.0; // total range to display, in decibels
const float linearBlend = 0.3;   // useful range is 0 to 0.7

DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];

const int config = WS2811_GRB | WS2811_800kHz;

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

AudioInputI2S           audioInput;         // audio shield: mic or line-in
AudioAnalyzeFFT1024     myFFT;
AudioOutputI2S          audioOutput;        // audio shield: headphones & line-out

// Connect live input
AudioConnection patchCord1(audioInput, 0, myFFT, 0);
AudioConnection c3(audioInput,0,audioOutput,0);
AudioConnection c4(audioInput,1,audioOutput,1);

AudioControlSGTL5000 audioShield;

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

const int lowerFFTBins[] = {0,1,2,3,4,5,6,8,10,12,15,18,22,27,32,38,45,53,63,74,87,102,119,138,160,186,216,250,289,334,385,444};
const int upperFFTBins[] = {0,1,2,3,4,5,7,9,11,14,17,21,26,31,37,44,52,62,73,86,101,118,137,159,185,215,249,288,333,384,443,511};
float thresholdVertical[matrix_height];
float thresholdVert[matrix_height];

Bounce encButton1 = Bounce(EncoderOneSwitch, 5);
Bounce encButton2 = Bounce(EncoderTwoSwitch, 5);

Encoder myEnc1(EncoderOneA, EncoderOneB);
Encoder myEnc2(EncoderTwoA, EncoderTwoB);

void setup() {
  Serial.begin(115200);
  pinMode(EncoderOneSwitch, INPUT_PULLUP);
  pinMode(EncoderTwoSwitch, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  leds.begin();
  leds.show();

  analogReadResolution(8);
  analogReadAveraging(4);

  AudioMemory(20);

  // compute the vertical thresholds before starting
  computeVerticalLevels();

  for(int i = 0; i < 8; i++){
    Serial.print("thresholdVertical ");
    Serial.print(i);
    Serial.print(" = ");
    Serial.println(thresholdVertical[i]);
  }

  for(unsigned int j = 0; j < matrix_height; j++) {
    thresholdVert[j] = thresholdVertical[matrix_height-j-1];
  }

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.6);
  
  // Configure the window algorithm to use
  myFFT.windowFunction(AudioWindowHanning1024);
  //myFFT.windowFunction(NULL);

}

byte oldEncValue1 = 0;
byte oldEncValue2 = 0;
byte color = 0;
byte bright = 0;
byte potOne;
byte potTwo;
byte mode;
unsigned int x, y;
float level;
elapsedMillis fps;
elapsedMillis cycle;
//elapsedMicros xTime;

void loop() {

  if (cycle > 200){
    cycle = 0;
    potOne = analogRead(analogPotOne);
    potTwo = analogRead(analogPotTwo);
    if(potOne >= 0 && potOne < 64){mode = 0;}
    if(potOne >= 64 && potOne < 128){mode = 1;}
    if(potOne >= 128 && potOne < 192){mode = 2;}
    if(potOne >= 192 && potOne < 256){mode = 3;}
  }
  
  if (myFFT.available()) {
    //xTime = 0;
    switch(mode){
      case 0:
        colorRainbow();
        break;     
      case 1:
        colorFixed(potTwo);
        break;
      case 2:
        colorSweep(potTwo);
        break;
      case 3:
        colorBright(potTwo);
        break;
      default:
        colorRainbow();
    }
    //Serial.println(xTime);
  }
}    

void colorRainbow(){
   for(x=0; x < matrix_width; x++){
      level = myFFT.read(lowerFFTBins[x],upperFFTBins[x]);
      for(y=0; y < 8; y++){
        if(level >= thresholdVert[y]){
          leds.setPixel(xy(x,y), Wheel(y*32));
        } 
        else {
          leds.setPixel(xy(x,y), 0x000000);
        }
      }
    }
    leds.show();
}

void colorFixed(byte c){
    for(x=0; x < matrix_width; x++){
      level = myFFT.read(lowerFFTBins[x],upperFFTBins[x]);
      
      for(y=0; y < 8; y++){
        if(level >= thresholdVert[y]){
          leds.setPixel(xy(x,y), Wheel(c));
        } 
        else {
          leds.setPixel(xy(x,y), 0x000000);
        }
      }
    }
    leds.show();
}

void colorSweep(byte p){
    if(fps > p){
      fps = 0;
      color++;
    }
    
    for(x=0; x < matrix_width; x++){
      level = myFFT.read(lowerFFTBins[x],upperFFTBins[x]);
      
      for(y=0; y < 8; y++){
        if(level >= thresholdVert[y]){
          leds.setPixel(xy(x,y), Wheel(color));
        } 
        else {
          leds.setPixel(xy(x,y), 0x000000);
        }
      }
    }
    leds.show();
}

void colorBright(byte c){
  for(x=0; x < matrix_width; x++){
      level = myFFT.read(lowerFFTBins[x],upperFFTBins[x]);

    for(y=0; y < 8; y++){
      if(level >= thresholdVert[y]){
        bright = 16+y*24;
      }
      else{
        break;
      }
    }
    
    for(y=0; y < 8; y++){
      if(level >= thresholdVert[y]){
        leds.setPixel(xy(x,y), WheelBright(c, bright));
      } 
      else {
        leds.setPixel(xy(x,y), 0x000000);
      }
    }
  }
  leds.show();
}

void colorPixel(byte c){
  for(x=0; x < matrix_width; x++){
    level = myFFT.read(lowerFFTBins[x],upperFFTBins[x]);
    
    for(y=0; y < 7; y++){
      if((level >= thresholdVert[y]) && (level < thresholdVert[y+1])){
        leds.setPixel(xy(x,y), 128);
      }
      else {
        leds.setPixel(xy(x,y), 0x000000);
      }
    }
  }
  leds.show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return color2color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return color2color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return color2color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

uint32_t WheelBright(byte WheelPos, byte b) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return color2color(b*(255 - WheelPos * 3)/255, 0, b*(WheelPos * 3)/255);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return color2color(0, b*(WheelPos * 3)/255, b*(255 - WheelPos * 3)/255);
  } else {
   WheelPos -= 170;
   return color2color(b*(WheelPos * 3)/255, b*(255 - WheelPos * 3)/255, 0);
  }
}

uint32_t color2color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

unsigned int xy(unsigned int x, unsigned int y) {
  return x*8+y;
}

// Run once from setup, the compute the vertical levels
void computeVerticalLevels() {
  unsigned int y;
  float n, logLevel, linearLevel;

  for (y=0; y < matrix_height; y++) {
    n = (float)y / (float)(matrix_height - 1);
    logLevel = pow10f(n * -1.0 * (dynamicRange / 20.0));
    linearLevel = 1.0 - n;
    linearLevel = linearLevel * linearBlend;
    logLevel = logLevel * (1.0 - linearBlend);
    thresholdVertical[y] = (logLevel + linearLevel) * maxLevel;
  }
}
