#include "stk/FileWvIn.h"
#include "stk/RtAudio.h"
#include "stk/PitShift.h"
#include "stk/Echo.h"

#include <iostream>
#include <signal.h>
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <cmath>

using namespace stk;

std::mutex i2cData, effectLock;
int distortionFactor;

bool done;
StkFrames frames;
PitShift shifter;
Echo echo;
static void finish(int ignore){ done = true; }

void i2cConnection()
{
    // Create I2C bus
    int file;
    char *bus = "/dev/i2c-1";
    if((file = open(bus, O_RDWR)) < 0) {
        printf("Failed to open the bus. \n");
        exit(1);
    }
    
    // Get I2C device, MMA8452 I2C address is dx10(1d)
    int addr = 0x1d;
    if(ioctl(file, I2C_SLAVE, addr) < 0) {
        printf("Failed to connect to I2C device. \n");
    }

    // Select mode register(0x2A)
	// Standby mode(0x00)
	char config[2] = {0};
	config[0] = 0x2A;
	config[1] = 0x00;
	write(file, config, 2);

	// Select mode register(0x2A)
	// Active mode(0x01)
	config[0] = 0x2A;
	config[1] = 0x01;
	write(file, config, 2);
	
	// Select configuration register(0x0E)
	// Set range to +/- 2g(0x00)
	config[0] = 0x0E;
	config[1] = 0x00;
	write(file, config, 2);
	sleep(0.5);

    for(;;){
        // Read data from I2C
        char reg[1] = {0x00};
        write(file, reg, 1);
        int length = 7;
        char data[7] = {0};
        if (read(file, data, length) != length) {
            printf("I/O error. \n");
        } else {
            int xAccl = ((data[1] * 256) + data[2]) / 16;
            if(xAccl > 2047) {
                xAccl -= 4096;
            }

            int yAccl = ((data[3] * 256) + data[4]) / 16;
            if(yAccl > 2047) {
                yAccl -= 4096;
            }

            int zAccl = ((data[5] * 256) + data[6]) / 16;
            if(zAccl > 2047) {
                zAccl -= 4096;
            }

            // printf("Acceleration in X/Y/Z-Axis : %d %d %d \n", xAccl, yAccl, zAccl);
            i2cData.lock();
            distortionFactor = xAccl + yAccl + (zAccl-1000);
            i2cData.unlock();
        }
    }
}

void audioProcessing() {
//    double prevShift = 0;
    for(;;) {
        i2cData.lock();
        double effectShift = (double)distortionFactor/1000+1;
        printf("Distortion Factor: %d, Scaled: %f \n", distortionFactor, effectShift);
        i2cData.unlock();

//        if(abs(effectShift - prevShift) > 0) {
//            prevShift = effectShift;
            effectLock.lock();
            shifter.setShift(effectShift);
            echo.setDelay( (unsigned long) (Stk::sampleRate() * effectShift) );
            echo.setEffectMix(effectShift);
            effectLock.unlock();
//        }
    }
};

// This tick() function handles sample computation only.  It will be
// called automatically when the system needs a new buffer of audio
// samples.
int tick( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData )
{
    FileWvIn *input = (FileWvIn *) userData;
    register StkFloat *samples = (StkFloat *) outputBuffer;
    register StkFloat shiftSamples;

    input->tick(frames);
    
    effectLock.lock();
    for ( unsigned int i=0; i<frames.size(); i++ ) {
        shiftSamples = shifter.tick(frames[i]);
        *samples++ = shiftSamples;
        if ( input->channelsOut() == 1 ) *samples++ = shiftSamples; // play mono files in stereo
    }
    effectLock.unlock();

    if ( input->isFinished() ) {
        done = true;
        return 1;
    }
    else
        return 0;
}

void startMusic()
{
    // Initialize our WvIn and RtAudio pointers.
    RtAudio dac;
    FileWvIn input;

    // Try to load the soundfile.
    try {
      input.openFile("note.wav");
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
  
    effectLock.lock();
    shifter.setShift(1.0);
    shifter.setEffectMix(1.0);
    echo.setDelay( (unsigned long) (Stk::sampleRate() * 0.95) );
    echo.setEffectMix(1.0);
    effectLock.unlock();

    // Figure out how many bytes in an StkFloat and setup the RtAudio stream.
    RtAudio::StreamParameters parameters;
    // parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.deviceId = 2;

    std::printf("Channels: %d \n", channels);
    std::printf("Default output device: %d \n", parameters.deviceId);
    std::printf("Number of audio devices: %d \n", dac.getDeviceCount());

    for (unsigned int i = 0; i < dac.getDeviceCount(); i++) {
        RtAudio::DeviceInfo info = dac.getDeviceInfo(i);
        std::printf("Device: %d, Name: %s \n", i, info.name.c_str());
    }
  
  
    parameters.nChannels = ( channels == 1 ) ? 2 : channels; //  Play mono files as stereo.
    RtAudioFormat format = ( sizeof(StkFloat) == 8 ) ? RTAUDIO_FLOAT64 : RTAUDIO_FLOAT32;
    unsigned int bufferFrames = RT_BUFFER_SIZE;
    try {
      dac.openStream( &parameters, NULL, format, (unsigned int)Stk::sampleRate(), &bufferFrames, &tick, (void *)&input );
    }
    catch ( RtAudioError &error ) {
      error.printMessage();
      return;
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
      return;
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
}

int main()
{
    std::thread t1(i2cConnection);
    std::thread t2(audioProcessing);
    std::thread t3(startMusic);

    // std::cout << "main thread" << std::endl;
    t1.join();
    t2.join();
    t3.join();
    return 0;
}
