NAV
=====

NPad Audio Video decoding library.

Performs audio and video decoding library using platform-specific backends.

Backends
-----

NAV relies on various backends to perform audio and video decoding.

| Backend           | Kind      | OS Availability         | `nav_backend_name()` | Disablement Env. Var          | Additional Notes                                                        |
|-------------------|-----------|-------------------------|----------------------|-------------------------------|-------------------------------------------------------------------------|
| [NdkMedia]        | OS API    | Android                 | `"android"`          | `NAV_DISABLE_ANDROIDNDK`      | Due to API limitations, Android 9 is required.                          |
| [FFmpeg 6]        | 3rd-Party | Windows, Linux, Android | `"ffmpeg"`           | `NAV_DISABLE_FFMPEG`          | Requires the appropriate header files to be present when compiling NAV. |
| [GStreamer]       | 3rd-Party | Linux                   | `"gstreamer"`        | `NAV_DISABLE_GSTREAMER`       |                                                                         |
| [MediaFoundation] | OS API    | Windows                 | `"mediafoundation"`  | `NAV_DISABLE_MEDIAFOUNDATION` | Due to API limitations, UWP target is currently unsupported.            |

[FFmpeg 6]: https://ffmpeg.org/
[MediaFoundation]: https://learn.microsoft.com/en-us/windows/win32/medfound/microsoft-media-foundation-sdk
[NdkMedia]: https://developer.android.com/ndk/reference/group/media
[GStreamer]: https://gstreamer.freedesktop.org/

License
-----

zlib/libpng license.
