#include "StdAfx.h"
#include "EterBase/Stl.h"
#include "GrpTexture.h"
#include "StateManager.h"

void CGraphicTexture::DestroyDeviceObjects()
{
	safe_release(m_lpSRV);
}

void CGraphicTexture::Destroy()
{
	DestroyDeviceObjects();

	Initialize();
}

void CGraphicTexture::Initialize()
{
	m_lpSRV = NULL;
	m_width = 0;
	m_height = 0;
	m_bEmpty = true;
}

bool CGraphicTexture::IsEmpty() const
{
	return m_bEmpty;
}

void CGraphicTexture::SetTextureStage(int stage) const
{
	STATEMANAGER.SetTexture(stage, m_lpSRV);
}

ID3D11ShaderResourceView* CGraphicTexture::GetSRV() const
{
	return m_lpSRV;
}

int CGraphicTexture::GetWidth() const
{
	return m_width;
}

int CGraphicTexture::GetHeight() const
{
	return m_height;
}

CGraphicTexture::CGraphicTexture()
{
	Initialize();
}

CGraphicTexture::~CGraphicTexture()
{
}
