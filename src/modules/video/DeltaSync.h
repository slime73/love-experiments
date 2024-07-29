#ifndef LOVE_VIDEO_DELTASYNC_H
#define LOVE_VIDEO_DELTASYNC_H

// LOVE
#include "common/types.h"
#include "thread/threads.h"
#include "FrameSync.h"

namespace love::video
{

class DeltaSync : public FrameSync
{
public:
	static love::Type type;

	DeltaSync(bool manual);
	~DeltaSync();

	double getPosition() const override;
	void update(double dt) override;
	void updateManual(double dt);
	bool isAutomatic() const;

	void play() override;
	void pause() override;
	void seek(double time) override;
	bool isPlaying() const override;

private:
	bool playing;
	bool manual;
	double position;
	double speed;
	love::thread::MutexRef mutex;
};

} // love::video

#endif