#include "StdAfx.h"
#include "PackLib/PackManager.h"
#include "GrpImageTexture.h"
#include "EterBase/Stl.h"
#include "DecodedImageData.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>

#if defined(_M_IX86) || defined(_M_X64)
#include <emmintrin.h>  // SSE2
#include <tmmintrin.h>  // SSSE3
#endif

///////////////////////////////////////////////////////////////////////////////
// Helper: create D3D11 SRV from BGRA pixel data
///////////////////////////////////////////////////////////////////////////////
bool CGraphicImageTexture::CreateSRVFromBGRA(UINT width, UINT height, const void* pPixels, UINT pitch)
{
	if (!ms_lpd3d11Device || !pPixels)
		return false;

	D3D11_TEXTURE2D_DESC td = {};
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = pPixels;
	sd.SysMemPitch = pitch;

	ID3D11Texture2D* pTex = NULL;
	HRESULT hr = ms_lpd3d11Device->CreateTexture2D(&td, &sd, &pTex);
	if (FAILED(hr))
		return false;

	hr = ms_lpd3d11Device->CreateShaderResourceView(pTex, NULL, &m_lpSRV);
	pTex->Release();

	return SUCCEEDED(hr);
}

///////////////////////////////////////////////////////////////////////////////
// Lock/Unlock — for font textures that update dynamically
///////////////////////////////////////////////////////////////////////////////
bool CGraphicImageTexture::Lock(int* pRetPitch, void** ppRetPixels, int level)
{
	if (m_lockBuffer.empty())
	{
		m_lockPitch = m_width * 4;
		m_lockBuffer.resize(m_lockPitch * m_height, 0);
	}

	*pRetPitch = (int)m_lockPitch;
	*ppRetPixels = m_lockBuffer.data();
	return true;
}

void CGraphicImageTexture::Unlock(int level)
{
	// Recreate the SRV from updated pixel data
	safe_release(m_lpSRV);
	CreateSRVFromBGRA(m_width, m_height, m_lockBuffer.data(), m_lockPitch);
}

///////////////////////////////////////////////////////////////////////////////
// Initialize / Destroy
///////////////////////////////////////////////////////////////////////////////
void CGraphicImageTexture::Initialize()
{
	CGraphicTexture::Initialize();

	m_stFileName = "";
	m_d3dFmt = D3DFMT_UNKNOWN;
	m_dwFilter = 0;
	m_lockPitch = 0;
}

void CGraphicImageTexture::Destroy()
{
	CGraphicTexture::Destroy();
	m_lockBuffer.clear();
	m_lockBuffer.shrink_to_fit();
	Initialize();
}

