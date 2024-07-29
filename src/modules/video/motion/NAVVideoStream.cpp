// STL/CRT
#include <cassert>

// LOVE
#include "common/Exception.h"
#include "common/navinput.h"
#include "video/DeltaSync.h"
#include "NAVVideoStream.h"

namespace love::video::motion
{

NAVVideoStream::NAVVideoStream(filesystem::File *file)
: input(streamToNAVInput(file))
, nav(nullptr)
, streamInfo(nullptr)
, streamIndex(0)
, position(0)
, hasNewFrame(false)
, filename(file->getFilename())
, frontBuffer(new NAVFrame())
, backBuffer(new NAVFrame())
, decodeMutex()
{
	// Open NAV
	nav = nav_open(&input, filename.c_str(), nullptr);
	if (!nav)
	{
		std::string err = nav_error();
		throw love::Exception("NAV Error: %s", err.c_str());
	}

	// Only activate first video stream
	size_t streams = nav_nstreams(nav);

	for (size_t i = 0; i < streams; i++)
	{
		if (streamInfo)
			nav_stream_enable(nav, i, false);
		else
		{
			streamInfo = nav_stream_info(nav, i);
			if (nav_streaminfo_type(streamInfo) == NAV_STREAMTYPE_VIDEO)
			{
				nav_stream_enable(nav, i, true);
				streamIndex = i;
				// no break, we want to disable the rest of the stream
			}
			else
				streamInfo = nullptr;
		}
	}

	if (!streamInfo)
	{
		nav_close(nav);
		throw love::Exception("No video stream found");
	}

	// Verify the pixel format
	switch (nav_video_pixel_format(streamInfo))
	{
		default:
			nav_close(nav);
			throw love::Exception("Unsupported pixel format");
		case NAV_PIXELFORMAT_YUV420:
		case NAV_PIXELFORMAT_YUV444:
		case NAV_PIXELFORMAT_NV12:
			break;
	}

	// Let's fill the frames
	if (!stepToBackbuffer())
		throw love::Exception("Cannot get first frame");
	frontBuffer.swap(backBuffer);
	stepToBackbuffer();

	frameSync.set(new DeltaSync(false), Acquire::NORETAIN);
}

NAVVideoStream::~NAVVideoStream()
{
	if (nav)
		nav_close(nav);
	else if (input.userdata)
		input.closef();
}

const void *NAVVideoStream::getFrontBuffer() const
{
	return frontBuffer.get();
}

size_t NAVVideoStream::getSize() const
{
	return sizeof(Frame);
}

void NAVVideoStream::fillBackBuffer()
{
}

void NAVVideoStream::threadedFillBackBuffer(double dt)
{
	std::lock_guard<std::mutex> _lock(decodeMutex);
	frameSync->update(dt);

	double newPos = frameSync->tell();
	if (newPos < position)
	{
		// Seeking backward
		nav_seek(nav, newPos);
		backBuffer->set();
	}

	while (newPos > backBuffer->pts)
	{
		if (!stepToBackbuffer())
			// Yeah it's not worth it.
			return;
	}

	position = newPos;
}

bool NAVVideoStream::swapBuffers()
{
	std::lock_guard<std::mutex> _lock(decodeMutex);

	double position = frameSync->tell();

	if (hasNewFrame)
	{
		frontBuffer.swap(backBuffer);
		hasNewFrame = false;
		return true;
	}

	return false;
}

int NAVVideoStream::getWidth() const
{
	uint32_t width, height;
	nav_video_dimensions(streamInfo, &width, &height);
	return width;
}

int NAVVideoStream::getHeight() const
{
	uint32_t width, height;
	nav_video_dimensions(streamInfo, &width, &height);
	return height;
}

const std::string &NAVVideoStream::getFilename() const
{
	return filename;
}

double NAVVideoStream::getDuration() const
{
	return nav_duration(nav);
}

void NAVVideoStream::setSync(FrameSync *frameSync)
{
	std::lock_guard<std::mutex> _lock(decodeMutex);
	this->frameSync = frameSync;
}

bool NAVVideoStream::isPlaying() const
{
	return frameSync->isPlaying();
}

bool NAVVideoStream::stepToBackbuffer()
{
	while (UniqueNAVFrame frame{nav_read(nav), nav_frame_free})
	{
		// In case certain stream didn't respect stream enablement
		if (nav_frame_streamindex(frame.get()) != streamIndex)
			continue;

		backBuffer->set(frame.get());
		hasNewFrame = true;
		return true;
	}

	return false;
}

NAVVideoStream::NAVFrame::NAVFrame()
: Frame()
, pts(-1)
{
}

void NAVVideoStream::NAVFrame::set(nav_frame_t *frame)
{
	delete[] yplane; yplane = nullptr;
	delete[] cbplane; cbplane = nullptr;
	delete[] crplane; crplane = nullptr;
	pts = -1;

	if (frame)
	{
		nav_streaminfo_t *streamInfo = nav_frame_streaminfo(frame);
		assert(nav_streaminfo_type(streamInfo) == NAV_STREAMTYPE_VIDEO);

		uint32_t width, height;
		nav_video_dimensions(streamInfo, &width, &height);
		const uint8_t *frameBuffer = (const uint8_t*) nav_frame_buffer(frame);

		yw = (int) width;
		yh = (int) height;
		pts = nav_frame_tell(frame);

		switch (nav_video_pixel_format(streamInfo))
		{
			case NAV_PIXELFORMAT_YUV420:
			{
				cw = (width + 1) / 2;
				ch = (height + 1) / 2;
				size_t ysize = ((size_t) width) * height;
				size_t uvsize = ((size_t) cw) * ch;
				yplane = new unsigned char[ysize];
				cbplane = new unsigned char[uvsize];
				crplane = new unsigned char[uvsize];
				memcpy(yplane, frameBuffer, ysize);
				memcpy(cbplane, frameBuffer + ysize, uvsize);
				memcpy(crplane, frameBuffer + ysize + uvsize, uvsize);
				break;
			}
			case NAV_PIXELFORMAT_YUV444:
			{
				cw = yw;
				ch = yh;
				size_t size = ((size_t) width) * height;
				yplane = new unsigned char[size];
				cbplane = new unsigned char[size];
				crplane = new unsigned char[size];
				memcpy(yplane, frameBuffer, size);
				memcpy(cbplane, frameBuffer + size, size);
				memcpy(crplane, frameBuffer + size * 2, size);
				break;
			}
			case NAV_PIXELFORMAT_NV12:
			{
				cw = (width + 1) / 2;
				ch = (height + 1) / 2;
				size_t ysize = ((size_t) width) * height;
				size_t uvsize = ((size_t) cw) * ch;
				yplane = new unsigned char[ysize];
				cbplane = new unsigned char[uvsize];
				crplane = new unsigned char[uvsize];
				memcpy(yplane, frameBuffer, ysize);

				const uint8_t *uvBuffer = frameBuffer + ysize;

				// Need to iterate UV manually.
				for (size_t i = 0; i < uvsize; i++)
				{
					cbplane[i] = uvBuffer[i * 2 + 0];
					crplane[i] = uvBuffer[i * 2 + 1];
				}
				break;
			}
			default:
			{
				yplane = new unsigned char[1];
				cbplane = new unsigned char[1];
				crplane = new unsigned char[1];
				yw = 1;
				yh = 1;
				cw = 1;
				ch = 1;
				break;
			}
		}
	}
}

} // love::video::motion
