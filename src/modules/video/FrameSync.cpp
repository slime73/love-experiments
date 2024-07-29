#include "common/Object.h"
#include "FrameSync.h"

namespace love::video
{

love::Type FrameSync::type("FrameSync", &Object::type);

FrameSync::~FrameSync()
{
}

void FrameSync::update(double dt)
{
}

void FrameSync::copyState(const FrameSync *other)
{
	seek(other->tell());
	if (other->isPlaying())
		play();
	else
		pause();
}

double FrameSync::tell() const
{
	return getPosition();
}

} // love::video