///////////////////////////////////////////////////////////////////////////////
// CreateDeviceObjects — called for re-creation (e.g. font textures)
///////////////////////////////////////////////////////////////////////////////
bool CGraphicImageTexture::CreateDeviceObjects()
{
	assert(ms_lpd3d11Device != NULL);

	if (m_stFileName.empty())
	{
		// Font texture — create empty, will be filled via Lock/Unlock
		m_lockPitch = m_width * 4;
		m_lockBuffer.resize(m_lockPitch * m_height, 0);
		CreateSRVFromBGRA(m_width, m_height, m_lockBuffer.data(), m_lockPitch);
		m_bEmpty = false;
		return true;
	}
	else
	{
		TPackFile mappedFile;
		if (!CPackManager::Instance().GetFile(m_stFileName, mappedFile))
			return false;

		return CreateFromMemoryFile(mappedFile.size(), mappedFile.data(), m_d3dFmt, m_dwFilter);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Create empty texture (for font textures)
///////////////////////////////////////////////////////////////////////////////
bool CGraphicImageTexture::Create(UINT width, UINT height, D3DFORMAT d3dFmt, DWORD dwFilter)
{
	Destroy();

	m_width = width;
	m_height = height;
	m_d3dFmt = d3dFmt;
	m_dwFilter = dwFilter;

	return CreateDeviceObjects();
}

///////////////////////////////////////////////////////////////////////////////
// CreateFromTexturePointer — share another texture's SRV
///////////////////////////////////////////////////////////////////////////////
void CGraphicImageTexture::CreateFromTexturePointer(const CGraphicTexture* c_pSrcTexture)
{
	safe_release(m_lpSRV);

	m_width = c_pSrcTexture->GetWidth();
	m_height = c_pSrcTexture->GetHeight();
	m_lpSRV = c_pSrcTexture->GetSRV();

	if (m_lpSRV)
		m_lpSRV->AddRef();

	m_bEmpty = false;
}

///////////////////////////////////////////////////////////////////////////////
// DDS loading — parse DDS header, create D3D11 texture directly
///////////////////////////////////////////////////////////////////////////////
bool CGraphicImageTexture::CreateFromDDSMemory(UINT bufSize, const void* c_pvBuf)
{
	if (!ms_lpd3d11Device || bufSize < 128)
		return false;

	const uint8_t* pData = (const uint8_t*)c_pvBuf;

	// Verify DDS magic
	if (pData[0] != 'D' || pData[1] != 'D' || pData[2] != 'S' || pData[3] != ' ')
		return false;

	// Parse DDS header
	// DDS_HEADER layout (after 4-byte magic):
	//   [0]  dwSize             [1]  dwFlags
	//   [2]  dwHeight           [3]  dwWidth
	//   [4]  dwPitchOrLinearSize [5] dwDepth
	//   [6]  dwMipMapCount
	//   [7..17] dwReserved1[11]
	//   [18] ddspf.dwSize       [19] ddspf.dwFlags
	//   [20] ddspf.dwFourCC     [21] ddspf.dwRGBBitCount
	const uint32_t* header = (const uint32_t*)(pData + 4);
	uint32_t height = header[2];
	uint32_t width = header[3];
	uint32_t mipCount = header[6];
	if (mipCount == 0) mipCount = 1;
	uint32_t pfFlags = header[19];
	uint32_t pfFourCC = header[20];
	uint32_t pfBitCount = header[21];
	uint32_t pfRBitMask = header[22];
	uint32_t pfGBitMask = header[23];
	uint32_t pfBBitMask = header[24];
	uint32_t pfABitMask = header[25];

	DXGI_FORMAT dxgiFormat;
	uint32_t blockSize = 0;	// 0 = uncompressed, otherwise bytes per block (BC1=8, BC2/3=16)
	uint32_t bytesPerPixel = 0;	// for uncompressed
	const uint8_t* pixelData = pData + 4 + 124; // after magic + header
	uint32_t pixelDataSize = bufSize - 128;

	if (pfFlags & 0x4) // DDPF_FOURCC
	{
		char fourcc[5] = {};
		memcpy(fourcc, &pfFourCC, 4);

		if (strcmp(fourcc, "DXT1") == 0)
		{
			dxgiFormat = DXGI_FORMAT_BC1_UNORM;
			blockSize = 8;
		}
		else if (strcmp(fourcc, "DXT3") == 0)
		{
			dxgiFormat = DXGI_FORMAT_BC2_UNORM;
			blockSize = 16;
		}
		else if (strcmp(fourcc, "DXT5") == 0)
		{
			dxgiFormat = DXGI_FORMAT_BC3_UNORM;
			blockSize = 16;
		}
		else
		{
			return false; // Unknown compressed format
		}
	}
	else if (pfFlags & 0x40) // DDPF_RGB
	{
		if (pfBitCount == 32)
		{
			dxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
			bytesPerPixel = 4;
		}
		else if (pfBitCount == 24)
		{
			return false;	// 24-bit RGB unsupported in D3D11, fall through to STB
		}
		else if (pfBitCount == 16)
		{
			bool hasAlpha = (pfFlags & 0x1) != 0; // DDPF_ALPHAPIXELS
			if (hasAlpha)
			{
				// 16-bit with alpha (A4R4G4B4, A1R5G5B5, etc.)
				// D3D11 has limited 16-bit alpha format support, so convert to 32-bit BGRA.
				// Build a temporary BGRA8888 buffer and use that instead.
				uint32_t rowPitch16 = width * 2;
				uint32_t totalPixels = width * height;
				std::vector<uint32_t> bgra(totalPixels);
				const uint16_t* pSrc16 = (const uint16_t*)pixelData;

				if (pfABitMask == 0xF000) // A4R4G4B4
				{
					for (uint32_t p = 0; p < totalPixels; ++p)
					{
						uint16_t c = pSrc16[p];
						uint8_t a = (uint8_t)(((c >> 12) & 0xF) * 255 / 15);
						uint8_t r = (uint8_t)(((c >> 8)  & 0xF) * 255 / 15);
						uint8_t g = (uint8_t)(((c >> 4)  & 0xF) * 255 / 15);
						uint8_t b = (uint8_t)(( c        & 0xF) * 255 / 15);
						bgra[p] = (a << 24) | (r << 16) | (g << 8) | b;
					}
				}
				else // A1R5G5B5 or other 16-bit alpha formats
				{
					for (uint32_t p = 0; p < totalPixels; ++p)
					{
						uint16_t c = pSrc16[p];
						uint8_t a = (c & 0x8000) ? 255 : 0;
						uint8_t r = (uint8_t)(((c >> 10) & 0x1F) * 255 / 31);
						uint8_t g = (uint8_t)(((c >> 5)  & 0x1F) * 255 / 31);
						uint8_t b = (uint8_t)(( c        & 0x1F) * 255 / 31);
						bgra[p] = (a << 24) | (r << 16) | (g << 8) | b;
					}
				}

				// Create SRV from converted BGRA data (mip 0 only)
				m_width = width;
				m_height = height;
				return CreateSRVFromBGRA(width, height, bgra.data(), width * 4);
			}
			else
			{
				dxgiFormat = DXGI_FORMAT_B5G6R5_UNORM;
				bytesPerPixel = 2;
			}
		}
		else
		{
			return false;
		}
	}
	else if (pfFlags & 0x20000) // DDPF_LUMINANCE
	{
		if (pfBitCount == 8)
		{
			dxgiFormat = DXGI_FORMAT_R8_UNORM;
			bytesPerPixel = 1;
		}
		else if (pfBitCount == 16 && (pfFlags & 0x1)) // A8L8 (luminance + alpha)
		{
			uint32_t totalPixels = width * height;
			std::vector<uint32_t> bgra(totalPixels);
			const uint8_t* pSrc = pixelData;
			for (uint32_t p = 0; p < totalPixels; ++p)
			{
				uint8_t lum = pSrc[p * 2];
				uint8_t alp = pSrc[p * 2 + 1];
				bgra[p] = (alp << 24) | (lum << 16) | (lum << 8) | lum;
			}
			m_width = width;
			m_height = height;
			return CreateSRVFromBGRA(width, height, bgra.data(), width * 4);
		}
		else
		{
			return false;
		}
	}
	else if (pfFlags & 0x2) // DDPF_ALPHA
	{
		dxgiFormat = DXGI_FORMAT_A8_UNORM;
		bytesPerPixel = 1;
	}
	else
	{
		return false;
	}

	// Build subresource data for each mip level
	const UINT MAX_MIPS = 16;
	if (mipCount > MAX_MIPS)
		mipCount = MAX_MIPS;

	D3D11_SUBRESOURCE_DATA mipData[MAX_MIPS] = {};

	uint32_t mipW = width;
	uint32_t mipH = height;
	const uint8_t* curPtr = pixelData;
	const uint8_t* endPtr = pData + bufSize;

	for (UINT i = 0; i < mipCount; ++i)
	{
		uint32_t mipPitch;
		uint32_t mipBytes;

		if (blockSize > 0)
		{
			// Compressed: minimum block dimensions of 4x4
			uint32_t blocksW = (mipW + 3) / 4;
			uint32_t blocksH = (mipH + 3) / 4;
			if (blocksW < 1) blocksW = 1;
			if (blocksH < 1) blocksH = 1;
			mipPitch = blocksW * blockSize;
			mipBytes = blocksW * blocksH * blockSize;
		}
		else
		{
			mipPitch = mipW * bytesPerPixel;
			mipBytes = mipPitch * mipH;
		}

		// Bounds check
		if (curPtr + mipBytes > endPtr)
		{
			// Truncated mip data — clamp to actual mip count we read
			mipCount = i;
			break;
		}

		mipData[i].pSysMem = curPtr;
		mipData[i].SysMemPitch = mipPitch;
		mipData[i].SysMemSlicePitch = mipBytes;

		curPtr += mipBytes;

		if (mipW > 1) mipW >>= 1;
		if (mipH > 1) mipH >>= 1;
	}

	if (mipCount == 0)
		return false;

	// Create D3D11 texture with all mip levels
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = width;
	td.Height = height;
	td.MipLevels = mipCount;
	td.ArraySize = 1;
	td.Format = dxgiFormat;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_IMMUTABLE;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	ID3D11Texture2D* pTex = NULL;
	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&td, mipData, &pTex)))
		return false;

	HRESULT hr = ms_lpd3d11Device->CreateShaderResourceView(pTex, NULL, &m_lpSRV);
	pTex->Release();

	if (FAILED(hr))
		return false;

	m_width = width;
	m_height = height;
	m_bEmpty = false;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// STB loading — decode PNG/TGA/JPG/BMP to BGRA, create SRV
