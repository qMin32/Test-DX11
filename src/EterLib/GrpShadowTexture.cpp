#include "StdAfx.h"
#include "GrpShadowTexture.h"
#include "EterBase/Stl.h"
#include "StateManager.h"

void CGraphicShadowTexture::Destroy()
{
	CGraphicTexture::Destroy();

	safe_release(m_pShadowRTV);
	safe_release(m_pShadowTex);
	safe_release(m_pDepthDSV);
	safe_release(m_pDepthTex);

	Initialize();
}

bool CGraphicShadowTexture::Create(int width, int height)
{
	Destroy();

	if (!ms_lpd3d11Device)
		return false;

	m_width = width;
	m_height = height;

	// Color render target
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&td, NULL, &m_pShadowTex)))
		return false;

	if (FAILED(ms_lpd3d11Device->CreateRenderTargetView(m_pShadowTex, NULL, &m_pShadowRTV)))
		return false;

	if (FAILED(ms_lpd3d11Device->CreateShaderResourceView(m_pShadowTex, NULL, &m_lpSRV)))
		return false;

	// Depth stencil
	D3D11_TEXTURE2D_DESC dd = {};
	dd.Width = width;
	dd.Height = height;
	dd.MipLevels = 1;
	dd.ArraySize = 1;
	dd.Format = DXGI_FORMAT_D16_UNORM;
	dd.SampleDesc.Count = 1;
	dd.Usage = D3D11_USAGE_DEFAULT;
	dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&dd, NULL, &m_pDepthTex)))
		return false;

	if (FAILED(ms_lpd3d11Device->CreateDepthStencilView(m_pDepthTex, NULL, &m_pDepthDSV)))
		return false;

	m_bEmpty = false;
	return true;
}

void CGraphicShadowTexture::Set(int stage) const
{
	STATEMANAGER.SetTexture(stage, m_lpSRV);
}

const D3DXMATRIX& CGraphicShadowTexture::GetLightVPMatrixReference() const
{
	return m_d3dLightVPMatrix;
}

