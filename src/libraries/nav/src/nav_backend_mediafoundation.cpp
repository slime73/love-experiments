#include "nav_backend_mediafoundation.hpp"

#ifdef NAV_BACKEND_MEDIAFOUNDATION

#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <initguid.h>
#include <windows.h>
#include <mferror.h>

#include "nav_common.hpp"
#include "nav_error.hpp"
#include "nav_backend_mediafoundation_internal.hpp"

/* Notes on MF backend:
 * * Currently it's software-renderer. Using HW accelerated renderer needs D3D11 device. This is TODO.
 * * Audio is always assumed 16-bit PCM.
 * * Video is always assumed YUV420 planar.
 */

namespace
{

// Don't use GUID_NULL, it needs linking to uuid.lib which is no-no.
constexpr GUID NULL_GUID = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
constexpr GUID IMFAttributes_GUID = {0x2cd2d921, 0xc447, 0x44a7, {0xa1, 0x3c, 0x4a, 0xda, 0xbf, 0xc2, 0x47, 0xe3}};
constexpr GUID IMFTransform_GUID = {0xbf94c121, 0x5b05, 0x4e6f, {0x80, 0x00, 0xba, 0x59, 0x89, 0x61, 0x41, 0x4d}};
constexpr GUID IMF2DBuffer_GUID = {0x7DC9D5F9, 0x9ED9, 0x44ec, {0x9B, 0xBF, 0x06, 0x00, 0xBB, 0x58, 0x9F, 0xBB}};

constexpr uint64_t MF_100NS_UNIT = 10000000;

struct MediaTypeCombination
{
	GUID guid;
	nav_pixelformat pixfmt;
} mediaTypeCombination[] = {
	{MFVideoFormat_I420, NAV_PIXELFORMAT_YUV420},
	{MFVideoFormat_IYUV, NAV_PIXELFORMAT_YUV420},
	{MFVideoFormat_NV12, NAV_PIXELFORMAT_NV12},
	{MFVideoFormat_RGB24, NAV_PIXELFORMAT_RGB8},
};

struct KnownBestCombination
{
	GUID codec;
	GUID rawformat;
	nav_pixelformat pixfmt;
} knownBestCombination[] = {
	{MFVideoFormat_H264, MFVideoFormat_I420, NAV_PIXELFORMAT_YUV420},
	{MFVideoFormat_VP90, MFVideoFormat_NV12, NAV_PIXELFORMAT_NV12},
};

inline void unixTimestampToFILETIME(FILETIME &ft, uint64_t t = 0LL)
{
	ULARGE_INTEGER tv;
	tv.QuadPart = (t * 10000000LL) + 116444736000000000LL;
	ft.dwHighDateTime = tv.HighPart;
	ft.dwLowDateTime = tv.LowPart;
}

struct WrappedPropVariant: public PROPVARIANT
{
	WrappedPropVariant(const WrappedPropVariant &other) = delete;
	WrappedPropVariant(WrappedPropVariant &&other) = delete;

	WrappedPropVariant()
	{
		PropVariantInit(this);
	}

	WrappedPropVariant(uint64_t v)
	: WrappedPropVariant()
	{
		this->vt = VT_UI8;
		this->uhVal.QuadPart = v;
	}

