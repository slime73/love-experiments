#include <exception>
#include <mutex>
#include <numeric>
#include <vector>

#include "nav_internal.hpp"
#include "nav_backend.hpp"
#include "nav_backend_ffmpeg.hpp"
#include "nav_backend_mediafoundation.hpp"
#include "nav_error.hpp"
#include "nav_input_file.hpp"
#include "nav_input_memory.hpp"

#include "nav/nav.h"

static class BackendContainer
{
public:
	BackendContainer(std::initializer_list<nav::Backend*(*)()> backendlist)
	: initialized(false)
	, activeBackend()
	, factory(backendlist)
	, mutex()
	{}

	~BackendContainer()
	{
		for (nav::Backend *b: activeBackend)
			delete b;
	}

	void init()
	{
		std::lock_guard lg(mutex);

		if (!initialized)
		{
			for (auto func: factory)
			{
				if (func)
				{
					nav::Backend *b = func();

					if (b != nullptr)
						activeBackend.push_back(b);
				}
			}

			for (size_t i = 0; i < activeBackend.size(); i++)
				defaultOrder.push_back(i + 1);
			defaultOrder.push_back(0);

			initialized = true;
		}

	}

	inline void ensureInit()
	{
		if (!initialized)
			init();
	}

	nav::State *open(nav_input *input, const char *filename, const size_t *order)
	{
		ensureInit();

		std::vector<std::string> errors;
		if (!order)
			order = defaultOrder.data();

		for (size_t backendIndex = *order; *order; order++)
		{
			if (backendIndex > 0 && backendIndex <= activeBackend.size())
			{
				nav::Backend *b = activeBackend[backendIndex - 1];

				try
				{
					return b->open(input, filename);
				}
				catch (const std::exception &e)
				{
					errors.push_back(e.what());
				}
			}
		}

		if (errors.empty())
			nav::error::set("No backend available");
		else
			nav::error::set(std::reduce(errors.begin(), errors.end(), std::string("\n")));
		return nullptr;
	}

	size_t count()
	{
		ensureInit();
		return activeBackend.size();
	}

	nav::Backend *getBackend(size_t i)
	{
		ensureInit();

		if (i > 0 && i <= activeBackend.size())
			return activeBackend[i - 1];

		nav::error::set("Index out of range");
		return nullptr;
	}

private:
	bool initialized;
	std::vector<nav::Backend*> activeBackend;
	std::vector<nav::Backend*(*)()> factory;
	std::vector<size_t> defaultOrder;
	std::mutex mutex;
} backendContainer({
#if defined(NAV_BACKEND_FFMPEG) && (NAV_BACKEND_FFMPEG_OK)
	&nav::ffmpeg::create,
#endif
#ifdef NAV_BACKEND_MEDIAFOUNDATION
	&nav::mediafoundation::create,
#endif
	nullptr
});

template<typename T, typename C, typename... Args, typename... UArgs>
T wrapcall(C *c, T(C::*m)(Args...), T defval, UArgs... args)
{
    try
	{
		nav::error::set("");
		T result = (c->*m)(args...);
		return result;
	}
    catch (const std::exception &e)
	{
		nav::error::set(e.what());
		return defval;
	}
}

extern "C" uint32_t nav_version()
{
	return NAV_VERSION;
}

extern "C" const char *nav_version_string()
{
	return NAV_STRINGIZE(NAV_VERSION_MAJOR) "." NAV_STRINGIZE(NAV_VERSION_MINOR) "." NAV_STRINGIZE(NAV_VERSION_PATCH);
}

extern "C" const char *nav_error()
{
	return nav::error::get();
}

extern "C" void nav_input_populate_from_memory(nav_input *input, void *buf, size_t size)
{
	nav::error::set("");
	nav::input::memory::populate(input, buf, size);
}

extern "C" nav_bool nav_input_populate_from_file(nav_input *input, const char *filename)
{
	nav::error::set("");
	return (nav_bool) nav::input::file::populate(input, filename);
}

extern "C" size_t nav_backend_count()
{
	nav::error::set("");
	return backendContainer.count();
}

extern "C" const char *nav_backend_name(size_t index)
{
	nav::error::set("");
	nav::Backend *backend = backendContainer.getBackend(index);
	return backend ? backend->getName() : nullptr;
}

extern "C" nav_backendtype nav_backend_type(size_t index)
{
	nav::error::set("");
	nav::Backend *backend = backendContainer.getBackend(index);
	return backend ? backend->getType() : NAV_BACKENDTYPE_UNKNOWN;
}

extern "C" const char *nav_backend_info(size_t index)
{
	nav::error::set("");
	nav::Backend *backend = backendContainer.getBackend(index);
	return backend ? wrapcall<const char*>(backend, &nav::Backend::getInfo, nullptr) : nullptr;
}

extern "C" nav_t *nav_open(nav_input *input, const char *filename, const size_t *order)
{
	return wrapcall<nav_t*>(&backendContainer, &BackendContainer::open, nullptr, input, filename, order);
}

