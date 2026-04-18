#include "StdAfx.h"
#include "EterBase/Stl.h"
#include "GrpIndexBuffer.h"
#include "StateManager.h"

ID3D11Buffer* CGraphicIndexBuffer::GetD3DIndexBuffer() const
{
	return m_lpd3dIdxBuf;
}

DXGI_FORMAT CGraphicIndexBuffer::GetDXGIFormat() const
{
	return (m_d3dFmt == D3DFMT_INDEX32) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
}

void CGraphicIndexBuffer::SetIndices(int startIndex) const
{
	if (!ms_lpd3d11Context || !m_lpd3dIdxBuf)
		return;

	UINT bytesPerIndex = (m_d3dFmt == D3DFMT_INDEX32) ? 4u : 2u;
	UINT offset = (UINT)startIndex * bytesPerIndex;
	ms_lpd3d11Context->IASetIndexBuffer(m_lpd3dIdxBuf, GetDXGIFormat(), offset);
}

bool CGraphicIndexBuffer::Lock(void** pretIndices) const
{
	if (!m_lpd3dIdxBuf)
		return false;

	m_cpuStaging.resize(m_dwBufferSize);
	*pretIndices = m_cpuStaging.data();
	m_bLocked = true;
	return true;
}

void CGraphicIndexBuffer::Unlock() const
{
	if (!m_lpd3dIdxBuf || !m_bLocked || !ms_lpd3d11Context)
		return;

	ms_lpd3d11Context->UpdateSubresource(m_lpd3dIdxBuf, 0, NULL, m_cpuStaging.data(), m_dwBufferSize, 0);
	m_bLocked = false;
}

bool CGraphicIndexBuffer::Lock(void** pretIndices)
{
	return const_cast<const CGraphicIndexBuffer*>(this)->Lock(pretIndices);
}

void CGraphicIndexBuffer::Unlock()
{
	const_cast<const CGraphicIndexBuffer*>(this)->Unlock();
}

bool CGraphicIndexBuffer::Copy(int bufSize, const void* srcIndices)
{
	void* dstIndices;
	if (!Lock(&dstIndices))
		return false;

	memcpy(dstIndices, srcIndices, bufSize);
	Unlock();
	return true;
}

bool CGraphicIndexBuffer::Create(int faceCount, TFace* faces)
{
	int idxCount = faceCount * 3;
	if (!Create(idxCount, D3DFMT_INDEX16))
		return false;

	void* dst;
	if (!Lock(&dst))
		return false;

	WORD* dstIndices = (WORD*)dst;
	for (int i = 0; i < faceCount; ++i, dstIndices += 3)
	{
		TFace* curFace = faces + i;
		dstIndices[0] = curFace->indices[0];
		dstIndices[1] = curFace->indices[1];
		dstIndices[2] = curFace->indices[2];
	}

	Unlock();
	return true;
}

bool CGraphicIndexBuffer::CreateDeviceObjects()
{
	if (!ms_lpd3d11Device)
		return false;

	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = m_dwBufferSize;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	if (FAILED(ms_lpd3d11Device->CreateBuffer(&bd, NULL, &m_lpd3dIdxBuf)))
		return false;

	return true;
}

void CGraphicIndexBuffer::DestroyDeviceObjects()
{
	safe_release(m_lpd3dIdxBuf);
}

bool CGraphicIndexBuffer::Create(int idxCount, D3DFORMAT d3dFmt)
{
	Destroy();

	m_iidxCount = idxCount;
	UINT bytesPerIndex = (d3dFmt == D3DFMT_INDEX32) ? 4u : 2u;
	m_dwBufferSize = bytesPerIndex * idxCount;
	m_d3dFmt = d3dFmt;

	return CreateDeviceObjects();
}

void CGraphicIndexBuffer::Destroy()
{
	DestroyDeviceObjects();
	m_cpuStaging.clear();
	m_cpuStaging.shrink_to_fit();
}

void CGraphicIndexBuffer::Initialize()
{
	m_lpd3dIdxBuf = NULL;
	m_dwBufferSize = 0;
	m_d3dFmt = D3DFMT_INDEX16;
	m_iidxCount = 0;
	m_bLocked = false;
}

CGraphicIndexBuffer::CGraphicIndexBuffer()
{
	Initialize();
}

CGraphicIndexBuffer::~CGraphicIndexBuffer()
{
	Destroy();
}
