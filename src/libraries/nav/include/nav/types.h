#ifndef _NAV_TYPES_H_
#define _NAV_TYPES_H_

#include <stddef.h>
#include <stdint.h>

typedef struct nav_t nav_t;
typedef struct nav_streaminfo_t nav_streaminfo_t;
typedef struct nav_frame_t nav_frame_t;

#ifdef __cplusplus
typedef bool nav_bool;
#else
#include <stdbool.h>
typedef _Bool nav_bool;
#endif

typedef enum nav_pixelformat
{
	/* Unknown pixel format (may denote errors) */
	NAV_PIXELFORMAT_UNKNOWN = -1,
	/* RGB, 24 bits per pixel, packed. */
	NAV_PIXELFORMAT_RGB8,
	/* YUV 4:2:0 subsampling, planar. */
	NAV_PIXELFORMAT_YUV420,
	/* YUV 4:4:4 subsampling, planar. */
	NAV_PIXELFORMAT_YUV444,
	/* YUV 4:2:0 subsampling, Y is planar, UV is packed. */
	NAV_PIXELFORMAT_NV12
} nav_pixelformat;

typedef enum nav_streamtype
{
	/* Unknown stream type */
	NAV_STREAMTYPE_UNKNOWN = -1,
	/* Audio stream */
	NAV_STREAMTYPE_AUDIO,
	/* Video stream */
	NAV_STREAMTYPE_VIDEO
} nav_streamtype;

typedef enum nav_backendtype
{
	/* Unknown backend type. */
	NAV_BACKENDTYPE_UNKNOWN = -1,
	/* This backend leverages OS-specific API to work (almost guaranteed to exist in particular OS). */
	NAV_BACKENDTYPE_OS_API,
	/* This backend leverages 3rd-party library to work (depends if the user installed the library). */
	NAV_BACKENDTYPE_3RD_PARTY
} nav_backendtype;

#endif /* _NAV_TYPES_H_ */