extern "C" void nav_close(nav_t *state)
{
	nav::error::set("");
	delete state;
}

extern "C" size_t nav_nstreams(nav_t *state)
{
	nav::error::set("");
	return state->getStreamCount();
}

extern "C" nav_streaminfo_t *nav_stream_info(nav_t *state, size_t index)
{
	nav::error::set("");
	return state->getStreamInfo(index);
}

extern "C" nav_bool nav_stream_is_enabled(nav_t *state, size_t index)
{
	nav::error::set("");
	return (nav_bool) state->isStreamEnabled(index);
}

extern "C" nav_bool nav_stream_enable(nav_t *state, size_t index, nav_bool enable)
{
	return (nav_bool) wrapcall(state, &nav::State::setStreamEnabled, false, index, enable);
}

extern "C" double nav_tell(nav_t *state)
{
	nav::error::set("");
	return state->getPosition();
}

extern "C" double nav_duration(nav_t *state)
{
	nav::error::set("");
	return state->getDuration();
}

extern "C" double nav_seek(nav_t *state, double position)
{
	return wrapcall(state, &nav::State::setPosition, -1., position);
}

extern "C" nav_frame_t *nav_read(nav_t *state)
{
	return wrapcall<nav_frame_t*>(state, &nav::State::read, nullptr);
}

extern "C" nav_streamtype nav_streaminfo_type(nav_streaminfo_t *sinfo)
{
	nav::error::set("");
	return sinfo->type;
}

extern "C" size_t nav_audio_size(nav_streaminfo_t *sinfo)
{
	if (sinfo->type != NAV_STREAMTYPE_AUDIO)
	{
		nav::error::set("Not an audio stream");
		return 0;
	}

	nav::error::set("");
	return sinfo->audio.size();
}

extern "C" uint32_t nav_audio_sample_rate(nav_streaminfo_t *sinfo)
{
	if (sinfo->type != NAV_STREAMTYPE_AUDIO)
	{
		nav::error::set("Not an audio stream");
		return 0;
	}

	nav::error::set("");
	return sinfo->audio.sample_rate;
}

extern "C" uint32_t nav_audio_nchannels(nav_streaminfo_t *sinfo)
{
	if (sinfo->type != NAV_STREAMTYPE_AUDIO)
	{
		nav::error::set("Not an audio stream");
		return 0;
	}

	nav::error::set("");
	return sinfo->audio.nchannels;
}

extern "C" nav_audioformat nav_audio_format(nav_streaminfo_t *sinfo)
{
	if (sinfo->type != NAV_STREAMTYPE_AUDIO)
	{
		nav::error::set("Not an audio stream");
		return 0;
	}

	nav::error::set("");
	return sinfo->audio.format;
}

extern "C" size_t nav_video_size(nav_streaminfo_t *sinfo)
{
	if (sinfo->type != NAV_STREAMTYPE_VIDEO)
	{
		nav::error::set("Not a video stream");
		return 0;
	}

	nav::error::set("");
	return sinfo->video.size();
}

extern "C" void nav_video_dimensions(nav_streaminfo_t *sinfo, uint32_t *width, uint32_t *height)
{
	if (sinfo->type != NAV_STREAMTYPE_VIDEO)
	{
		nav::error::set("Not a video stream");
		*width = *height = 0;
		return;
	}

	nav::error::set("");
	*width = sinfo->video.width;
	*height = sinfo->video.height;
}

extern "C" nav_pixelformat nav_video_pixel_format(nav_streaminfo_t *sinfo)
{
	if (sinfo->type != NAV_STREAMTYPE_VIDEO)
	{
		nav::error::set("Not a video stream");
		return NAV_PIXELFORMAT_UNKNOWN;
	}

	nav::error::set("");
	return sinfo->video.format;
}

extern "C" double nav_video_fps(nav_streaminfo_t *sinfo)
{
	if (sinfo->type != NAV_STREAMTYPE_VIDEO)
	{
		nav::error::set("Not a video stream");
		return 0.0;
	}

	nav::error::set("");
	return sinfo->video.fps;
}

extern "C" size_t nav_frame_streamindex(nav_frame_t *frame)
{
	nav::error::set("");
	return frame->getStreamIndex();
}

extern "C" nav_streaminfo_t *nav_frame_streaminfo(nav_frame_t *frame)
{
	nav::error::set("");
	return frame->getStreamInfo();
}

extern "C" double nav_frame_tell(nav_frame_t *frame)
{
	nav::error::set("");
	return frame->tell();
}

extern "C" size_t nav_frame_size(nav_frame_t *frame)
{
	return frame->size();
}

extern "C" const void *nav_frame_buffer(nav_frame_t *frame)
{
	return frame->data();
}

extern "C" void nav_frame_free(nav_frame_t *frame)
{
	delete frame;
}
