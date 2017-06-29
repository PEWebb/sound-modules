// rtex2.cpp
//
// Realtime audio input/output example with comb filter using callback functionality.
#include "stk/RtAudio.h"
#include <cstdlib>
#include "stk/Delay.h"
using namespace stk;

Delay delay;

int comb( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
			double, RtAudioStreamStatus, void * )

{

	// Cast the buffers to the correct data type.
	double *idata = (double *) inputBuffer;
	double *odata = (double *) outputBuffer;
	// We know we only have 1 sample per frame here.
	for ( int i=0; i<nBufferFrames; i++ ) {
		odata[i] = idata[i] + 0.98 * delay.lastOut(); // feedback comb
//    odata[i] = idata[i] + delay.tick( idata[i] ); // feedforward comb
		odata[i] *= 0.45;
		delay.tick( odata[i] );  // feedback comb
	}

	return 0;
}

int main()
{
	unsigned int nBufferFrames = 256;  // 256 sample frames
	unsigned int sampleRate = 44100;
	unsigned int nChannels = 1;
	RtAudio adac;
	delay.setDelay( 1000 );

	// Open the default realtime output device.
	RtAudio::StreamParameters oParameters, iParameters;
	oParameters.deviceId = adac.getDefaultOutputDevice();
	iParameters.deviceId = adac.getDefaultInputDevice();
	oParameters.nChannels = nChannels;
	iParameters.nChannels = nChannels;

	try {
		adac.openStream( &oParameters, &iParameters, RTAUDIO_FLOAT64, sampleRate, &nBufferFrames, &comb );
	}
	catch ( RtAudioError &error ) {
		error.printMessage();
		exit( EXIT_FAILURE );
	}

	try {
		adac.startStream();
	}
	catch ( RtAudioError &error ) {
		error.printMessage();
		exit( EXIT_FAILURE );
	}

	char input;
	std::cin.get( input );

	// Stop the stream.
	try {
		adac.stopStream();
	}

	catch ( RtAudioError &error ) {
		error.printMessage();
	}
	return 0;
}