	~WrappedPropVariant()
	{
		PropVariantClear(this);
	}
};

// static std::ostream &operator<<(std::ostream &st, const GUID &guid)
// {
// 	char buf[64];
// 	std::fill(buf, buf + 64, '\0');

// 	sprintf(
// 		buf,
// 		"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
// 		guid.Data1,
// 		guid.Data2,
// 		guid.Data3,
// 		guid.Data4[0],
// 		guid.Data4[1],
// 		guid.Data4[2],
// 		guid.Data4[3],
// 		guid.Data4[4],
// 		guid.Data4[5],
// 		guid.Data4[6],
// 		guid.Data4[7]
// 	);
// 	return st << buf;
// }

// static void dumpIMFAttributes(IMFAttributes *attr)
// {
// 	UINT32 count = 0;
// 	if (SUCCEEDED(attr->GetCount(&count)))
// 	{
// 		for (UINT32 i = 0; i < count; i++)
// 		{
// 			GUID key = NULL_GUID;
// 			WrappedPropVariant value;

// 			if (SUCCEEDED(attr->GetItemByIndex(i, &key, &value)))
// 			{
// 				std::cout << key << ": ";
// 				if (value.vt == VT_CLSID)
// 					std::cout << *value.puuid;
// 				else
// 					std::cout << value.uhVal.QuadPart;
// 				std::cout << std::endl;
// 			}
// 		}
// 	}
// }

// static GUID getBestPixelFormat(decltype(MFTEnumEx) *MFTEnumEx, IMFMediaType *mediaType)
// {
// 	using namespace nav::mediafoundation;

// 	GUID inputMediaType;
// 	if (FAILED(mediaType->GetGUID(MF_MT_SUBTYPE, &inputMediaType)))
// 		return NULL_GUID;

// 	MFT_REGISTER_TYPE_INFO input = {MFMediaType_Video, inputMediaType};
// 	IMFActivate **activator = nullptr;
// 	UINT32 numDecoders;

// 	if (FAILED(MFTEnumEx(
// 		MFT_CATEGORY_VIDEO_DECODER,
// 		MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_HARDWARE,
// 		&input,
// 		nullptr,
// 		&activator,
// 		&numDecoders
// 	)))
// 		return NULL_GUID;

// 	GUID result = NULL_GUID;
// 	bool found = false;

// 	for (UINT32 i = 0; i < numDecoders; i++)
// 	{
// 		ComPtr<IMFActivate> act(activator[i], false);

// 		if (!found)
// 		{
// 			// https://www.winehq.org/pipermail/wine-devel/2020-March/162153.html
// 			ComPtr<IMFTransform> transform;

// 			if (FAILED(act->ActivateObject(IMFTransform_GUID, (void**) transform.dptr())))
// 				continue;

// 			if (FAILED(transform->SetInputType(0, mediaType, 0)))
// 			{
// 				act->ShutdownObject();
// 				continue;
// 			}

// 			for (UINT32 j = 0;; j++)
// 			{
// 				ComPtr<IMFMediaType> outputType;

// 				if (FAILED(transform->GetOutputAvailableType(0, j, outputType.dptr())))
// 					continue;
				
// 				GUID outputMediaType;

// 				if (SUCCEEDED(outputType->GetGUID(MF_MT_SUBTYPE, &outputMediaType)))
// 				{
// 					result = outputMediaType;
// 					found = true;
// 				}
// 			}

// 			act->ShutdownObject();
// 		}
// 	}

// 	CoTaskMemFree(activator);
// 	return result;
// }

static size_t bruteForceExtraHeight(uint32_t width, uint32_t height, size_t contigSize, nav_pixelformat pixelformat)
{
	constexpr size_t MAX_ADD_HEIGHT = 32;
	size_t w = width, h = height;

	switch (pixelformat)
	{
		case NAV_PIXELFORMAT_YUV420:
		{
			for (size_t i = 0; i <= MAX_ADD_HEIGHT; i++)
			{
				size_t size = w * (h + i) + 2 * ((w + 1) / 2) * ((h + i + 1) / 2);
				if (size == contigSize)
					return i;
			}
			break;
		}
		case NAV_PIXELFORMAT_YUV444:
		{
			for (size_t i = 0; i <= MAX_ADD_HEIGHT; i++)
			{
				size_t size = w * (h + i) * 3;
				if (size == contigSize)
					return i;
			}
			break;
		}
		case NAV_PIXELFORMAT_NV12:
		{
			for (size_t i = 0; i <= MAX_ADD_HEIGHT; i++)
			{
				size_t size = w * (h + i) + w * (h + i + 1) / 2;
				if (size == contigSize)
					return i;
			}
			break;
		}
		default:
			break;
	}

	return 0;
}

}

