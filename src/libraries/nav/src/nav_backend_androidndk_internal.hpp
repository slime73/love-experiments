#ifndef _NAV_BACKEND_ANDROIDNDK_INTERNAL_HPP_
#define _NAV_BACKEND_ANDROIDNDK_INTERNAL_HPP_

#include "nav_backend_androidndk.hpp"

#ifdef NAV_BACKEND_ANDROIDNDK

#include <memory>
#include <vector>

#include <media/NdkMediaExtractor.h>

#include "nav_backend.hpp"
#include "nav_dynlib.hpp"

namespace nav::androidndk
{

using UniqueMediaExtractor = std::unique_ptr<AMediaExtractor, decltype(&AMediaExtractor_delete)>;
using UniqueMediaFormat = std::unique_ptr<AMediaFormat, decltype(&AMediaFormat_delete)>;
using UniqueMediaCodec = std::unique_ptr<AMediaCodec, decltype(&AMediaCodec_delete)>;

class AndroidNDKBackend;

class MediaSourceWrapper
{
public:
	MediaSourceWrapper(const MediaSourceWrapper &other) = delete;
	MediaSourceWrapper(AndroidNDKBackend *backend, nav_input *input);
	MediaSourceWrapper(MediaSourceWrapper &&other);
	~MediaSourceWrapper();
	AMediaDataSource *get() const noexcept;

private:
	static ssize_t readAt(void *userdata, off64_t offset, void * buffer, size_t size);
	static ssize_t getSize(void *userdata);
	static void close(void *userdata);
	static ssize_t getAvailableSize(void *userdata, off64_t offset);

	AndroidNDKBackend *f;
	AMediaDataSource *datasource;
	nav_input *input;
};

class AndroidNDKState: public State
{
public:
	AndroidNDKState(AndroidNDKBackend *backend, UniqueMediaExtractor &ext, MediaSourceWrapper &ds);
	~AndroidNDKState() override;
	size_t getStreamCount() noexcept override;
	nav_streaminfo_t *getStreamInfo(size_t index) noexcept override;
	bool isStreamEnabled(size_t index) noexcept override;
	bool setStreamEnabled(size_t index, bool enabled) override;
	double getDuration() noexcept override;
	double getPosition() noexcept override;
	double setPosition(double off) override;
	nav_frame_t *read() override;

private:
	AndroidNDKBackend *f;
	int64_t durationUs;
	int64_t positionUs;

	std::vector<bool> activeStream, hasEOS;
	std::vector<nav_streaminfo_t> streamInfo;
	std::vector<UniqueMediaCodec> decoders;

	UniqueMediaExtractor extractor;
	MediaSourceWrapper dataSource;
};

class AndroidNDKBackend: public Backend
{
public:
	AndroidNDKBackend();
	~AndroidNDKBackend() override;
	const char *getName() const noexcept override;
	nav_backendtype getType() const noexcept override;
	const char *getInfo() override;
	State *open(nav_input *input, const char *filename) override;

private:
	friend class MediaSourceWrapper;
	friend class AndroidNDKState;
	DynLib mediandk;

#define _NAV_PROXY_FUNCTION_POINTER(n) decltype(n) *ptr_##n;
#include "nav_backend_androidndk_funcptr.h"
#undef _NAV_PROXY_FUNCTION_POINTER
};

}

#endif /* NAV_BACKEND_ANDROIDNDK */

#endif /* _NAV_BACKEND_ANDROIDNDK_INTERNAL_HPP_ */
