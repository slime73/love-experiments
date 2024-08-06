#include <algorithm>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

#include "nav_common.hpp"

namespace nav
{

FrameVector::FrameVector(nav_streaminfo_t *streaminfo, size_t streamindex, double position, const void *data, size_t size)
: buffer(size)
, streaminfo(streaminfo)
, streamindex(streamindex)
, position(position)
{
	if (data)
		std::copy((const uint8_t*) data, ((const uint8_t*) data) + size, buffer.data());
}

FrameVector::~FrameVector()
{}

size_t FrameVector::getStreamIndex() const noexcept
{
	return streamindex;
}

nav_streaminfo_t *FrameVector::getStreamInfo() const noexcept
{
	return streaminfo;
}

double FrameVector::tell() const noexcept
{
	return position;
}

size_t FrameVector::size() const noexcept
{
	return buffer.size();
}

void *FrameVector::data() noexcept
{
	return buffer.data();
}

bool FrameVector::operator<(const FrameVector &rhs) const noexcept
{
	return position < rhs.position;
}

bool checkBackendDisabled(const std::string &backendNameUppercase)
{
	std::string name = "NAV_DISABLE_" + backendNameUppercase;
	const char *value = getenv(name.c_str());

	return value != nullptr && (
		strcmp(value, "1") == 0 ||
		strcmp(value, "ON") == 0 ||
		strcmp(value, "on") == 0 ||
		strcmp(value, "On") == 0 ||
		strcmp(value, "YES") == 0 ||
		strcmp(value, "yes") == 0 ||
		strcmp(value, "Yes") == 0
	);
}

#ifdef _WIN32
std::wstring fromUTF8(const std::string &str)
{
	int wsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), str.length(), nullptr, 0);
	if (wsize == 0)
		throw std::runtime_error("Invalid filename");

	std::vector<wchar_t> wide(wsize + 1, 0);
	int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), str.length(), wide.data(), wsize);
	if (!result)
		throw std::runtime_error("Invalid filename");
	
	return std::wstring(wide.data());
}
#endif /* _WIN32 */

}
