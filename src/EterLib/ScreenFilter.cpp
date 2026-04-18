#include "StdAfx.h"
#include "ScreenFilter.h"
#include "StateManager.h"

void CScreenFilter::Render()
{
	if (!m_bEnable)
		return;

	STATEMANAGER.SaveTransform(Projection, &ms_matIdentity);
 	STATEMANAGER.SaveTransform(View, &ms_matIdentity);
 	STATEMANAGER.SetTransform(World, &ms_matIdentity);
	STATEMANAGER.SaveRenderState(RS11_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_SRCBLEND, m_bySrcType);
	STATEMANAGER.SaveRenderState(RS11_DESTBLEND, m_byDestType);

	SetOrtho2D(CScreen::ms_iWidth, CScreen::ms_iHeight, 400.0f);
	SetDiffuseColor(m_Color.r, m_Color.g, m_Color.b, m_Color.a);
	RenderBar2d(0, 0, CScreen::ms_iWidth, CScreen::ms_iHeight);

	STATEMANAGER.RestoreRenderState(RS11_ALPHABLENDENABLE);
	STATEMANAGER.RestoreRenderState(RS11_SRCBLEND);
	STATEMANAGER.RestoreRenderState(RS11_DESTBLEND);
 	STATEMANAGER.RestoreTransform(View);
	STATEMANAGER.RestoreTransform(Projection);
}

void CScreenFilter::SetEnable(BOOL /*bFlag*/)
{
	m_bEnable = FALSE;
}

void CScreenFilter::SetBlendType(BYTE bySrcType, BYTE byDestType)
{
	m_bySrcType = bySrcType;
	m_byDestType = byDestType;
}
void CScreenFilter::SetColor(const D3DXCOLOR & c_rColor)
{
	m_Color = c_rColor;
}

CScreenFilter::CScreenFilter()
{
	m_bEnable = FALSE;
	m_bySrcType = D3D11_BLEND_SRC_ALPHA;
	m_byDestType = D3D11_BLEND_INV_SRC_ALPHA;
	m_Color = D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f);
}
CScreenFilter::~CScreenFilter()
{
}