void CGraphicShadowTexture::Begin()
{
	if (!ms_lpd3d11Context)
		return;

	D3DXMatrixMultiply(&m_d3dLightVPMatrix, &ms_matView, &ms_matProj);

	// Save current render targets and viewport
	ms_lpd3d11Context->OMGetRenderTargets(1, &m_pOldRTV, &m_pOldDSV);
	m_uOldNumViewports = 1;
	ms_lpd3d11Context->RSGetViewports(&m_uOldNumViewports, &m_d3dOldViewport);

	// Set shadow render target
	ms_lpd3d11Context->OMSetRenderTargets(1, &m_pShadowRTV, m_pDepthDSV);

	D3D11_VIEWPORT vp = {};
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = (float)m_width;
	vp.Height = (float)m_height;
	ms_lpd3d11Context->RSSetViewports(1, &vp);

	// Clear
	float clearColor[4] = { 0, 0, 0, 0 };
	ms_lpd3d11Context->ClearRenderTargetView(m_pShadowRTV, clearColor);
	ms_lpd3d11Context->ClearDepthStencilView(m_pDepthDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Save state for shadow rendering
	STATEMANAGER.SaveRenderState(RS11_CULLMODE, D3D11_CULL_NONE);
	STATEMANAGER.SaveRenderState(RS11_ZFUNC, D3D11_COMPARISON_LESS_EQUAL);
	STATEMANAGER.SaveRenderState(RS11_ALPHABLENDENABLE, true);
	STATEMANAGER.SaveRenderState(RS11_ALPHATESTENABLE, true);
	STATEMANAGER.SaveRenderState(RS11_TEXTUREFACTOR, 0xbb000000);

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SaveTextureStageState(0, TSS11_COLORARG1, TA11_TFACTOR);
	STATEMANAGER.SaveTextureStageState(0, TSS11_COLORARG2, TA11_TEXTURE);
	STATEMANAGER.SaveTextureStageState(0, TSS11_COLOROP,   TOP11_MODULATE);
	STATEMANAGER.SaveTextureStageState(0, TSS11_ALPHAARG1, TA11_TFACTOR);
	STATEMANAGER.SaveTextureStageState(0, TSS11_ALPHAARG2, TA11_TEXTURE);
	STATEMANAGER.SaveTextureStageState(0, TSS11_ALPHAOP,   TOP11_MODULATE);

	STATEMANAGER.SaveSamplerState(0, SS11_MINFILTER, TF11_POINT);
	STATEMANAGER.SaveSamplerState(0, SS11_MAGFILTER, TF11_POINT);
	STATEMANAGER.SaveSamplerState(0, SS11_MIPFILTER, TF11_POINT);
	STATEMANAGER.SaveSamplerState(0, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_CLAMP);
	STATEMANAGER.SaveSamplerState(0, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_CLAMP);

	STATEMANAGER.SetTexture(1, NULL);
	STATEMANAGER.SaveTextureStageState(1, TSS11_COLORARG1, TA11_CURRENT);
	STATEMANAGER.SaveTextureStageState(1, TSS11_COLORARG2, TA11_TEXTURE);
	STATEMANAGER.SaveTextureStageState(1, TSS11_COLOROP,   TOP11_SELECTARG1);
	STATEMANAGER.SaveTextureStageState(1, TSS11_ALPHAARG1, TA11_CURRENT);
	STATEMANAGER.SaveTextureStageState(1, TSS11_ALPHAARG2, TA11_TEXTURE);
	STATEMANAGER.SaveTextureStageState(1, TSS11_ALPHAOP,   TOP11_SELECTARG1);

	STATEMANAGER.SaveSamplerState(1, SS11_MINFILTER, TF11_POINT);
	STATEMANAGER.SaveSamplerState(1, SS11_MAGFILTER, TF11_POINT);
	STATEMANAGER.SaveSamplerState(1, SS11_MIPFILTER, TF11_POINT);
	STATEMANAGER.SaveSamplerState(1, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_CLAMP);
	STATEMANAGER.SaveSamplerState(1, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_CLAMP);
}

void CGraphicShadowTexture::End()
{
	if (!ms_lpd3d11Context)
		return;

	// Restore previous render target and viewport
	ms_lpd3d11Context->OMSetRenderTargets(1, &m_pOldRTV, m_pOldDSV);
	ms_lpd3d11Context->RSSetViewports(1, &m_d3dOldViewport);

	safe_release(m_pOldRTV);
	safe_release(m_pOldDSV);

	STATEMANAGER.RestoreRenderState(RS11_CULLMODE);
	STATEMANAGER.RestoreRenderState(RS11_ZFUNC);
	STATEMANAGER.RestoreRenderState(RS11_ALPHABLENDENABLE);
	STATEMANAGER.RestoreRenderState(RS11_ALPHATESTENABLE);
	STATEMANAGER.RestoreRenderState(RS11_TEXTUREFACTOR);

	STATEMANAGER.RestoreTextureStageState(0, TSS11_COLORARG1);
	STATEMANAGER.RestoreTextureStageState(0, TSS11_COLORARG2);
	STATEMANAGER.RestoreTextureStageState(0, TSS11_COLOROP);
	STATEMANAGER.RestoreTextureStageState(0, TSS11_ALPHAARG1);
	STATEMANAGER.RestoreTextureStageState(0, TSS11_ALPHAARG2);
	STATEMANAGER.RestoreTextureStageState(0, TSS11_ALPHAOP);

	STATEMANAGER.RestoreSamplerState(0, SS11_MINFILTER);
	STATEMANAGER.RestoreSamplerState(0, SS11_MAGFILTER);
	STATEMANAGER.RestoreSamplerState(0, SS11_MIPFILTER);
	STATEMANAGER.RestoreSamplerState(0, SS11_ADDRESSU);
	STATEMANAGER.RestoreSamplerState(0, SS11_ADDRESSV);

	STATEMANAGER.RestoreTextureStageState(1, TSS11_COLORARG1);
	STATEMANAGER.RestoreTextureStageState(1, TSS11_COLORARG2);
	STATEMANAGER.RestoreTextureStageState(1, TSS11_COLOROP);
	STATEMANAGER.RestoreTextureStageState(1, TSS11_ALPHAARG1);
	STATEMANAGER.RestoreTextureStageState(1, TSS11_ALPHAARG2);
	STATEMANAGER.RestoreTextureStageState(1, TSS11_ALPHAOP);

	STATEMANAGER.RestoreSamplerState(1, SS11_MINFILTER);
	STATEMANAGER.RestoreSamplerState(1, SS11_MAGFILTER);
	STATEMANAGER.RestoreSamplerState(1, SS11_MIPFILTER);
	STATEMANAGER.RestoreSamplerState(1, SS11_ADDRESSU);
	STATEMANAGER.RestoreSamplerState(1, SS11_ADDRESSV);
}

void CGraphicShadowTexture::Initialize()
{
	CGraphicTexture::Initialize();

	m_pShadowTex = NULL;
	m_pShadowRTV = NULL;
	m_pDepthTex = NULL;
	m_pDepthDSV = NULL;
	m_pOldRTV = NULL;
	m_pOldDSV = NULL;
	m_uOldNumViewports = 0;
	memset(&m_d3dOldViewport, 0, sizeof(m_d3dOldViewport));
}

CGraphicShadowTexture::CGraphicShadowTexture()
{
	Initialize();
}

CGraphicShadowTexture::~CGraphicShadowTexture()
{
	Destroy();
}
