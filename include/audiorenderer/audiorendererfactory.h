#ifndef AUDIORENDERERFACTORY_H
#define AUDIORENDERERFACTORY_H

class AudioRenderer;

class AudioRendererFactory {
  public:
	enum AudioOutputType {
		OPENAL_OUTPUT,
		ALSA_OUTPUT
	};

	static AudioRenderer *create( AudioOutputType type );
};

#endif
