#ifndef LOVE_VIDEO_MOTION_NAVVIDEOSTREAM_H
#define LOVE_VIDEO_MOTION_NAVVIDEOSTREAM_H

// STL
#include <memory>
#include <mutex>

// LOVE
#include "filesystem/File.h"
#include "video/VideoStream.h"

// NAV
#include "nav/nav.h"

namespace love::video::motion
{

class NAVVideoStream: public VideoStream
{
public:
	NAVVideoStream(filesystem::File *file);
	~NAVVideoStream() override;

	const void *getFrontBuffer() const override;
	size_t getSize() const override;
	void fillBackBuffer() override;
	void threadedFillBackBuffer(double dt) override;
	bool swapBuffers() override;

	int getWidth() const override;
	int getHeight() const override;
	const std::string &getFilename() const override;
	double getDuration() const override;
	void setSync(FrameSync *frameSync) override;

	bool isPlaying() const override;

private:
	struct NAVFrame: Frame
	{
		NAVFrame();
		void set(nav_frame_t *frame = nullptr);

		double pts;
	};

	using UniqueNAVFrame = std::unique_ptr<nav_frame_t, decltype(&nav_frame_free)>;
	bool stepToBackbuffer();

	nav_input input;
	nav_t *nav;
	nav_streaminfo_t *streamInfo;
	size_t streamIndex;
	bool hasNewFrame;

	std::string filename;
	std::unique_ptr<NAVFrame> frontBuffer, backBuffer;
	std::mutex decodeMutex;
};

} // love::video::motion

#endif
