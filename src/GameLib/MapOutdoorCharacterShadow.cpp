#include "StdAfx.h"
#include "EterLib/StateManager.h"
#include "EterLib/Camera.h"

#include "MapOutdoor.h"

static int recreate = false;

void CMapOutdoor::SetShadowTextureSize(WORD size)
{
	if (m_wShadowMapSize != size)
	{
		recreate = true;
		Tracenf("ShadowTextureSize changed %d -> %d", m_wShadowMapSize, size);
	}

	m_wShadowMapSize = size;
}

void CMapOutdoor::CreateCharacterShadowTexture()
{
	extern bool GRAPHICS_CAPS_CAN_NOT_DRAW_SHADOW;

	if (GRAPHICS_CAPS_CAN_NOT_DRAW_SHADOW)
		return;

	ReleaseCharacterShadowTexture();

	if (!ms_lpd3d11Device)
		return;

	if (IsLowTextureMemory())
		SetShadowTextureSize(128);

	m_ShadowMapViewport.TopLeftX = 1;
	m_ShadowMapViewport.TopLeftY = 1;
	m_ShadowMapViewport.Width = (float)(m_wShadowMapSize - 2);
	m_ShadowMapViewport.Height = (float)(m_wShadowMapSize - 2);
	m_ShadowMapViewport.MinDepth = 0.0f;
	m_ShadowMapViewport.MaxDepth = 1.0f;

	// Color render target (shadow map)
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = m_wShadowMapSize;
	td.Height = m_wShadowMapSize;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_B5G6R5_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&td, NULL, &m_lpCharacterShadowMapTex)))
	{
		TraceError("CMapOutdoor Unable to create Character Shadow texture\n");
		return;
	}

	if (FAILED(ms_lpd3d11Device->CreateRenderTargetView(m_lpCharacterShadowMapTex, NULL, &m_lpCharacterShadowMapRTV)))
	{
		TraceError("CMapOutdoor Unable to create Character Shadow RTV\n");
		return;
	}

	if (FAILED(ms_lpd3d11Device->CreateShaderResourceView(m_lpCharacterShadowMapTex, NULL, &m_lpCharacterShadowMapTexture)))
	{
		TraceError("CMapOutdoor Unable to create Character Shadow SRV\n");
		return;
	}

	// Depth stencil
	D3D11_TEXTURE2D_DESC dd = {};
	dd.Width = m_wShadowMapSize;
	dd.Height = m_wShadowMapSize;
	dd.MipLevels = 1;
	dd.ArraySize = 1;
	dd.Format = DXGI_FORMAT_D16_UNORM;
	dd.SampleDesc.Count = 1;
	dd.Usage = D3D11_USAGE_DEFAULT;
	dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&dd, NULL, &m_lpCharacterShadowMapDepthTex)))
	{
		TraceError("CMapOutdoor Unable to create Character Shadow depth texture\n");
		return;
	}

	if (FAILED(ms_lpd3d11Device->CreateDepthStencilView(m_lpCharacterShadowMapDepthTex, NULL, &m_lpCharacterShadowMapDSV)))
	{
		TraceError("CMapOutdoor Unable to create Character Shadow DSV\n");
		return;
	}
}

void CMapOutdoor::ReleaseCharacterShadowTexture()
{
	SAFE_RELEASE(m_lpCharacterShadowMapDSV);
	SAFE_RELEASE(m_lpCharacterShadowMapDepthTex);
	SAFE_RELEASE(m_lpCharacterShadowMapRTV);
	SAFE_RELEASE(m_lpCharacterShadowMapTexture);
	SAFE_RELEASE(m_lpCharacterShadowMapTex);
}

DWORD dwLightEnable = FALSE;

bool CMapOutdoor::BeginRenderCharacterShadowToTexture()
{
	D3DXMATRIX matLightView, matLightProj;

	CCamera* pCurrentCamera = CCameraManager::Instance().GetCurrentCamera();

	if (!pCurrentCamera)
		return false;

	if (!ms_lpd3d11Context)
		return false;

	if (recreate)
	{
		CreateCharacterShadowTexture();
		recreate = false;
	}

	D3DXVECTOR3 v3Target = pCurrentCamera->GetTarget();

	D3DXVECTOR3 v3Eye(v3Target.x - 1.732f * 1250.0f,
					  v3Target.y - 1250.0f,
					  v3Target.z + 2.0f * 1.732f * 1250.0f);

	const auto vv = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
	D3DXMatrixLookAtRH(&matLightView, &v3Eye, &v3Target, &vv);
	D3DXMatrixOrthoRH(&matLightProj, 2550.0f, 2550.0f, 1.0f, 15000.0f);

	STATEMANAGER.SaveTransform(View, &matLightView);
	STATEMANAGER.SaveTransform(Projection, &matLightProj);

	dwLightEnable = STATEMANAGER.GetRenderState(RS11_LIGHTING);
	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);

	STATEMANAGER.SaveRenderState(RS11_TEXTUREFACTOR, 0xFF808080);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP, TOP11_SELECTARG1);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TFACTOR);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP, TOP11_DISABLE);
	STATEMANAGER.SetTextureStageState(1, TSS11_COLOROP, TOP11_DISABLE);

	// Save current render targets and viewport
	ms_lpd3d11Context->OMGetRenderTargets(1, &m_lpBackupRTV, &m_lpBackupDSV);
	m_uBackupNumViewports = 1;
	ms_lpd3d11Context->RSGetViewports(&m_uBackupNumViewports, &m_BackupViewport);

	// Set shadow render target
	ms_lpd3d11Context->OMSetRenderTargets(1, &m_lpCharacterShadowMapRTV, m_lpCharacterShadowMapDSV);

	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	ms_lpd3d11Context->ClearRenderTargetView(m_lpCharacterShadowMapRTV, clearColor);
	ms_lpd3d11Context->ClearDepthStencilView(m_lpCharacterShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	ms_lpd3d11Context->RSSetViewports(1, &m_ShadowMapViewport);

	return true;
}

void CMapOutdoor::EndRenderCharacterShadowToTexture()
{
	if (!ms_lpd3d11Context)
		return;

	// Restore previous render target and viewport
	ms_lpd3d11Context->RSSetViewports(1, &m_BackupViewport);
	ms_lpd3d11Context->OMSetRenderTargets(1, &m_lpBackupRTV, m_lpBackupDSV);

	SAFE_RELEASE(m_lpBackupRTV);
	SAFE_RELEASE(m_lpBackupDSV);

	STATEMANAGER.RestoreTransform(View);
	STATEMANAGER.RestoreTransform(Projection);

	// Restore Device Context
	STATEMANAGER.SetRenderState(RS11_LIGHTING, dwLightEnable);
	STATEMANAGER.RestoreRenderState(RS11_TEXTUREFACTOR);
}
