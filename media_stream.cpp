#include "media_stream.h"

MediaStream::MediaStream(): sinks_(), audioSSRC_(rand()), videoSSRC_(rand()) {

}