namespace nav::mediafoundation
{

NavInputStream::NavInputStream(nav_input *input, const char *filename)
: filename(filename ? filename : "")
, input(input)
, refc(1)
{}

ULONG STDMETHODCALLTYPE NavInputStream::AddRef()
{
	return ++refc;
}

HRESULT STDMETHODCALLTYPE NavInputStream::QueryInterface(REFIID riid, void **out)
{
	if (riid == IID_IUnknown || riid == IID_ISequentialStream || riid == IID_IStream)
	{
		*out = this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE NavInputStream::Release()
{
	ULONG newref = --refc;
	if (refc == 0)
		delete this;
	
	return newref;
}

HRESULT STDMETHODCALLTYPE NavInputStream::Read(void *dest, ULONG size, ULONG *readed)
{
	ULONG r = (ULONG) input->readf(dest, size);
	if (readed)
		*readed = r;
	return r == size ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE NavInputStream::Write(const void *src, ULONG size, ULONG *written)
{
	if (written)
		*written = 0;
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE NavInputStream::Clone(IStream **out)
{
	/* Is this needed? */
	*out = nullptr;
	return STG_E_INVALIDFUNCTION;
}
HRESULT STDMETHODCALLTYPE NavInputStream::Commit(DWORD flags)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE NavInputStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *readed, ULARGE_INTEGER *written)
{
	uint8_t *buf = new (std::nothrow) uint8_t[(size_t) cb.QuadPart];
	if (buf == nullptr)
		return E_OUTOFMEMORY;

	ULONG wr = 0;
	size_t rd = input->readf(buf, cb.QuadPart);
	HRESULT res = pstm->Write(buf, (ULONG) rd, &wr);
	delete[] buf;

	if (FAILED(res))
		return res;
	
	if (readed)
		readed->QuadPart = rd;
	if (written)
		written->QuadPart = wr;

	return ((ULONG) rd) == wr ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE NavInputStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}
HRESULT STDMETHODCALLTYPE NavInputStream::Revert(void)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE NavInputStream::Seek(LARGE_INTEGER offset, DWORD origin, ULARGE_INTEGER *newpos)
{
	uint64_t realoff = 0;

	switch (origin)
	{
		case STREAM_SEEK_SET:
		{
			if(offset.QuadPart < 0)
				return STG_E_INVALIDFUNCTION;

			realoff = (uint64_t) offset.QuadPart;
			break;
		}
		case STREAM_SEEK_CUR:
		{
			int64_t curpos = (int64_t) input->tellf();
			if (offset.QuadPart < 0 && (-offset.QuadPart) > curpos)
				return STG_E_INVALIDFUNCTION;

			realoff = uint64_t(curpos + offset.QuadPart);
			break;
		}
		case STREAM_SEEK_END:
		{
			if (offset.QuadPart > 0)
				return STG_E_INVALIDFUNCTION;

			int64_t curpos = (int64_t) input->sizef();
			realoff = uint64_t(curpos + offset.QuadPart);
			break;
		}
		default:
			return STG_E_INVALIDPARAMETER;
	}
	if (!input->seekf(realoff))
		return E_FAIL;
	
	if (newpos)
		newpos->QuadPart = input->tellf();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE NavInputStream::SetSize(ULARGE_INTEGER libNewSize)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE NavInputStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	if (pstatstg == nullptr)
		return STG_E_INVALIDPOINTER;
	
	if (!(grfStatFlag | STATFLAG_NONAME))
	{
		std::wstring widefilename = nav::fromUTF8(filename);
		wchar_t *mem = (wchar_t*) CoTaskMemAlloc(widefilename.length() + 1);
		if (mem == nullptr)
			return E_OUTOFMEMORY;

		std::copy(widefilename.c_str(), widefilename.c_str() + widefilename.length() + 1, mem);
		pstatstg->pwcsName = mem;
	}
	else
		pstatstg->pwcsName = nullptr;

	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = input->sizef();
	unixTimestampToFILETIME(pstatstg->mtime);
	unixTimestampToFILETIME(pstatstg->ctime);
	unixTimestampToFILETIME(pstatstg->atime);
	pstatstg->grfMode = STGM_READ;
	pstatstg->grfLocksSupported = 0;
	pstatstg->clsid = NULL_GUID;
	pstatstg->grfStateBits = 0;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE NavInputStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

MediaFoundationState::MediaFoundationState(MediaFoundationBackend *backend, nav_input *input, ComPtr<NavInputStream> &is, ComPtr<IMFByteStream> &mfbs, ComPtr<IMFSourceReader> &mfsr)
: backend(backend)
, originalInput(input)
, inputStream(is)
, mfByteStream(mfbs)
, mfSourceReader(mfsr)
, currentPosition(0)
{
	if (FAILED(mfsr->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, TRUE)))
		throw std::runtime_error("IMFSourceReader::SetStreamSelection failed");
	
	// Find how many streams
	for (DWORD i = 0; i < 0xFFFFFFFCU; i++)
	{
		ComPtr<IMFMediaType> mfMediaType, mfNativeMediaType;
		HRESULT hr = mfsr->GetCurrentMediaType(i, mfMediaType.dptr());

		if (hr == MF_E_INVALIDSTREAMNUMBER)
			break;
		else if (FAILED(hr))
			throw std::runtime_error("IMFSourceReader::GetCurrentMediaType failed");

		nav::StreamInfo streamInfo {NAV_STREAMTYPE_UNKNOWN};
		GUID majorType;

		if (SUCCEEDED(mfMediaType->GetMajorType(&majorType)))
		{
			ComPtr<IMFMediaType> partialType, decoderType;
			bool failed = false;

			if (majorType == MFMediaType_Audio)
			{
				// FIXME: Do not assume non-float and signed bps 16.
				failed = FAILED(backend->MFCreateMediaType(partialType.dptr()));

				if (!failed)
				{
					partialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
					partialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
					partialType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
					failed = failed || FAILED(mfsr->SetCurrentMediaType(i, nullptr, partialType.get()));
				}

				if (!failed && SUCCEEDED(mfsr->GetCurrentMediaType(i, decoderType.dptr())))
				{
					UINT32 bps;

					// Sample rate
					hr = mfMediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &streamInfo.audio.sample_rate);
					failed = failed || FAILED(hr);

					// Number of channels
					hr = mfMediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &streamInfo.audio.nchannels);
					failed = failed || FAILED(hr);

					// Bits per sample
					hr = decoderType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bps);
					failed = failed || FAILED(hr);

					if (!failed)
					{
						streamInfo.type = NAV_STREAMTYPE_AUDIO;
						streamInfo.audio.format = makeAudioFormat(bps, false, bps > 8);
					}
				}
			}
			else if (majorType == MFMediaType_Video)
			{
				failed = FAILED(backend->MFCreateMediaType(partialType.dptr()));
				nav_pixelformat pixfmt = NAV_PIXELFORMAT_UNKNOWN;

				if (!failed)
				{
					// GUID encodedType = NULL_GUID;
					// mfMediaType->GetGUID(MF_MT_SUBTYPE, &encodedType);

					// GUID bestPixelFormat = getBestPixelFormat(backend->MFTEnumEx, mfMediaType.get());
					// if (bestPixelFormat != NULL_GUID)
					// {
					// 	mfMediaType->CopyAllItems(partialType.get());
					// 	partialType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_I420);
					// 	hr = mfsr->SetCurrentMediaType(i, nullptr, partialType.get());
					// 	failed = failed || FAILED(hr);
					// }
					// else
					// 	failed = true;
					bool gotCodec = false;
					GUID codec = NULL_GUID;

					mfMediaType->GetGUID(MF_MT_SUBTYPE, &codec);
					mfMediaType->CopyAllItems(partialType.get());
					mfMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

					// Try best known combination first
					for (const KnownBestCombination &comb: knownBestCombination)
					{
						if (comb.codec != NULL_GUID && comb.codec == codec)
						{
							partialType->SetGUID(MF_MT_SUBTYPE, comb.rawformat);

							if (SUCCEEDED(mfsr->SetCurrentMediaType(i, nullptr, partialType.get())))
							{
								gotCodec = true;
								pixfmt = comb.pixfmt;
							}

							break;
						}
					}

					if (!gotCodec)
					{
						for (const MediaTypeCombination &comb: mediaTypeCombination)
						{
							partialType->SetGUID(MF_MT_SUBTYPE, comb.guid);
							if (SUCCEEDED(mfsr->SetCurrentMediaType(i, nullptr, partialType.get())))
							{
								pixfmt = comb.pixfmt;
								break;
							}
						}
					}
				}

				failed = failed || pixfmt == NAV_PIXELFORMAT_UNKNOWN;

				if (!failed)
				{
					ULARGE_INTEGER fps, dimensions;

					// Dimensions
					hr = mfMediaType->GetUINT64(MF_MT_FRAME_SIZE, &dimensions.QuadPart);
					failed = failed || FAILED(hr);

					// FPS
					hr = mfMediaType->GetUINT64(MF_MT_FRAME_RATE, &fps.QuadPart);
					failed = failed || FAILED(hr);

					if (!failed)
					{
						streamInfo.type = NAV_STREAMTYPE_VIDEO;
						streamInfo.video.fps = derationalize(fps.HighPart, fps.LowPart);
						streamInfo.video.width = dimensions.HighPart;
						streamInfo.video.height = dimensions.LowPart;
						streamInfo.video.format = pixfmt;
					}
				}
			}
			else
				mfsr->SetStreamSelection(i, FALSE);
		}

		streamInfoList.push_back(streamInfo);
	}
}

MediaFoundationState::~MediaFoundationState()
{}

size_t MediaFoundationState::getStreamCount() noexcept
{
	return streamInfoList.size();
}

nav_streaminfo_t *MediaFoundationState::getStreamInfo(size_t index) noexcept
{
	if (index >= streamInfoList.size())
	{
		nav::error::set("Stream index out of range");
		return nullptr;
	}

	return &streamInfoList[index];
}

bool MediaFoundationState::isStreamEnabled(size_t index) noexcept
{
	BOOL enabled = FALSE;

	if (index >= streamInfoList.size())
	{
		nav::error::set("Index out of range");
		return false;
	}

	if (FAILED(mfSourceReader->GetStreamSelection((DWORD) index, &enabled)))
	{
		nav::error::set("IMFSourceReader::GetStreamSelection failed");
		return false;
	}

	return (bool) enabled;
}

bool MediaFoundationState::setStreamEnabled(size_t index, bool enabled)
{
	if (index >= streamInfoList.size())
	{
		nav::error::set("Index out of range");
		return false;
	}

	if (FAILED(mfSourceReader->SetStreamSelection((DWORD) index, (BOOL) enabled)))
	{
		nav::error::set("IMFSourceReader::SetStreamSelection failed");
		return false;
	}

	return true;
}

double MediaFoundationState::getDuration() noexcept
{
	WrappedPropVariant pvar;

	if (FAILED(mfSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &pvar)))
		return -1.0;
	
