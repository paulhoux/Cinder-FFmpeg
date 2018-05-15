#include "ffmpeg/audiorendererfactory.h"
#include "ffmpeg/openalrenderer.h"

#include <stdexcept>

namespace ffmpeg {

AudioRenderer *AudioRendererFactory::create( AudioOutputType type )
{
	switch( type ) {
	case OPENAL_OUTPUT:
		return new OpenAlRenderer();
	default:
		throw std::logic_error( "AudioRendererFactory: Unsupported audio output type provided" );
	}
}

} // namespace ffmpeg
