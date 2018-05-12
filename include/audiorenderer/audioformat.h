#ifndef AUDIOFORMAT_H
#define AUDIOFORMAT_H

struct AudioFormat {
	unsigned int bits;
	unsigned int rate;
	unsigned int numChannels;
	unsigned int framesPerPacket;
};

#endif
