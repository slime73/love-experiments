#ifndef LOVE_SOUND_LULLABY_NAV_DECODER_H
#define LOVE_SOUND_LULLABY_NAV_DECODER_H

// STL
#include <deque>
#include <vector>

// LOVE
#include "common/Stream.h"
#include "sound/Decoder.h"

// nav
#include "nav/nav.h"

namespace love
{
namespace sound
{
namespace lullaby
{

class NAVDecoder: public love::sound::Decoder
{
public:
	NAVDecoder(Stream *stream, int bufsize);
	virtual ~NAVDecoder();

	love::sound::Decoder *clone() override;
	int decode() override;
	bool seek(double s) override;
	bool rewind() override;
	bool isSeekable() override;
	int getChannelCount() const override;
	int getBitDepth() const override;
	int getSampleRate() const override;
	double getDuration() override;

private:
	nav_input input;
	nav_t *nav;
	nav_streaminfo_t *streamInfo;
	size_t streamIndex;

	std::deque<std::vector<uint8_t>> queuedBuffers;

	int getBitDepthRaw() const;
	bool refillBuffers(int64 targetDuration);
	std::vector<uint8_t> reduceBitDepth(const uint8_t *buf, size_t size, nav_audioformat format);
};

} // lullaby
} // sound
} // love

#endif