///////////////////////////////////////////////////////////////////////////////
bool CGraphicImageTexture::CreateFromSTBMemory(UINT bufSize, const void* c_pvBuf)
{
	int width, height, channels;
	unsigned char* data = stbi_load_from_memory((stbi_uc*)c_pvBuf, bufSize, &width, &height, &channels, 4);
	if (!data)
		return false;

	// Convert RGBA → BGRA for D3D11 B8G8R8A8_UNORM
	size_t pixelCount = (size_t)width * height;

	#if defined(_M_IX86) || defined(_M_X64)
	{
		size_t simdPixels = pixelCount & ~3;
		__m128i shuffle_mask = _mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15);

		for (size_t i = 0; i < simdPixels; i += 4)
		{
			__m128i pixels = _mm_loadu_si128((__m128i*)(data + i * 4));
			pixels = _mm_shuffle_epi8(pixels, shuffle_mask);
			_mm_storeu_si128((__m128i*)(data + i * 4), pixels);
		}

		for (size_t i = simdPixels; i < pixelCount; ++i)
		{
			size_t idx = i * 4;
			uint8_t r = data[idx + 0];
			data[idx + 0] = data[idx + 2];
			data[idx + 2] = r;
		}
	}
	#else
	for (size_t i = 0; i < pixelCount; ++i)
	{
		size_t idx = i * 4;
		uint8_t r = data[idx + 0];
		data[idx + 0] = data[idx + 2];
		data[idx + 2] = r;
	}
	#endif

	bool ok = CreateSRVFromBGRA(width, height, data, width * 4);
	stbi_image_free(data);

	if (ok)
	{
		m_width = width;
		m_height = height;
		m_bEmpty = false;
	}

	return ok;
}

