#include "NAVDecoder.h"

// STL
#include <algorithm>
#include <memory>
#include <string>

#include "common/Exception.h"
#include "common/navinput.h"
#include "filesystem/File.h"

namespace love
{

template<typename T>
int16 reduceBits(const uint8_t *buf)
{
	constexpr size_t byteSize = sizeof(T);
	T value = T();
	std::copy(buf, buf + byteSize, (uint8_t *) &value);

	if constexpr (std::is_floating_point<T>::value)
		return (int16) (std::clamp(value, T(-1), T(1)) * 32767.0f);
	else
		return value >> ((byteSize - sizeof(int16)) * 8LL);
}

namespace sound::lullaby
{

using UniqueNAVFrame = std::unique_ptr<nav_frame_t, decltype(&nav_frame_free)>;

NAVDecoder::NAVDecoder(Stream *stream, int bufsize)
: Decoder(stream, bufsize)
, input(streamToNAVInput(stream))
, nav(nullptr)
, streamInfo(nullptr)
, streamIndex(0)
{
	// If possible, get the filename.
	std::string filename;
	if (love::filesystem::File *streamAsFile = dynamic_cast<love::filesystem::File*>(stream))
		filename = streamAsFile->getFilename();

	// Open NAV
	nav = nav_open(&input, filename.empty() ? nullptr : filename.c_str(), nullptr);
	if (!nav)
	{
		std::string err = nav_error();
		input.closef();
		throw love::Exception("NAV Error: %s", err);
	}

	// Only activate first audio stream
	size_t streams = nav_nstreams(nav);

	for (size_t i = 0; i < streams; i++)
	{
		if (streamInfo)
			nav_stream_enable(nav, i, false);
		else
		{
			streamInfo = nav_stream_info(nav, i);
			if (nav_streaminfo_type(streamInfo) == NAV_STREAMTYPE_AUDIO)
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
		throw love::Exception("No audio stream found");
	}

	if (NAV_AUDIOFORMAT_BITSIZE(nav_audio_format(streamInfo)) % 8 != 0)
	{
		nav_close(nav);
		throw love::Exception("Unsupported bit depth not divisible by 8");
	}
}

NAVDecoder::~NAVDecoder()
{
	if (nav)
		nav_close(nav); // automatically calls input.closef()
	else if (input.userdata)
		input.closef();
}

love::sound::Decoder *NAVDecoder::clone()
{
	return new NAVDecoder(stream->clone(), bufferSize);
}

int NAVDecoder::decode()
{
	int byteDepth = getBitDepthRaw() / 8;
	int channelCount = getChannelCount();
	int frameSize = byteDepth * channelCount;

	// Note: bufferSize is in bytes, not in sample frames.
	int maxFrames = bufferSize / frameSize;
	int filledFrames = 0;

	while (filledFrames < maxFrames)
	{
		int neededFrames = maxFrames - filledFrames;
		uint8_t *bufferToFill = ((uint8_t*) buffer) + filledFrames * frameSize;

		if (!queuedBuffers.empty())
		{
			// Poll from buffers first
			std::vector<uint8_t> &qbuf = queuedBuffers.front();
			size_t framesInBuffer = qbuf.size() / frameSize;

			if (framesInBuffer > neededFrames)
			{
				// Copy then resize the qbuf.
				std::copy(qbuf.begin(), qbuf.begin() + neededFrames * frameSize, bufferToFill);
				std::copy(qbuf.begin() + neededFrames * frameSize, qbuf.end(), qbuf.begin());
				qbuf.resize((framesInBuffer - neededFrames) * frameSize);
				filledFrames += neededFrames;
			}
			else
			{
				// Copy then pop
				std::copy(qbuf.begin(), qbuf.end(), bufferToFill);
				queuedBuffers.pop_front(); // Warning: qbuf is no longer valid
				filledFrames += framesInBuffer;
			}
		}

		if (queuedBuffers.empty() && !refillBuffers(-1))
			// EOF
			break;
	}

	return filledFrames * frameSize;
}

int NAVDecoder::getBitDepthRaw() const
{
	return NAV_AUDIOFORMAT_BITSIZE(nav_audio_format(streamInfo));
}

bool NAVDecoder::refillBuffers(int64 target)
{
	int bitDepth = getBitDepthRaw();

	while (UniqueNAVFrame frame{nav_read(nav), nav_frame_free})
	{
		// In case certain stream didn't respect stream enablement
		if (nav_frame_streamindex(frame.get()) != streamIndex)
			continue;

		const uint8_t *navFrameBuffer = (const uint8_t*) nav_frame_buffer(frame.get());
		size_t navFrameSize = nav_frame_size(frame.get());

		if (target != -1)
		{
			int frameSizeBytes = (bitDepth / 8) * getChannelCount();
			int64 navFrameCount = (int64) (navFrameSize / frameSizeBytes);
			int64 startSample = (int64) (nav_frame_tell(frame.get()) * getSampleRate());

			if ((startSample + navFrameCount) < target)
				// Need to seek more
				continue;

			int64 offFrame = startSample + navFrameCount - target;
			size_t offSizeBytes = (startSample + navFrameCount - target) * frameSizeBytes;
			navFrameBuffer += offSizeBytes;
			navFrameSize -= offSizeBytes;
		}

		if (bitDepth <= 16)
			// For 8-bit and 16-bit, no conversion
			queuedBuffers.emplace_back(navFrameBuffer, navFrameBuffer + navFrameSize);
		else
			// For anything else, perform conversion
			queuedBuffers.push_back(reduceBitDepth(navFrameBuffer, navFrameSize, nav_audio_format(streamInfo)));

		return true;
	}

	if (nav_error() == nullptr)
		eof = true;

	return false;
}

std::vector<uint8_t> NAVDecoder::reduceBitDepth(const uint8_t *buf, size_t size, nav_audioformat format)
{
	int byteSize = NAV_AUDIOFORMAT_BYTESIZE(format);
	size_t iterCount = size / byteSize;

	switch (byteSize)
	{
		// TODO: Handle big endian and signedness
		case 1:
		case 2:
		case 3:
		default:
			// TODO
			break;
		case 4:
		{
			std::vector<uint8_t> newbuf(iterCount * sizeof(int16));

			if (NAV_AUDIOFORMAT_ISFLOAT(format))
			{
				for (size_t i = 0; i < iterCount; i++)
				{
					int16 intval = reduceBits<float>(buf + i * byteSize);
					std::copy((uint8_t*) &intval, ((uint8_t*) &intval) + sizeof(int16), newbuf.begin() + i * sizeof(int16));
				}
			}
			else
			{
				for (size_t i = 0; i < iterCount; i++)
				{
					int16 intval = reduceBits<int32>(buf + i * byteSize);
					std::copy((uint8_t*) &intval, ((uint8_t*) &intval) + sizeof(int16), newbuf.begin() + i * sizeof(int16));
				}
			}

			return newbuf;
		}
		case 8:
		{
			std::vector<uint8_t> newbuf(iterCount * sizeof(int16));

			if (NAV_AUDIOFORMAT_ISFLOAT(format))
			{
				for (size_t i = 0; i < iterCount; i++)
				{
					int16 intval = reduceBits<double>(buf + i * byteSize);
					std::copy((uint8_t*) &intval, ((uint8_t*) &intval) + sizeof(int16), newbuf.begin() + i * sizeof(int16));
				}
			}
			else
			{
				for (size_t i = 0; i < iterCount; i++)
				{
					int16 intval = reduceBits<int64>(buf + i * byteSize);
					std::copy((uint8_t*) &intval, ((uint8_t*) &intval) + sizeof(int16), newbuf.begin() + i * sizeof(int16));
				}
			}

			return newbuf;
		}
	}

	// No conversion
	return std::vector<uint8_t>(buf, buf + size);
}

bool NAVDecoder::seek(double s)
{
	if (nav_seek(nav, s) != -1.0)
	{
		// Flush buffers
		queuedBuffers.clear();
		// Un EOF
		eof = false;
		// Do tiny seek
		return refillBuffers((int64) (s * getSampleRate()));
	}

	return false;
}

bool NAVDecoder::rewind()
{
	return seek(0.0);
}

bool NAVDecoder::isSeekable()
{
	// NAV requires seeking
	return true;
}

int NAVDecoder::getChannelCount() const
{
	return nav_audio_nchannels(streamInfo);
}

int NAVDecoder::getBitDepth() const
{
	// Note: LOVE doesn't support bit depth > 16 bits. We'll downsample it.
	return std::min<int>(getBitDepthRaw(), 16);
}

int NAVDecoder::getSampleRate() const
{
	return nav_audio_sample_rate(streamInfo);
}

double NAVDecoder::getDuration()
{
	return nav_duration(nav);
}

} // sound::lullaby
} // love
