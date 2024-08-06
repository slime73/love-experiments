#ifndef _NAV_COMMON_H_
#define _NAV_COMMON_H_

#include <numeric>
#include <string>
#include <vector>
#include <type_traits>

#include "nav_internal.hpp"

namespace nav
{

struct FrameVector: public nav_frame_t
{
	FrameVector(nav_streaminfo_t *streaminfo, size_t streamindex, double position, const void *data, size_t size);
	~FrameVector() override;
	size_t getStreamIndex() const noexcept override;
	nav_streaminfo_t *getStreamInfo() const noexcept override;
	double tell() const noexcept override;
	size_t size() const noexcept override;
	void *data() noexcept override;

	bool operator<(const FrameVector &rhs) const noexcept;

private:
	std::vector<uint8_t> buffer;
	nav_streaminfo_t *streaminfo;
	size_t streamindex;
	double position;
};

template<typename T>
double derationalize(T num, T den, double dv0 = 0.0)
{
	if (!den)
		return dv0;
	T gcd = std::gcd(num, den);
	num /= gcd;
	den /= gcd;
	return double(num) / double(den);
}

constexpr nav_audioformat makeAudioFormat(uint8_t bps, bool is_float, bool is_signed)
{
	uint16_t floatval = NAV_AUDIOFORMAT_ISFLOAT(0xFFFFu) * is_float;
	uint16_t signedval = NAV_AUDIOFORMAT_ISSIGNED(0xFFFFu) * is_signed;
	return nav_audioformat(uint16_t(bps) | floatval | signedval);
}

bool checkBackendDisabled(const std::string &backendNameUppercase);

#ifdef _WIN32
std::wstring fromUTF8(const std::string &str);
#endif /* _WIN32 */

}

#endif /* _NAV_COMMON_H_ */
