/******************************************/
/*
  Example program to play an N channel
  soundfile.

  This program will load WAV, SND, AIF, and
  MAT-file formatted files of various data
  types.  If the audio system does not support
  the number of channels or sample rate of
  the soundfile, the program will stop.

  By Gary P. Scavone, 2000 - 2004.
*/
/******************************************/

#include "stk/FileWvIn.h"
#include "stk/RtAudio.h"
#include "stk/LentPitShift.h"

#include <signal.h>
#include <iostream>
#include <cstdlib>

using namespace stk;

// Eewww ... global variables! :-)
bool done;
StkFrames frames;
LentPitShift shifter;
static void finish(int ignore){ done = true; }

// This tick() function handles sample computation only.  It will be
// called automatically when the system needs a new buffer of audio
// samples.
int tick( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData )
{
  FileWvIn *input = (FileWvIn *) userData;
  register StkFloat *samples = (StkFloat *) outputBuffer;
  // register StkFloat rSamples;

  input->tick(frames);
  // shifter.tick(frames);

  for (unsigned int i = 0; i < frames.channels(); i++) {
  //   input->tick(frames, i);
    shifter.tick(frames, i);
  }
  
  for ( unsigned int i=0; i<frames.size(); i++ ) {
    *samples++ = frames[i];
    if ( input->channelsOut() == 1 ) *samples++ = frames[i]; // play mono files in stereo
  }
  
  if ( input->isFinished() ) {
    done = true;
    return 1;
  }
  else
    return 0;
}

int main(int argc, char *argv[])
{
  // Initialize our WvIn and RtAudio pointers.
  RtAudio dac;
  FileWvIn input;

  // Try to load the soundfile.
  try {
    input.openFile("song.wav");
  }
  catch ( StkError & ) {
    exit( 1 );
  }

  // Set default sample rate
  Stk::setSampleRate( 44100.0 );

  // Set default playback rate
  double rate = 1.0;
  input.setRate( rate );

  input.ignoreSampleRateChange();

  // Find out how many channels we have.
  int channels = input.channelsOut();

  shifter.setShift(2.0);

  // Figure out how many bytes in an StkFloat and setup the RtAudio stream.
  RtAudio::StreamParameters parameters;
  parameters.deviceId = dac.getDefaultOutputDevice();
  parameters.nChannels = ( channels == 1 ) ? 2 : channels; //  Play mono files as stereo.
  RtAudioFormat format = ( sizeof(StkFloat) == 8 ) ? RTAUDIO_FLOAT64 : RTAUDIO_FLOAT32;
  unsigned int bufferFrames = RT_BUFFER_SIZE;
  try {
    dac.openStream( &parameters, NULL, format, (unsigned int)Stk::sampleRate(), &bufferFrames, &tick, (void *)&input );
  }
  catch ( RtAudioError &error ) {
    error.printMessage();
    goto cleanup;
  }

  // Install an interrupt handler function.
  (void) signal(SIGINT, finish);

  // Resize the StkFrames object appropriately.
  frames.resize( bufferFrames, channels );

  try {
    dac.startStream();
  }
  catch ( RtAudioError &error ) {
    error.printMessage();
    goto cleanup;
  }

  // Block waiting until callback signals done.
  while ( !done )
    Stk::sleep( 100 );
  
  // By returning a non-zero value in the callback above, the stream
  // is automatically stopped.  But we should still close it.
  try {
    dac.closeStream();
  }
  catch ( RtAudioError &error ) {
    error.printMessage();
  }

 cleanup:
  return 0;
}