	return derationalize(pvar.uhVal.QuadPart, MF_100NS_UNIT);
}

double MediaFoundationState::getPosition() noexcept
{
	return derationalize(currentPosition, MF_100NS_UNIT);
}

double MediaFoundationState::setPosition(double position)
{
	UINT64 newPos = (UINT64) (position * 1e7);
	WrappedPropVariant pvar = newPos;

	if (FAILED(mfSourceReader->SetCurrentPosition(NULL_GUID, pvar)))
	{
		nav::error::set("IMFSourceReader::SetCurrentPosition failed");
		return -1.0;
	}

	currentPosition = newPos;
	return position;
}

nav_frame_t *MediaFoundationState::read()
{
	while (true)
	{
		ComPtr<IMFSample> mfSample = nullptr;
		DWORD streamIndex, streamFlags;
		LONGLONG timestamp;

		if (FAILED(mfSourceReader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, &streamIndex, &streamFlags, &timestamp, mfSample.dptr())))
			throw std::runtime_error("MediaFoundation assertion failed");

		currentPosition = (UINT64) timestamp;

		if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
			return nullptr;
		if (mfSample)
		{
			ComPtr<IMFMediaBuffer> testbuf;
			DWORD bufcount;

			if (FAILED(mfSample->GetBufferCount(&bufcount)))
				throw std::runtime_error("MediaFoundation assertion failed");
			if (bufcount > 0)
				return decode(mfSample, streamIndex, timestamp);
		}
	}
}

