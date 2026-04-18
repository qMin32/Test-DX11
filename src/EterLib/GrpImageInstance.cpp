#include "StdAfx.h"
#include "GrpImageInstance.h"
#include "StateManager.h"

#include "EterBase/CRC32.h"

CDynamicPool<CGraphicImageInstance>		CGraphicImageInstance::ms_kPool;

void CGraphicImageInstance::CreateSystem(UINT uCapacity)
{
	ms_kPool.Create(uCapacity);
}

void CGraphicImageInstance::DestroySystem()
{
	ms_kPool.Destroy();
}

CGraphicImageInstance* CGraphicImageInstance::New()
{
	return ms_kPool.Alloc();
}

void CGraphicImageInstance::Delete(CGraphicImageInstance* pkImgInst)
{
	pkImgInst->Destroy();
	ms_kPool.Free(pkImgInst);
}

void CGraphicImageInstance::Render()
{
	if (IsEmpty())
		return;

	assert(!IsEmpty());

	OnRender();
}

void CGraphicImageInstance::OnRender()
{
	CGraphicImage* pImage = m_roImage.GetPointer();
	if (!pImage)
		return;

	CGraphicTexture* pTexture = pImage->GetTexturePointer();
	if (!pTexture || !pTexture->GetSRV())
		return;

	float fimgWidth = (float)pImage->GetWidth();
	float fimgHeight = (float)pImage->GetHeight();

	const RECT& c_rRect = pImage->GetRectReference();
	float texReverseWidth = 1.0f / float(pTexture->GetWidth());
	float texReverseHeight = 1.0f / float(pTexture->GetHeight());

	float su = c_rRect.left * texReverseWidth;
	float sv = c_rRect.top * texReverseHeight;
	float eu = c_rRect.right * texReverseWidth;
	float ev = c_rRect.bottom * texReverseHeight;

	TPDTVertex vertices[4];

	vertices[0].position.x = m_v2Position.x;
	vertices[0].position.y = m_v2Position.y;
	vertices[0].position.z = 0.0f;
	vertices[0].texCoord = TTextureCoordinate(su, sv);
	vertices[0].diffuse = m_DiffuseColor;

	vertices[1].position.x = m_v2Position.x + fimgWidth;
	vertices[1].position.y = m_v2Position.y;
	vertices[1].position.z = 0.0f;
	vertices[1].texCoord = TTextureCoordinate(eu, sv);
	vertices[1].diffuse = m_DiffuseColor;

	vertices[2].position.x = m_v2Position.x;
	vertices[2].position.y = m_v2Position.y + fimgHeight;
	vertices[2].position.z = 0.0f;
	vertices[2].texCoord = TTextureCoordinate(su, ev);
	vertices[2].diffuse = m_DiffuseColor;

	vertices[3].position.x = m_v2Position.x + fimgWidth;
	vertices[3].position.y = m_v2Position.y + fimgHeight;
	vertices[3].position.z = 0.0f;
	vertices[3].texCoord = TTextureCoordinate(eu, ev);
	vertices[3].diffuse = m_DiffuseColor;

	STATEMANAGER.SaveRenderState(RS11_CULLMODE, D3D11_CULL_NONE);
	STATEMANAGER.SaveRenderState(RS11_ZENABLE, FALSE);
	STATEMANAGER.SaveRenderState(RS11_ZWRITEENABLE, FALSE);
	STATEMANAGER.SaveRenderState(RS11_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_SRCBLEND, D3D11_BLEND_SRC_ALPHA);
	STATEMANAGER.SaveRenderState(RS11_DESTBLEND, D3D11_BLEND_INV_SRC_ALPHA);
	STATEMANAGER.SaveRenderState(RS11_ALPHATESTENABLE, FALSE);

	if (CGraphicBase::SetPDTStream(vertices, 4))
	{
		CGraphicBase::SetDefaultIndexBuffer(CGraphicBase::DEFAULT_IB_FILL_RECT);

		STATEMANAGER.SetTexture(0, pTexture->GetSRV());
		STATEMANAGER.SetTexture(1, NULL);
		STATEMANAGER.SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
		STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 4, 0, 2);
	}

	STATEMANAGER.RestoreRenderState(RS11_ALPHATESTENABLE);
	STATEMANAGER.RestoreRenderState(RS11_DESTBLEND);
	STATEMANAGER.RestoreRenderState(RS11_SRCBLEND);
	STATEMANAGER.RestoreRenderState(RS11_ALPHABLENDENABLE);
	STATEMANAGER.RestoreRenderState(RS11_ZWRITEENABLE);
	STATEMANAGER.RestoreRenderState(RS11_ZENABLE);
	STATEMANAGER.RestoreRenderState(RS11_CULLMODE);
}

