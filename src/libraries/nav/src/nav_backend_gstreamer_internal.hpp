#ifndef _NAV_BACKEND_GSTREAMER_INTERNAL_H_
#define _NAV_BACKEND_GSTREAMER_INTERNAL_H_

#include "nav_config.hpp"

#ifdef NAV_BACKEND_GSTREAMER

#include <gst/gst.h>

#include <memory>
#include <string>
#include <vector>
#include <queue>

#include "nav_internal.hpp"
#include "nav_backend.hpp"
#include "nav_common.hpp"
#include "nav_dynlib.hpp"

namespace nav::gstreamer
{

template<typename T>
using UniqueGst = std::unique_ptr<T, void(*)(T*)>;
template<typename T>
using UniqueGstObject = std::unique_ptr<T, decltype(&gst_object_unref)>;
using UniqueGstElement = UniqueGstObject<GstElement>;

class GStreamerBackend;

struct GstMemoryMapLock: GstMapInfo
{
	GstMemoryMapLock(const GstMemoryMapLock &) = delete;
	GstMemoryMapLock(const GstMemoryMapLock &&) = delete;
	GstMemoryMapLock(GStreamerBackend *f, GstMemory *mem, GstMapFlags flags, bool throwOnFail);
	~GstMemoryMapLock();

	GStreamerBackend *f;
	GstMemory *memory;
	bool success;
};

class GStreamerState: public State
{
public:
	GStreamerState(GStreamerBackend *backend, nav_input *input);
	~GStreamerState() override;
	size_t getStreamCount() noexcept override;
	nav_streaminfo_t *getStreamInfo(size_t index) noexcept override;
	bool isStreamEnabled(size_t index) noexcept override;
	bool setStreamEnabled(size_t index, bool enabled) override;
	double getDuration() noexcept override;
	double getPosition() noexcept override;
	double setPosition(double off) override;
	nav_frame_t *read() override;

private:
	struct AppSinkWrapper
	{
		AppSinkWrapper() = delete;
		AppSinkWrapper(const AppSinkWrapper &other) = default;
		AppSinkWrapper(AppSinkWrapper &&other) = default;
		AppSinkWrapper(GStreamerState *state, size_t streamIndex);
		~AppSinkWrapper();

		inline bool ok()
		{
			return queue && convert && sink && streamInfo.type != NAV_STREAMTYPE_UNKNOWN;
		}

		nav_streaminfo_t streamInfo;
		size_t streamIndex;
		GStreamerState *self;
		GstElement *queue, *convert, *sink;
		gulong probeID;
		bool eos, enabled;
	};

	struct FrameComparator
	{
		bool operator()(const FrameVector *lhs, const FrameVector *rhs) const noexcept;
	};

	GStreamerBackend *f;
	nav_input input;
	UniqueGstObject<GstBus> bus;
	UniqueGstElement pipeline;
	GstElement *source, *decoder;
	std::vector<std::unique_ptr<AppSinkWrapper>> streams;
	std::priority_queue<FrameVector*, std::deque<FrameVector*>, FrameComparator> queuedFrames;
	bool padProbed, eos;

	GstCaps *newVideoCapsForNAV();
	GstCaps *newAudioCapsForNAV();
	void clearQueuedFrames();
	void pollBus(bool noexception = false);
	static void padAdded(GstElement *element, GstPad *newPad, GStreamerState *self);
	static void noMorePads(GstElement *element, GStreamerState *self);
	static void needData(GstElement *element, guint length, GStreamerState *self);
	static gboolean seekData(GstElement *element, guint64 pos, GStreamerState *self);
};

class GStreamerBackend: public Backend
{
public:
	GStreamerBackend();
	~GStreamerBackend() override;
	const char *getName() const noexcept override;
	nav_backendtype getType() const noexcept override;
	const char *getInfo() override;
	State *open(nav_input *input, const char *filename) override;

private:
	DynLib glib, gobject, gstreamer;
	std::string version;

public:
#define _NAV_PROXY_FUNCTION_POINTER_GST(lib, n) decltype(n) *ptr_##n;
#include "nav_backend_gstreamer_funcptr.h"
#undef _NAV_PROXY_FUNCTION_POINTER_GST
};

}

#endif /* NAV_BACKEND_GSTREAMER */
#endif /* _NAV_BACKEND_GSTREAMER_INTERNAL_H_ */