nav_frame_t *MediaFoundationState::decode(ComPtr<IMFSample> &mfSample, size_t streamIndex, LONGLONG timestamp)
{
	DWORD maxbufsize, bufsize;

	ComPtr<IMFMediaBuffer> mfMediaBuffer;
	if (FAILED(mfSample->GetBufferByIndex(0, mfMediaBuffer.dptr())))
		throw std::runtime_error("IMFSample::GetBufferByIndex failed");
	
	ComPtr<IMF2DBuffer> buf2d;
	if (SUCCEEDED(mfMediaBuffer.cast(IMF2DBuffer_GUID, buf2d)))
		return decode2D(buf2d, streamIndex, timestamp);

	BYTE *source;
	if (FAILED(mfMediaBuffer->Lock(&source, &maxbufsize, &bufsize)))
		throw std::runtime_error("IMFMediaBuffer::Lock failed");
	
	if (bufsize == 0)
		bufsize = maxbufsize;

	nav::FrameVector *frame = new (std::nothrow) nav::FrameVector(
		&streamInfoList[streamIndex],
		streamIndex,
		derationalize(timestamp, (int64_t) MF_100NS_UNIT),
		source,
		bufsize
	);
	mfMediaBuffer->Unlock();
	return frame;
}

nav_frame_t *MediaFoundationState::decode2D(ComPtr<IMF2DBuffer> &buf2d, size_t streamIndex, LONGLONG timestamp)
{
	BYTE *source = nullptr;
	nav_streaminfo_t *streamInfo = &streamInfoList[streamIndex];
	size_t dstStride = streamInfo->video.stride();
	LONG stride = (LONG) dstStride;
	DWORD contSize = 0;

	if (FAILED(buf2d->GetContiguousLength(&contSize)))
		throw std::runtime_error("IMF2DBuffer::GetContiguousLength failed");

	if (FAILED(buf2d->Lock2D(&source, &stride)))
		throw std::runtime_error("IMF2DBuffer::Lock2D failed");

	nav::FrameVector *frame = new (std::nothrow) nav::FrameVector(
		streamInfo,
		streamIndex,
		derationalize(timestamp, (int64_t) MF_100NS_UNIT),
		source,
		streamInfo->video.size()
	);
	if (frame == nullptr)
	{
		buf2d->Unlock2D();
		throw std::runtime_error("Cannot create nav_frame_t");
	}

	uint8_t *frameData = (uint8_t*) frame->data();

	for (size_t y = 0; y < streamInfo->video.height; y++)
		std::copy(source + y * stride, source + (y + 1) * stride, frameData + y * dstStride);
	
	// HACK: Some pixel format doesn't provide a way to know additional padding at height.
	size_t extraHeight = bruteForceExtraHeight((uint32_t) stride, streamInfo->video.height, contSize, streamInfo->video.format);

	LONG chromaStride = stride;
	size_t chromaDstStride = dstStride;
	uint32_t chromaHeight = streamInfo->video.height;
	size_t chromaRealHeight = chromaHeight + extraHeight;

	switch (streamInfo->video.format)
	{
		case NAV_PIXELFORMAT_RGB8:
			buf2d->Unlock2D();
			return frame; // We're done
		case NAV_PIXELFORMAT_YUV420:
			chromaStride = (chromaStride + 1) / 2;
			chromaDstStride = (chromaDstStride + 1) / 2;
			[[fallthrough]];
		case NAV_PIXELFORMAT_NV12:
			chromaHeight = (chromaHeight + 1) / 2;
			chromaRealHeight = (chromaRealHeight + 1) / 2;
			break;
	}

	BYTE *uvSource = source + (streamInfo->video.height + extraHeight) * stride;
	BYTE *uvDest = frameData + streamInfo->video.height * dstStride;

	for (int i = 0; i < (streamInfo->video.format == NAV_PIXELFORMAT_NV12 ? 1 : 2); i++)
	{
		// Copy U plane (or UV for NV12)
		for (size_t y = 0; y < chromaHeight; y++)
			std::copy(uvSource + y * chromaStride, uvSource + (y + 1) * chromaStride, uvDest + y * chromaDstStride);

		uvSource += chromaRealHeight * chromaStride;
		uvDest += chromaHeight * chromaDstStride;
	}

	buf2d->Unlock2D();
	return frame;
}

