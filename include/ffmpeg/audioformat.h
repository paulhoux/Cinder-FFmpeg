#pragma once

namespace ffmpeg {

struct AudioFormat {
	unsigned int bits;
	unsigned int rate;
	unsigned int numChannels;
	unsigned int framesPerPacket;
};

}
