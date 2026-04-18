#include "StdAfx.h"
#include "SnowEnvironment.h"

#include "EterLib/StateManager.h"
#include "EterLib/Camera.h"
#include "EterLib/ResourceManager.h"
#include "SnowParticle.h"
#include "qMin32Lib/All.h"

void CSnowEnvironment::Enable()
{
	if (!m_bSnowEnable)
	{
		Create();
	}

	m_bSnowEnable = TRUE;
}

void CSnowEnvironment::Disable()
{
	m_bSnowEnable = FALSE;
}

void CSnowEnvironment::Update(const D3DXVECTOR3 & c_rv3Pos)
{
	if (!m_bSnowEnable)
	{
		if (m_kVct_pkParticleSnow.empty())
			return;
	}

	m_v3Center=c_rv3Pos;
}

void CSnowEnvironment::Deform()
{
	if (!m_bSnowEnable)
	{
		if (m_kVct_pkParticleSnow.empty())
			return;
	}

	const D3DXVECTOR3 & c_rv3Pos=m_v3Center;
	
	static long s_lLastTime = CTimer::Instance().GetCurrentMillisecond();
	long lcurTime = CTimer::Instance().GetCurrentMillisecond();
	float fElapsedTime = float(lcurTime - s_lLastTime) / 1000.0f;
	s_lLastTime = lcurTime;

	CCamera * pCamera = CCameraManager::Instance().GetCurrentCamera();
	if (!pCamera)
		return;

	const D3DXVECTOR3 & c_rv3View = pCamera->GetView();

	D3DXVECTOR3 v3ChangedPos = c_rv3View * 3500.0f + c_rv3Pos;
	v3ChangedPos.z = c_rv3Pos.z;

	std::vector<CSnowParticle*>::iterator itor = m_kVct_pkParticleSnow.begin();
	for (; itor != m_kVct_pkParticleSnow.end();)
	{
		CSnowParticle * pSnow = *itor;
		pSnow->Update(fElapsedTime, v3ChangedPos);

		if (!pSnow->IsActivate())
		{
			CSnowParticle::Delete(pSnow);

			itor = m_kVct_pkParticleSnow.erase(itor);
		}
		else
		{
			++itor;
		}
	}

	if (m_bSnowEnable)
	{
		for (int p = 0; p < std::min(10ull, m_dwParticleMaxNum - m_kVct_pkParticleSnow.size()); ++p)
		{
			CSnowParticle * pSnowParticle = CSnowParticle::New();
			pSnowParticle->Init(v3ChangedPos);
			m_kVct_pkParticleSnow.push_back(pSnowParticle);
		}
	}
}

void CSnowEnvironment::__BeginBlur()
{
	if (!m_bBlurEnable)
		return;

	if (!ms_lpd3d11Context)
		return;

	ms_lpd3d11Context->OMGetRenderTargets(1, &m_lpOldRTV, &m_lpOldDSV);
	ms_lpd3d11Context->OMSetRenderTargets(1, &m_lpSnowRTV, m_lpSnowDSV);

	float clearColor[4] = { 0, 0, 0, 0 };
	ms_lpd3d11Context->ClearRenderTargetView(m_lpSnowRTV, clearColor);
	ms_lpd3d11Context->ClearDepthStencilView(m_lpSnowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	STATEMANAGER.SetRenderState(RS11_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SetRenderState(RS11_SRCBLEND, D3D11_BLEND_SRC_ALPHA);
	STATEMANAGER.SetRenderState(RS11_DESTBLEND, D3D11_BLEND_DEST_ALPHA);
}

void CSnowEnvironment::__ApplyBlur()
{
	if (!m_bBlurEnable)
		return;

	{
		// Restore previous render targets
		ms_lpd3d11Context->OMSetRenderTargets(1, &m_lpOldRTV, m_lpOldDSV);

		STATEMANAGER.SetTexture(0, m_lpSnowTexture);
		STATEMANAGER.SetRenderState(RS11_ALPHABLENDENABLE, TRUE);

		// Get backbuffer size from current viewport
		UINT numVP = 1;
		D3D11_VIEWPORT vp;
		ms_lpd3d11Context->RSGetViewports(&numVP, &vp);
		float sx = vp.Width;
		float sy = vp.Height;

		SAFE_RELEASE(m_lpOldRTV);
		SAFE_RELEASE(m_lpOldDSV);

		BlurVertex V[4] = {	BlurVertex(D3DXVECTOR3(0.0f,0.0f,0.0f),1.0f	,0xFFFFFF, 0,0) ,
							BlurVertex(D3DXVECTOR3(sx,0.0f,0.0f),1.0f	,0xFFFFFF, 1,0) ,
							BlurVertex(D3DXVECTOR3(0.0f,sy,0.0f),1.0f	,0xFFFFFF, 0,1) ,
							BlurVertex(D3DXVECTOR3(sx,sy,0.0f),1.0f		,0xFFFFFF, 1,1) };

		STATEMANAGER.SetFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE|D3DFVF_TEX1 );
		STATEMANAGER.DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,V,sizeof(BlurVertex));
	}
}