MediaFoundationBackend::MediaFoundationBackend()
: mfplat("mfplat.dll")
, mfreadwrite("mfreadwrite.dll")
, callCoUninitialize(true)
, MFStartup(nullptr)
, MFShutdown(nullptr)
, MFCreateMediaType(nullptr)
, MFCreateMFByteStreamOnStream(nullptr)
, MFCreateSourceReaderFromByteStream(nullptr)
{
	if (
		!mfplat.get("MFStartup", &MFStartup) ||
		!mfplat.get("MFShutdown", &MFShutdown) ||
		!mfplat.get("MFCreateMediaType", &MFCreateMediaType) ||
		!mfplat.get("MFCreateMFByteStreamOnStream", &MFCreateMFByteStreamOnStream) ||
		!mfreadwrite.get("MFCreateSourceReaderFromByteStream", &MFCreateSourceReaderFromByteStream)
	)
		throw std::runtime_error("cannot load MediaFoundation function pointer");

	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
		throw std::runtime_error("CoInitializeEx failed");

	callCoUninitialize = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;

	if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET)))
	{
		if (callCoUninitialize)
			CoUninitialize();
		throw std::runtime_error("MFStartup failed");
	}
}

MediaFoundationBackend::~MediaFoundationBackend()	
{
	MFShutdown();
	if (callCoUninitialize)
		CoUninitialize();
}