///////////////////////////////////////////////////////////////////////////////
// CreateFromMemoryFile — main entry point, tries DDS then STB
///////////////////////////////////////////////////////////////////////////////
bool CGraphicImageTexture::CreateFromMemoryFile(UINT bufSize, const void* c_pvBuf, D3DFORMAT d3dFmt, DWORD dwFilter)
{
	assert(ms_lpd3d11Device != NULL);

	// Release any existing SRV before creating a new one (avoid leak + dangling pointer if Create fails)
	safe_release(m_lpSRV);
	m_bEmpty = true;

	if (CreateFromDDSMemory(bufSize, c_pvBuf))
		return true;

	if (CreateFromSTBMemory(bufSize, c_pvBuf))
		return true;

	TraceError("CreateFromMemoryFile: Cannot create texture (%s, %u bytes)", m_stFileName.c_str(), bufSize);
	return false;
}

void CGraphicImageTexture::SetFileName(const char* c_szFileName)
{
	m_stFileName = c_szFileName;
}

bool CGraphicImageTexture::CreateFromDiskFile(const char* c_szFileName, D3DFORMAT d3dFmt, DWORD dwFilter)
{
	Destroy();

	SetFileName(c_szFileName);

	m_d3dFmt = d3dFmt;
	m_dwFilter = dwFilter;
	return CreateDeviceObjects();
}

bool CGraphicImageTexture::CreateFromDecodedData(const TDecodedImageData& decodedImage, D3DFORMAT d3dFmt, DWORD dwFilter)
{
	assert(ms_lpd3d11Device != NULL);

	// Release any existing SRV before creating a new one
	safe_release(m_lpSRV);

	if (!decodedImage.IsValid())
		return false;

	m_bEmpty = true;

	if (decodedImage.isDDS)
	{
		if (!CreateFromDDSMemory(decodedImage.pixels.size(), decodedImage.pixels.data()))
			return false;
	}
	else if (decodedImage.format == TDecodedImageData::FORMAT_RGBA8)
	{
		// Convert RGBA → BGRA
		std::vector<uint8_t> bgra(decodedImage.pixels.begin(), decodedImage.pixels.end());
		size_t pixelCount = (size_t)decodedImage.width * decodedImage.height;

		#if defined(_M_IX86) || defined(_M_X64)
		{
			size_t simdPixels = pixelCount & ~3;
			__m128i shuffle_mask = _mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15);

			for (size_t i = 0; i < simdPixels; i += 4)
			{
				__m128i pixels = _mm_loadu_si128((__m128i*)(bgra.data() + i * 4));
				pixels = _mm_shuffle_epi8(pixels, shuffle_mask);
				_mm_storeu_si128((__m128i*)(bgra.data() + i * 4), pixels);
			}

			for (size_t i = simdPixels; i < pixelCount; ++i)
			{
				size_t idx = i * 4;
				uint8_t r = bgra[idx + 0];
				bgra[idx + 0] = bgra[idx + 2];
				bgra[idx + 2] = r;
			}
		}
		#else
		for (size_t i = 0; i < pixelCount; ++i)
		{
			size_t idx = i * 4;
			uint8_t r = bgra[idx + 0];
			bgra[idx + 0] = bgra[idx + 2];
			bgra[idx + 2] = r;
		}
		#endif

		if (!CreateSRVFromBGRA(decodedImage.width, decodedImage.height, bgra.data(), decodedImage.width * 4))
			return false;

		m_width = decodedImage.width;
		m_height = decodedImage.height;
		m_bEmpty = false;
	}
	else
	{
		TraceError("CreateFromDecodedData: Unsupported decoded image format");
		return false;
	}

	return !m_bEmpty;
}

CGraphicImageTexture::CGraphicImageTexture()
{
	Initialize();
}

CGraphicImageTexture::~CGraphicImageTexture()
{
	Destroy();
}
