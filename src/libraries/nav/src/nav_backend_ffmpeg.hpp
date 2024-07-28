#ifndef _NAV_BACKEND_FFMPEG_
#define _NAV_BACKEND_FFMPEG_

#include "nav_config.hpp"

#ifdef NAV_BACKEND_FFMPEG

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#define NAV_BACKEND_FFMPEG_OK \
	(LIBAVCODEC_VERSION_MAJOR >= 60) && \
	(LIBAVFORMAT_VERSION_MAJOR >= 60) && \
	(LIBAVUTIL_VERSION_MAJOR >= 58) && \
	(LIBSWRESAMPLE_VERSION_MAJOR >= 4) && \
	(LIBSWSCALE_VERSION_MAJOR >= 7)

#if NAV_BACKEND_FFMPEG_OK

#include "nav_internal.hpp"
#include "nav_backend.hpp"

namespace nav::ffmpeg
{

Backend *create();

}

#endif /* NAV_BACKEND_FFMPEG_OK */
#endif /* NAV_BACKEND_FFMPEG */
#endif /* _NAV_BACKEND_FFMPEG_ */
