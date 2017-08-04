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

#include "stk/FileLoop.h"
#include "stk/RtAudio.h"

#include <signal.h>
#include <iostream>
#include <cstdlib>

using namespace stk;

// Eewww ... global variables! :-)
bool done;
StkFrames frames;
// static void finish(int ignore){ done = true; }

// This tick() function handles sample computation only.  It will be
// called automatically when the system needs a new buffer of audio
// samples.
int tick( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData )
{
  FileLoop *input = (FileLoop *) userData;
  // register StkFloat *samples = (StkFloat *) outputBuffer;
  double *samples = (double *) outputBuffer;

  /*
  for ( unsigned int i=0; i<nBufferFrames; i++ ) {
      samples[i] = input->lastOut();
      if ( input->channelsOut() == 1 ) samples[i+1] = samples[i];
//      else samples[i+1] = input->tick();
  }*/

  input->tick( frames );
  for ( unsigned int i=0; i<frames.size(); i++ ) {
    *samples++ = frames[i];
    if ( input->channelsOut() == 1 ) *samples++ = frames[i]; // play mono files in stereo
  }
  
  return 0;
}

int main(int argc, char *argv[])
{
  // Set the global sample rate before creating class instances.
  Stk::setSampleRate( (StkFloat) 44100 );

  // Initialize our WvIn and RtAudio pointers.
  RtAudio dac;
  //FileWvIn input;
  FileLoop input;

  // Try to load the soundfile.
  try {
    input.openFile( argv[1] );
  }
  catch ( StkError & ) {
    exit( 1 );
  }

  // Set input read rate based on the default STK sample rate.
  double rate = 1.0;
  //rate = input.getFileRate() / Stk::sampleRate();
  input.setRate( rate );

  // input.ignoreSampleRateChange();

  // Find out how many channels we have.
  int channels = input.channelsOut();

  // Figure out how many bytes in an StkFloat and setup the RtAudio stream.
  RtAudio::StreamParameters parameters;
  // parameters.deviceId = dac.getDefaultOutputDevice();
  parameters.deviceId = 2;
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
