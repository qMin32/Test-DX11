#pragma once

#include "GrpTexture.h"

struct TDecodedImageData;

class CGraphicImageTexture : public CGraphicTexture
{
	public:
		CGraphicImageTexture();
		virtual ~CGraphicImageTexture();

		void		Destroy();

		bool		Create(UINT width, UINT height, D3DFORMAT d3dFmt, DWORD dwFilter = D3DX_FILTER_LINEAR);
		bool		CreateDeviceObjects();

		void		CreateFromTexturePointer(const CGraphicTexture* c_pSrcTexture);
		bool		CreateFromDiskFile(const char* c_szFileName, D3DFORMAT d3dFmt, DWORD dwFilter = D3DX_FILTER_LINEAR);
		bool		CreateFromMemoryFile(UINT bufSize, const void* c_pvBuf, D3DFORMAT d3dFmt, DWORD dwFilter = D3DX_FILTER_LINEAR);
		bool		CreateFromDDSMemory(UINT bufSize, const void* c_pvBuf);
		bool		CreateFromSTBMemory(UINT bufSize, const void* c_pvBuf);
		bool		CreateFromDecodedData(const TDecodedImageData& decodedImage, D3DFORMAT d3dFmt, DWORD dwFilter);

		void		SetFileName(const char * c_szFileName);

		bool		Lock(int* pRetPitch, void** ppRetPixels, int level=0);
		void		Unlock(int level=0);

	protected:
		void		Initialize();

		// Helper: create D3D11 SRV from BGRA pixel data
		bool		CreateSRVFromBGRA(UINT width, UINT height, const void* pPixels, UINT pitch);

		D3DFORMAT	m_d3dFmt;
		DWORD		m_dwFilter;

		std::string m_stFileName;

		// CPU-side pixel copy for Lock/Unlock (font textures)
		std::vector<uint8_t>	m_lockBuffer;
		UINT					m_lockPitch;
};
