// LOVE
#include "common/Object.h"
#include "SourceSync.h"

namespace love::video
{

love::Type SourceSync::type("SourceSync", &FrameSync::type);

SourceSync::SourceSync(love::audio::Source *source)
: source(source)
{
}

double SourceSync::getPosition() const
{
	return source->tell(love::audio::Source::UNIT_SECONDS);
}

audio::Source *SourceSync::getSource() const
{
	return source.get();
}

void SourceSync::play()
{
	source->play();
}

void SourceSync::pause()
{
	source->pause();
}

void SourceSync::seek(double time)
{
	source->seek(time, love::audio::Source::UNIT_SECONDS);
}

bool SourceSync::isPlaying() const
{
	return source->isPlaying();
}


} // love::video