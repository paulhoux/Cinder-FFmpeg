#include "audiorenderer/audiorendererfactory.h"
#include "audiorenderer/openalrenderer.h"

#include <stdexcept>

AudioRenderer *AudioRendererFactory::create( AudioOutputType type )
{
	switch( type ) {
	case OPENAL_OUTPUT:
		return new OpenAlRenderer();
		break;
	default:
		throw std::logic_error( "AudioRendererFactory: Unsupported audio output type provided" );
	}
}
