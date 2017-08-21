/******************************************/
/*
  Adapted from stk examples program 

*/
/******************************************/

#include "stk/RtWvIn.h"
#include "stk/FileWvOut.h"
#include <cstdlib>

#include <iostream>
#include <thread>
#include <mutex>

using namespace stk;

std::mutex recordLock;
bool recordState;

void keyboardInterface() {
    recordLock.lock();
    recordState = false;
    recordLock.unlock();

    std::cout << "Press enter to start recording." << std::endl;
    std::cin.ignore();

    recordLock.lock();
    recordState = true;
    recordLock.unlock();

    std::cout << "Press enter to STOP recording." << std::endl;
    std::cin.ignore();

    recordLock.lock();
    recordState = false;
    recordLock.unlock();
}

int main()
{
    unsigned int channels = 1;
    StkFrames frame(1, channels);

    // Set the global sample rate.
    Stk::setSampleRate(44100.0);

    // Initialize our WvIn/WvOut pointers.
    RtWvIn *input = 0;
    FileWvOut *output = 0;

    // Open the realtime input device
    try {
      input = new RtWvIn(channels);
    }
    catch(StkError &) {
      exit(1);
    }

    while(true){
        recordLock.lock();
        if(recordState) break;
        recordLock.unlock();
    }

    // Open the soundfile for output.
    try {
        output = new FileWvOut( "song", channels, FileWrite::FILE_WAV );
    }
    catch (StkError &) {
        goto cleanup;
    }

    // Here's the runtime loop
    std::thread t1(keyboardInterface);

    while(true){
        output->tick( input->tick(frame) );
        recordLock.lock();
        if(!recordState) break;
        recordLock.unlock();
    }

    t1.join();
  
    cleanup:
    delete input;
    delete output;
    return 0;
}
