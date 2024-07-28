#ifndef _NAV_CONFIG_HPP_
#define _NAV_CONFIG_HPP_

#ifdef HAVE_CONFIG_H
#	include "config.h"
#else
	// Backend enablements
#	ifdef _WIN32
#		define NAV_BACKEND_MEDIAFOUNDATION
#	endif
#	if defined(__ANDROID__)
#		define NAV_BACKEND_ANDROIDNDK
#	endif
	// Test for __has_include support
#	if (defined(_MSC_VER) && _MSC_VER >= 1911) || defined(__GNUC__) || defined(__clang__)
		// __has_include is supported
#		if __has_include(<libavcodec/avcodec.h>)
#			define NAV_BACKEND_FFMPEG
#		endif
#	endif /* (defined(_MSC_VER) && _MSC_VER >= 1911) || defined(__GNUC__) || defined(__clang__) */
#endif /* HAVE_CONFIG_H */

#endif /* _NAV_CONFIG_HPP_ */
