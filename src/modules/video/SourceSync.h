#ifndef LOVE_VIDEO_SOURCESYNC_H
#define LOVE_VIDEO_SOURCESYNC_H

// LOVE
#include "common/types.h"
#include "audio/Source.h"
#include "FrameSync.h"

namespace love::video
{

class SourceSync: public FrameSync
{
public:
	static love::Type type;

	SourceSync(love::audio::Source *source);
	~SourceSync() override = default;

	double getPosition() const override;
	love::audio::Source *getSource() const;
	void play() override;
	void pause() override;
	void seek(double time) override;
	bool isPlaying() const override;

private:
	StrongRef<love::audio::Source> source;
};

} // love::video

#endif
