#pragma once

namespace ffmpeg {

class AudioRenderer;

class AudioRendererFactory {
  public:
	enum AudioOutputType {
		OPENAL_OUTPUT,
		CINDER_OUTPUT
	};

	static AudioRenderer *create( AudioOutputType type );
};

} // namespace ffmpeg
