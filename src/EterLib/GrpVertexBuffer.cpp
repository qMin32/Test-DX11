#include "StdAfx.h"
#include "EterBase/Stl.h"
#include "GrpVertexBuffer.h"
#include "StateManager.h"

///////////////////////////////////////////////////////////////////////////////
// FVF stride calculation (D3D11 has no equivalent of D3DXGetFVFVertexSize)
///////////////////////////////////////////////////////////////////////////////
UINT CGraphicVertexBuffer::GetFVFStride(DWORD fvf)
{
	UINT size = 0;

	if (fvf & D3DFVF_XYZRHW)
		size += 16;	// float4
	else if (fvf & D3DFVF_XYZ)
		size += 12;	// float3

	if (fvf & D3DFVF_NORMAL)
		size += 12;	// float3

	if (fvf & D3DFVF_PSIZE)
		size += 4;

	if (fvf & D3DFVF_DIFFUSE)
		size += 4;

	if (fvf & D3DFVF_SPECULAR)
		size += 4;

	UINT texCount = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	size += texCount * 8;	// float2 per texcoord (assumes 2D coords)

	return size;
}

int CGraphicVertexBuffer::GetVertexStride() const
{
	return (int)GetFVFStride(m_dwFVF);
}

DWORD CGraphicVertexBuffer::GetFlexibleVertexFormat() const
{
	return m_dwFVF;
}

int CGraphicVertexBuffer::GetVertexCount() const
{
	return m_vtxCount;
}

void CGraphicVertexBuffer::SetStream(int stride, int layer) const
{
	if (!ms_lpd3d11Context || !m_lpd3dVB)
		return;

	UINT uStride = (UINT)stride;
	UINT uOffset = 0;
	ms_lpd3d11Context->IASetVertexBuffers(layer, 1, &m_lpd3dVB, &uStride, &uOffset);
}

///////////////////////////////////////////////////////////////////////////////
// Lock / Unlock
//
// D3D11 buffers don't Lock like D3D9. We use:
//   - Dynamic buffers: Map(WRITE_DISCARD) → returns GPU pointer directly
//   - Static buffers: CPU staging buffer, copy to GPU on Unlock via UpdateSubresource
///////////////////////////////////////////////////////////////////////////////
bool CGraphicVertexBuffer::LockRange(unsigned count, void** pretVertices) const
{
	if (!m_lpd3dVB || !ms_lpd3d11Context)
		return false;

	DWORD lockSize = GetVertexStride() * count;

	if (m_bDynamic)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		if (FAILED(ms_lpd3d11Context->Map(m_lpd3dVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			return false;
		*pretVertices = mapped.pData;
		m_bLocked = true;
		return true;
	}
	else
	{
		m_cpuStaging.resize(m_dwBufferSize);
		*pretVertices = m_cpuStaging.data();
		m_bLocked = true;
		return true;
	}
}

bool CGraphicVertexBuffer::Lock(void** pretVertices) const
{
	return LockRange(m_vtxCount, pretVertices);
}

bool CGraphicVertexBuffer::Unlock() const
{
	if (!m_lpd3dVB || !m_bLocked || !ms_lpd3d11Context)
		return false;

	if (m_bDynamic)
	{
		ms_lpd3d11Context->Unmap(m_lpd3dVB, 0);
	}
	else
	{
		ms_lpd3d11Context->UpdateSubresource(m_lpd3dVB, 0, NULL, m_cpuStaging.data(), m_dwBufferSize, 0);
	}

	m_bLocked = false;
	return true;
}

bool CGraphicVertexBuffer::LockDynamic(void** pretVertices)
{
	if (!m_lpd3dVB || !ms_lpd3d11Context)
		return false;

	D3D11_MAPPED_SUBRESOURCE mapped;
	if (FAILED(ms_lpd3d11Context->Map(m_lpd3dVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		return false;

	*pretVertices = mapped.pData;
	m_bLocked = true;
	return true;
}

bool CGraphicVertexBuffer::Lock(void** pretVertices)
{
	return const_cast<const CGraphicVertexBuffer*>(this)->Lock(pretVertices);
}

bool CGraphicVertexBuffer::Unlock()
{
	return const_cast<const CGraphicVertexBuffer*>(this)->Unlock();
}

bool CGraphicVertexBuffer::Copy(int bufSize, const void* srcVertices)
{
	void* dstVertices;
	if (!Lock(&dstVertices))
		return false;

	memcpy(dstVertices, srcVertices, bufSize);
	Unlock();
	return true;
}

bool CGraphicVertexBuffer::IsEmpty() const
{
	return m_lpd3dVB == nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// Create / Destroy
///////////////////////////////////////////////////////////////////////////////
bool CGraphicVertexBuffer::CreateDeviceObjects()
{
	if (!ms_lpd3d11Device)
		return false;

	assert(m_lpd3dVB == NULL);

	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = m_dwBufferSize;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	if (m_bDynamic)
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
	}

	HRESULT hr = ms_lpd3d11Device->CreateBuffer(&bd, NULL, &m_lpd3dVB);
	if (FAILED(hr))
		return false;

	return true;
}

void CGraphicVertexBuffer::DestroyDeviceObjects()
{
	safe_release(m_lpd3dVB);
}

bool CGraphicVertexBuffer::Create(int vtxCount, DWORD fvf, DWORD usage, D3DPOOL d3dPool)
{
	assert(vtxCount > 0);

	Destroy();

	m_vtxCount = vtxCount;
	m_dwBufferSize = GetFVFStride(fvf) * m_vtxCount;
	m_d3dPool = d3dPool;
	m_dwUsage = usage;
	m_dwFVF = fvf;

	// Treat D3DUSAGE_DYNAMIC as dynamic, everything else as default
	m_bDynamic = (usage & D3DUSAGE_DYNAMIC) != 0;

	if (usage == D3DUSAGE_WRITEONLY || (usage & D3DUSAGE_DYNAMIC))
		m_dwLockFlag = 0;
	else
		m_dwLockFlag = D3DLOCK_READONLY;

	return CreateDeviceObjects();
}

void CGraphicVertexBuffer::Destroy()
{
	DestroyDeviceObjects();
	m_cpuStaging.clear();
	m_cpuStaging.shrink_to_fit();
}

void CGraphicVertexBuffer::Initialize()
{
	m_lpd3dVB = NULL;
	m_vtxCount = 0;
	m_dwBufferSize = 0;
	m_dwFVF = 0;
	m_dwUsage = 0;
	m_d3dPool = D3DPOOL_DEFAULT;
	m_dwLockFlag = 0;
	m_bLocked = false;
	m_bDynamic = false;
}

CGraphicVertexBuffer::CGraphicVertexBuffer()
{
	Initialize();
}

CGraphicVertexBuffer::~CGraphicVertexBuffer()
{
	Destroy();
}