void CSnowEnvironment::Render()
{
	if (!m_bSnowEnable)
	{
		if (m_kVct_pkParticleSnow.empty())
			return;
	}

	__BeginBlur();

	DWORD dwParticleCount = std::min((size_t)m_dwParticleMaxNum, m_kVct_pkParticleSnow.size());

	CCamera * pCamera = CCameraManager::Instance().GetCurrentCamera();
	if (!pCamera)
		return;

	const D3DXVECTOR3 & c_rv3Up = pCamera->GetUp();
	const D3DXVECTOR3 & c_rv3Cross = pCamera->GetCross();

	D3D11_MAPPED_SUBRESOURCE mapped;
	if (ms_lpd3d11Context && SUCCEEDED(ms_lpd3d11Context->Map(m_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		SParticleVertex * pv3Verticies = (SParticleVertex *)mapped.pData;
		int i = 0;
		std::vector<CSnowParticle*>::iterator itor = m_kVct_pkParticleSnow.begin();
		for (; i < dwParticleCount && itor != m_kVct_pkParticleSnow.end(); ++i, ++itor)
		{
			CSnowParticle * pSnow = *itor;
			pSnow->SetCameraVertex(c_rv3Up, c_rv3Cross);
			pSnow->GetVerticies(pv3Verticies[i*4+0],
								pv3Verticies[i*4+1],
								pv3Verticies[i*4+2],
								pv3Verticies[i*4+3]);
		}
		ms_lpd3d11Context->Unmap(m_pVB, 0);
	}

	STATEMANAGER.SaveRenderState(RS11_ZWRITEENABLE, FALSE);
	STATEMANAGER.SaveRenderState(RS11_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_CULLMODE, D3D11_CULL_NONE);
	STATEMANAGER.SetRenderState(RS11_SRCBLEND,  D3D11_BLEND_SRC_ALPHA);
	STATEMANAGER.SetRenderState(RS11_DESTBLEND, D3D11_BLEND_INV_SRC_ALPHA);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP, TOP11_SELECTARG1);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP, TOP11_SELECTARG1);
	STATEMANAGER.SetTexture(1, NULL);
	STATEMANAGER.SetTextureStageState(1, TSS11_COLOROP, TOP11_DISABLE);
	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP, TOP11_DISABLE);

	m_pImageInstance->GetGraphicImagePointer()->GetTextureReference().SetTextureStage(0);
	_mgr->SetIndexBuffer(m_pIB);
	STATEMANAGER.SetStreamSource(0, m_pVB, sizeof(SParticleVertex));
	STATEMANAGER.SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
	STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, dwParticleCount*4, 0, dwParticleCount*2);
	STATEMANAGER.RestoreRenderState(RS11_ALPHABLENDENABLE);
	STATEMANAGER.RestoreRenderState(RS11_ZWRITEENABLE);
	STATEMANAGER.RestoreRenderState(RS11_CULLMODE);

	__ApplyBlur();
}

bool CSnowEnvironment::__CreateBlurTexture()
{
	if (!m_bBlurEnable)
		return true;

	if (!ms_lpd3d11Device)
		return false;

	// Snow color render target
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = m_wBlurTextureSize;
	td.Height = m_wBlurTextureSize;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&td, NULL, &m_lpSnowTex)))
		return false;
	if (FAILED(ms_lpd3d11Device->CreateRenderTargetView(m_lpSnowTex, NULL, &m_lpSnowRTV)))
		return false;
	if (FAILED(ms_lpd3d11Device->CreateShaderResourceView(m_lpSnowTex, NULL, &m_lpSnowTexture)))
		return false;

	// Snow depth stencil
	D3D11_TEXTURE2D_DESC dd = {};
	dd.Width = m_wBlurTextureSize;
	dd.Height = m_wBlurTextureSize;
	dd.MipLevels = 1;
	dd.ArraySize = 1;
	dd.Format = DXGI_FORMAT_D16_UNORM;
	dd.SampleDesc.Count = 1;
	dd.Usage = D3D11_USAGE_DEFAULT;
	dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&dd, NULL, &m_lpSnowDepthTex)))
		return false;
	if (FAILED(ms_lpd3d11Device->CreateDepthStencilView(m_lpSnowDepthTex, NULL, &m_lpSnowDSV)))
		return false;

	// Accumulation color render target
	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&td, NULL, &m_lpAccumTex)))
		return false;
	if (FAILED(ms_lpd3d11Device->CreateRenderTargetView(m_lpAccumTex, NULL, &m_lpAccumRTV)))
		return false;
	if (FAILED(ms_lpd3d11Device->CreateShaderResourceView(m_lpAccumTex, NULL, &m_lpAccumTexture)))
		return false;

	// Accumulation depth stencil
	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&dd, NULL, &m_lpAccumDepthTex)))
		return false;
	if (FAILED(ms_lpd3d11Device->CreateDepthStencilView(m_lpAccumDepthTex, NULL, &m_lpAccumDSV)))
		return false;

	return true;
}

