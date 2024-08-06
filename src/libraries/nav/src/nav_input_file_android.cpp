// Android has its own separate file because handling them is messy.
// See https://android.googlesource.com/platform/bionic/+/HEAD/docs/32-bit-abi.md
// This means 32-bit platform < Android N is limited to 2GB file size.
#ifdef __ANDROID__

#include <cstdio>

#include <dlfcn.h>

#include "nav_input_file.hpp"
#include "nav_common.hpp"
#include "nav_error.hpp"

namespace nav::input::file
{

static int(*nav_fseeko64)(FILE *f, int64_t off, int origin) = nullptr;
static int64_t(*nav_ftello64)(FILE *f) = nullptr;
static bool hasInitLFS = false;

static void initLFS()
{
	if (!hasInitLFS)
	{
		nav_fseeko64 = (decltype(nav_fseeko64)) dlsym(RTLD_DEFAULT, "fseeko64");
		nav_ftello64 = (decltype(nav_ftello64)) dlsym(RTLD_DEFAULT, "ftello64");
		hasInitLFS = true;
	}
}

static void closefile(void **userdata)
{
	FILE **f = (FILE**) userdata;
	fclose(*f);
	*f = nullptr;
}

inline int seekimpl(FILE *f, int64_t pos, int origin)
{
	initLFS();

	if (nav_fseeko64)
		return nav_fseeko64(f, pos, origin);
	else
		return fseek(f, (long) pos, origin);
}

inline uint64_t tellimpl(FILE *f)
{
	initLFS();

	if (nav_ftello64)
		return (uint64_t) nav_ftello64(f);
	else
		return (uint64_t) ftell(f);
}

static size_t readfile(void *userdata, void *dest, size_t size)
{
	return fread(dest, 1, size, (FILE*) userdata);
}

static nav_bool seekfile(void *userdata, uint64_t pos)
{
	return seekimpl((FILE*) userdata, (int64_t) pos, SEEK_SET) == 0;
}

static uint64_t tellfile(void *userdata)
{
	return tellimpl((FILE*) userdata);
}

static uint64_t sizefile(void *userdata)
{
	FILE *f = (FILE*) userdata;
	uint64_t curpos = tellimpl(f);
	if (seekimpl(f, 0, SEEK_END) != 0)
		return 0;
	
	uint64_t size = tellimpl(f);
	seekimpl(f, curpos, SEEK_SET);
	return size;
}

bool populate(nav_input *input, const std::string &filename)
{
	initLFS();
	FILE *f = fopen(filename.c_str(), "rb");

	if (!f)
	{
		nav::error::set("Cannot open file");
		return false;
	}

	nav::error::set("");
	input->userdata = f;
	input->close = closefile;
	input->read = readfile;
	input->seek = seekfile;
	input->tell = tellfile;
	input->size = sizefile;
	return true;
}

}

#endif // #ifdef __ANDROID__