const CGraphicTexture & CGraphicImageInstance::GetTextureReference() const
{
	return m_roImage->GetTextureReference();
}

CGraphicTexture * CGraphicImageInstance::GetTexturePointer()
{
	CGraphicImage* pkImage = m_roImage.GetPointer();
	return pkImage ? pkImage->GetTexturePointer() : NULL;
}

CGraphicImage * CGraphicImageInstance::GetGraphicImagePointer()
{
	return m_roImage.GetPointer();
}

int CGraphicImageInstance::GetWidth()
{
	if (IsEmpty())
		return 0;
	
	return m_roImage->GetWidth();
}

int CGraphicImageInstance::GetHeight()
{
	if (IsEmpty())
		return 0;
	
	return m_roImage->GetHeight();
}

void CGraphicImageInstance::SetDiffuseColor(float fr, float fg, float fb, float fa)
{
	m_DiffuseColor.r = fr;
	m_DiffuseColor.g = fg;
	m_DiffuseColor.b = fb;
	m_DiffuseColor.a = fa;
}
void CGraphicImageInstance::SetPosition(float fx, float fy)
{
	m_v2Position.x = fx;
	m_v2Position.y = fy;
}

void CGraphicImageInstance::SetImagePointer(CGraphicImage * pImage)
{
	m_roImage.SetPointer(pImage);

	OnSetImagePointer();
}

void CGraphicImageInstance::ReloadImagePointer(CGraphicImage * pImage)
{
	if (m_roImage.IsNull())
	{
		SetImagePointer(pImage);
		return;
	}

	CGraphicImage * pkImage = m_roImage.GetPointer();

	if (pkImage)
		pkImage->Reload();
}

bool CGraphicImageInstance::IsEmpty() const
{
	if (!m_roImage.IsNull() && !m_roImage->IsEmpty())
		return false;

	return true;
}

bool CGraphicImageInstance::operator == (const CGraphicImageInstance & rhs) const
{
	return (m_roImage.GetPointer() == rhs.m_roImage.GetPointer());
}

DWORD CGraphicImageInstance::Type()
{
	static DWORD s_dwType = GetCRC32("CGraphicImageInstance", strlen("CGraphicImageInstance"));
	return (s_dwType);
}

BOOL CGraphicImageInstance::IsType(DWORD dwType)
{
	return OnIsType(dwType);
}

BOOL CGraphicImageInstance::OnIsType(DWORD dwType)
{
	if (CGraphicImageInstance::Type() == dwType)
		return TRUE;

	return FALSE;
}

void CGraphicImageInstance::OnSetImagePointer()
{
}

void CGraphicImageInstance::Initialize()
{
	m_DiffuseColor.r = m_DiffuseColor.g = m_DiffuseColor.b = m_DiffuseColor.a = 1.0f;
	m_v2Position.x = m_v2Position.y = 0.0f;
}

void CGraphicImageInstance::Destroy()
{
	m_roImage.SetPointer(NULL); // CRef 에서 레퍼런스 카운트가 떨어져야 함.
	Initialize();
}

CGraphicImageInstance::CGraphicImageInstance()
{
	Initialize();
}

CGraphicImageInstance::~CGraphicImageInstance()
{
	Destroy();
}