bool CSnowEnvironment::__CreateGeometry()
{
	if (!ms_lpd3d11Device)
		return false;

	// Dynamic vertex buffer (CPU writes each frame via Map/DISCARD)
	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.ByteWidth = sizeof(SParticleVertex) * m_dwParticleMaxNum * 4;
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(ms_lpd3d11Device->CreateBuffer(&vbDesc, nullptr, &m_pVB)))
		return false;

	// Build static index data
	std::vector<WORD> indices(m_dwParticleMaxNum * 6);
	const WORD c_awFillRectIndices[6] = { 0, 2, 1, 2, 3, 1, };
	for (DWORD i = 0; i < m_dwParticleMaxNum; ++i)
	{
		for (int j = 0; j < 6; ++j)
		{
			indices[i*6 + j] = static_cast<WORD>(i*4 + c_awFillRectIndices[j]);
		}
	}
	_mgr->CreateIndexBuffer(m_pIB, indices.data(), (UINT)indices.size(), true);
	return true;
}

bool CSnowEnvironment::Create()
{
	Destroy();

	if (!__CreateBlurTexture())
		return false;

	if (!__CreateGeometry())
		return false;

	CGraphicImage * pImage = (CGraphicImage *)CResourceManager::Instance().GetResourcePointer("d:/ymir work/special/snow.dds");
	m_pImageInstance = CGraphicImageInstance::New();
	m_pImageInstance->SetImagePointer(pImage);

	return true;
}

void CSnowEnvironment::Destroy()
{
	SAFE_RELEASE(m_lpSnowDSV);
	SAFE_RELEASE(m_lpSnowDepthTex);
	SAFE_RELEASE(m_lpSnowRTV);
	SAFE_RELEASE(m_lpSnowTexture);
	SAFE_RELEASE(m_lpSnowTex);
	SAFE_RELEASE(m_lpAccumDSV);
	SAFE_RELEASE(m_lpAccumDepthTex);
	SAFE_RELEASE(m_lpAccumRTV);
	SAFE_RELEASE(m_lpAccumTexture);
	SAFE_RELEASE(m_lpAccumTex);
	SAFE_RELEASE(m_pVB);

	stl_wipe(m_kVct_pkParticleSnow);
	CSnowParticle::DestroyPool();

	if (m_pImageInstance)
	{
		CGraphicImageInstance::Delete(m_pImageInstance);
		m_pImageInstance = NULL;
	}

	__Initialize();
}

void CSnowEnvironment::__Initialize()
{
	m_bSnowEnable = FALSE;
	m_lpSnowTex = NULL;
	m_lpSnowTexture = NULL;
	m_lpSnowRTV = NULL;
	m_lpSnowDepthTex = NULL;
	m_lpSnowDSV = NULL;
	m_lpAccumTex = NULL;
	m_lpAccumTexture = NULL;
	m_lpAccumRTV = NULL;
	m_lpAccumDepthTex = NULL;
	m_lpAccumDSV = NULL;
	m_lpOldRTV = NULL;
	m_lpOldDSV = NULL;
	m_pVB = NULL;
	m_pIB = NULL;
	m_pImageInstance = NULL;

	m_kVct_pkParticleSnow.reserve(m_dwParticleMaxNum);
}

CSnowEnvironment::CSnowEnvironment()
{
	m_bBlurEnable = FALSE;
	m_dwParticleMaxNum = 3000;
	m_wBlurTextureSize = 512;

	__Initialize();
}
CSnowEnvironment::~CSnowEnvironment()
{
	Destroy();
}
