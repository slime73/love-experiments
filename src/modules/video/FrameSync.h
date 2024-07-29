#ifndef LOVE_VIDEO_FRAMESYNC_H
#define LOVE_VIDEO_FRAMESYNC_H

// LOVE
#include "common/Object.h"
#include "common/types.h"

namespace love::video
{

class FrameSync: public Object
{
public:
	static love::Type type;

	virtual ~FrameSync();
	virtual double getPosition() const = 0;
	virtual void update(double dt);

	void copyState(const FrameSync *other);

	// Playback api
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void seek(double offset) = 0;
	virtual double tell() const;
	virtual bool isPlaying() const = 0;
};

} // love::video

#endif
