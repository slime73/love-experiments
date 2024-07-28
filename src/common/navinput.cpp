#include "navinput.h"

#include "Exception.h"

namespace love
{

static void closeStream(void **udata)
{
	Stream *stream = *(Stream **) udata;
	stream->release();
	*udata = nullptr;
}

static size_t readStream(void *udata, void *dest, size_t size)
{
	Stream *stream = (Stream *) udata;
	return (size_t) stream->read(dest, (int64) size);
}

static nav_bool seekStream(void *udata, uint64_t pos)
{
	Stream *stream = (Stream *) udata;
	return (nav_bool) stream->seek((int64) pos);
}

static uint64_t tellStream(void *udata)
{
	Stream *stream = (Stream *) udata;
	return (uint64_t) stream->tell();
}

static uint64_t sizeStream(void *udata)
{
	Stream *stream = (Stream *) udata;
	return (uint64_t) stream->getSize();
}

nav_input streamToNAVInput(love::Stream *stream)
{
	if (!stream->isReadable())
		throw Exception("stream is not readable");
	if (!stream->isSeekable())
		throw Exception("stream is not seekable");

	stream->retain();
	return {
		stream,
		&closeStream,
		&readStream,
		&seekStream,
		&tellStream,
		&sizeStream
	};
}

}