State *MediaFoundationBackend::open(nav_input *input, const char *filename)
{
	input->seekf(0);

	ComPtr<NavInputStream> istream(new NavInputStream(input, filename), false);		
	ComPtr<IMFByteStream> byteStream;

	if (FAILED(MFCreateMFByteStreamOnStream(istream.get(), byteStream.dptr())))
		throw std::runtime_error("MFCreateMFByteStreamOnStream failed");

	if (filename)
	{
		// Try set the filename
		ComPtr<IMFAttributes> attrs;

		if (SUCCEEDED(byteStream.cast(IMFAttributes_GUID, attrs)))
		{
			std::wstring wfilename = nav::fromUTF8(filename);
			attrs->SetString(MF_BYTESTREAM_ORIGIN_NAME, wfilename.c_str());
		}
	}

	ComPtr<IMFSourceReader> sourceReader;
	if (FAILED(MFCreateSourceReaderFromByteStream(byteStream.get(), nullptr, sourceReader.dptr())))
		throw std::runtime_error("MFCreateSourceReaderFromByteStream failed");

	// All good
	return new MediaFoundationState(this, input, istream, byteStream, sourceReader);
}

const char *MediaFoundationBackend::getName() const noexcept
{
	return "mediafoundation";
}

nav_backendtype MediaFoundationBackend::getType() const noexcept
{
	return NAV_BACKENDTYPE_OS_API;
}

const char *MediaFoundationBackend::getInfo()
{
	return nullptr;
}

Backend *create()
{
	try
	{
		return new MediaFoundationBackend();
	}
	catch(const std::exception&)
	{
		return nullptr;
	}
}

}

#endif /* NAV_BACKEND_MEDIAFOUNDATION */
