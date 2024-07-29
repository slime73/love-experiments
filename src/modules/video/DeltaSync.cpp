#include "DeltaSync.h"

namespace love::video
{

love::Type DeltaSync::type("DeltaSync", &FrameSync::type);

DeltaSync::DeltaSync(bool manual)
: playing(false)
, manual(manual)
, position(0)
, speed(1)
{
}

DeltaSync::~DeltaSync()
{
}

double DeltaSync::getPosition() const
{
	return position;
}

void DeltaSync::update(double dt)
{
	love::thread::Lock l(mutex);
	if (playing && !manual)
		position += dt*speed;
}

void DeltaSync::updateManual(double dt)
{
	if (manual)
		position += dt*speed;
}

bool DeltaSync::isAutomatic() const
{
	return !manual;
}

void DeltaSync::play()
{
	playing = true;
}

void DeltaSync::pause()
{
	playing = false;
}

void DeltaSync::seek(double time)
{
	thread::Lock l(mutex);
	position = time;
}

bool DeltaSync::isPlaying() const
{
	return playing;
}

} // love::video
