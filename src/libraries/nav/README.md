NAV
=====

NPad Audio Video decoding library.

Performs audio and video decoding library using platform-specific backends.

Supported Backends
-----

Backends and OS listed here are the ones that planned. More backends and OSes will be added on demand.

| Backend/OS | [FFmpeg 6] | [MediaFoundation] | [NdkMedia] | [GStreamer] | [libVLC] |
|------------|------------|-------------------|------------|-------------|----------|
| Windows    | **YES**    | **YES**           | N/A        | No          | No       |
| Linux      | **YES**    | N/A               | N/A        | **YES**     | Planned  |
| Android    | **YES**    | N/A               | **YES**    | N/A         | No       |

Note: FFmpeg 6 backend requires the appropriate header files to be present.

[FFmpeg 6]: https://ffmpeg.org/
[MediaFoundation]: https://learn.microsoft.com/en-us/windows/win32/medfound/microsoft-media-foundation-sdk
[NdkMedia]: https://developer.android.com/ndk/reference/group/media
[GStreamer]: https://gstreamer.freedesktop.org/documentation/libs.html
[libVLC]: https://www.videolan.org/vlc/libvlc.html

License
-----

zlib/libpng license.
