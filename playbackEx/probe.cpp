// probe.cpp

#include <iostream>
#include "stk/RtAudio.h"

int main()
{
  RtAudio *audio = 0;

  // Default RtAudio constructor
  try {
    audio = new RtAudio();
  }
  catch (RtError &error) {
    error.printMessage();
    exit(EXIT_FAILURE);
  }

  // Determine the number of devices available
  int devices = audio->getDeviceCount();

  // Scan through devices for various capabilities
  RtAudioDeviceInfo info;
  for (int i=1; i<=devices; i++) {

    try {
      info = audio->getDeviceInfo(i);
    }
    catch (RtError &error) {
      error.printMessage();
      break;
    }

    // Print, for example, the maximum number of output channels for each device
    std::cout << "device = " << i;
    std::cout << ": maximum output channels = " << info.outputChannels << "\n";
  }

  // Clean up
  delete audio;

  return 0;
}
