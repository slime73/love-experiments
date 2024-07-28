cmake_minimum_required(VERSION 3.21)
project(nav LANGUAGES C CXX)

if(POLICY CMP0077)
	cmake_policy(SET CMP0077 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Read version
file(READ include/nav/nav.h NAV_INCLUDE_H LIMIT 1024)
string(REGEX MATCH "#define NAV_VERSION_MAJOR ([0-9+])" _ "${NAV_INCLUDE_H}")
set(NAV_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define NAV_VERSION_MINOR ([0-9+])" _ "${NAV_INCLUDE_H}")
set(NAV_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define NAV_VERSION_PATCH ([0-9+])" _ "${NAV_INCLUDE_H}")
set(NAV_VERSION_PATCH ${CMAKE_MATCH_1})

# Public include files
set(NAV_PUBLIC_INCLUDE
	include/nav/audioformat.h
	include/nav/input.h
	include/nav/nav.h
	include/nav/types.h
)

# Shared/static library
if(NOT DEFINED NAV_SHARED)
	if(BUILD_SHARED_LIBS)
		set(NAV_SHARED_DEFAULT 1)
	else()
		set(NAV_SHARED_DEFAULT 0)
	endif()
endif()
option(NAV_SHARED "NAV: Build as shared library" ${NAV_SHARED_DEFAULT})

if(NAV_SHARED)
	set(NAV_SHARED_VALUE SHARED)
else()
	set(NAV_SHARED_VALUE STATIC)
endif()

add_library(nav ${NAV_SHARED_VALUE}
	${NAV_PUBLIC_INCLUDE}
	src/nav.cpp
	src/nav_backend.cpp
	src/nav_backend.hpp
	src/nav_backend_androidndk.cpp
	src/nav_backend_androidndk.hpp
	src/nav_backend_androidndk_funcptr.h
	src/nav_backend_androidndk_internal.hpp
	src/nav_backend_ffmpeg.cpp
	src/nav_backend_ffmpeg.hpp
	src/nav_backend_ffmpeg_funcptr.h
	src/nav_backend_ffmpeg_internal.hpp
	src/nav_backend_mediafoundation.cpp
	src/nav_backend_mediafoundation.hpp
	src/nav_backend_mediafoundation_internal.hpp
	src/nav_common.cpp
	src/nav_common.hpp
	src/nav_config.hpp
	src/nav_dynlib.cpp
	src/nav_dynlib.hpp
	src/nav_error.cpp
	src/nav_error.hpp
	src/nav_input_file.cpp
	src/nav_input_file.hpp
	src/nav_input_memory.cpp
	src/nav_input_memory.hpp
	src/nav_internal.hpp
	src/nav_internal.cpp
)
target_include_directories(nav PUBLIC include)
target_compile_features(nav PUBLIC c_std_99)
target_compile_features(nav PRIVATE cxx_std_17)

if(NAV_SHARED)
	target_compile_definitions(nav PUBLIC NAV_SHARED)

	if(NOT ANDROID)
		set_target_properties(nav PROPERTIES
			VERSION ${NAV_VERSION_MAJOR}.${NAV_VERSION_MINOR}.${NAV_VERSION_PATCH}
			SOVERSION ${NAV_VERSION_MAJOR}
		)
	endif()
endif()

# Find FFmpeg
if(FFMPEG_DIR)
	# Ok I trusted you
	set(FFMPEG_INCLUDE_DIRS ${FFMPEG_DIR}/include)
else()
	find_package(FFmpeg COMPONENTS AVCODEC AVFORMAT AVUTIL SWRESAMPLE SWSCALE)
endif()

if(FFMPEG_FOUND OR FFMPEG_INCLUDE_DIRS)
	message(STATUS "Using FFmpeg include directory: ${FFMPEG_INCLUDE_DIRS}")
	target_include_directories(nav PRIVATE ${FFMPEG_INCLUDE_DIRS})
endif()

# Install files
if(PROJECT_IS_TOP_LEVEL)
	include(GNUInstallDirs)
	install(TARGETS nav)
	install(FILES ${NAV_PUBLIC_INCLUDE} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nav)
	if(WIN32 AND NAV_SHARED)
		install(FILES $<TARGET_PDB_FILE:nav> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
	endif()
endif()
