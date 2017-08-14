#include "Lib/RTIMULib.h"

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
double distortionFactor;

bool done;
StkFrames frames;
PitShift shifter;
Echo echo;
static void finish(int ignore){ done = true; }

int i2cConnection()
{
    int sampleCount = 0;
    int sampleRate = 0;
    uint64_t rateTimer;
    uint64_t displayTimer;
    uint64_t now;

    RTIMUSettings *settings = new RTIMUSettings("RTIMULib");

    RTIMU *imu = RTIMU::createIMU(settings);

    if ((imu == NULL) || (imu->IMUType() == RTIMU_TYPE_NULL)) {
        printf("No IMU found\n");
        exit(1);
    }

    //  set up IMU
    imu->IMUInit();

    //  this is a convenient place to change fusion parameters
    imu->setSlerpPower(0.02);
    imu->setGyroEnable(true);
    imu->setAccelEnable(true);
    imu->setCompassEnable(true);

    //  set up for rate timer
    rateTimer = displayTimer = RTMath::currentUSecsSinceEpoch();

    //  now just process data
    while (1) {
        //  poll at the rate recommended by the IMU
        usleep(imu->IMUGetPollInterval() * 1000);

        while (imu->IMURead()) {
            RTIMU_DATA imuData = imu->getIMUData();
            sampleCount++;

            now = RTMath::currentUSecsSinceEpoch();

            //  display 10 times per second
            if ((now - displayTimer) > 100000) {
                printf("Sample rate %d: %s\r", sampleRate, RTMath::displayDegrees("", imuData.fusionPose));
                fflush(stdout);

                double x = imuData.fusionPose.x() * RTMath::RTMATH_RAD_TO_DEGREE;
                double y = imuData.fusionPose.y() * RTMath::RTMATH_RAD_TO_DEGREE;
                double z = imuData.fusionPose.z() * RTMath::RTMATH_RAD_TO_DEGREE;

                distortionFactor = x/30.0;

                displayTimer = now;
            }

            //  update rate every second
            if ((now - rateTimer) > 1000000) {
                sampleRate = sampleCount;
                sampleCount = 0;
                rateTimer = now;
            }
        }
    }
}

void audioProcessing() {
//    double prevShift = 0;
    for(;;) {
        i2cData.lock();
        double effectShift = distortionFactor+1;
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
