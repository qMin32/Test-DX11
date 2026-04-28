#include "StdAfx.h"
#include "StateManager.h"
#include "GrpLightManager.h"
#include "GrpD3D11Renderer.h"
#include "qMin32Lib/ConstantBufferManager.h"

// Global per-frame draw call counter (always-on, not just debug)
static int g_frameDrawCount = 0;
static int g_frameNumber = 0;
static int g_drawByStride[64] = {};	// indexed by stride/4

//#define StateManager_Assert(a) if (!(a)) puts("assert"#a)
#define StateManager_Assert(a) assert(a)

struct SLightData
{
	D3DLIGHT9 m_akD3DLight[8];
} m_kLightData;



void CStateManager::SetLight(DWORD index, CONST D3DLIGHT9* pLight)
{
	assert(index < 8);
	m_kLightData.m_akD3DLight[index] = *pLight;

	if (m_pD3D11Renderer)
		m_pD3D11Renderer->GetCbMgr()->SetLight(index, pLight);
}

void CStateManager::GetLight(DWORD index, D3DLIGHT9* pLight)
{
	assert(index < 8);
	*pLight = m_kLightData.m_akD3DLight[index];
}

void CStateManager::SetScissorRect(const RECT& c_rRect)
{
	if (CGraphicBase::ms_lpd3d11Context)
	{
		D3D11_RECT r = { c_rRect.left, c_rRect.top, c_rRect.right, c_rRect.bottom };
		CGraphicBase::ms_lpd3d11Context->RSSetScissorRects(1, &r);
	}
}

void CStateManager::GetScissorRect(RECT* pRect)
{
	if (CGraphicBase::ms_lpd3d11Context)
	{
		UINT n = 1;
		D3D11_RECT r = {};
		CGraphicBase::ms_lpd3d11Context->RSGetScissorRects(&n, &r);
		pRect->left = r.left; pRect->top = r.top; pRect->right = r.right; pRect->bottom = r.bottom;
	}
}

bool CStateManager::BeginScene()
{
	m_bScene = true;

	D3DXMATRIX m4Proj;
	D3DXMATRIX m4View;
	D3DXMATRIX m4World;
	GetTransform(World, &m4World);
	GetTransform(Projection, &m4Proj);
	GetTransform(View, &m4View);
	SetTransform(World, &m4World);
	SetTransform(Projection, &m4Proj);
	SetTransform(View, &m4View);

	static int s_beginCount = 0;
	if ((s_beginCount++ % 60) == 0)
	{
		UINT n = 1;
		D3D11_VIEWPORT vp = {};
		if (CGraphicBase::ms_lpd3d11Context)
			CGraphicBase::ms_lpd3d11Context->RSGetViewports(&n, &vp);
		Tracenf("BeginScene: viewport=(%g,%g %gx%g)", vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height);
	}

	if (CGraphicBase::ms_lpd3d11Context && CGraphicBase::ms_lpd3d11RTV)
	{
		CGraphicBase::ms_lpd3d11Context->OMSetRenderTargets(1, &CGraphicBase::ms_lpd3d11RTV, CGraphicBase::ms_lpd3d11DSV);

		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = (float)CGraphicBase::ms_iWidth;
		vp.Height = (float)CGraphicBase::ms_iHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		CGraphicBase::ms_lpd3d11Context->RSSetViewports(1, &vp);

		float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		CGraphicBase::ms_lpd3d11Context->ClearRenderTargetView(CGraphicBase::ms_lpd3d11RTV, clearColor);
		if (CGraphicBase::ms_lpd3d11DSV)
			CGraphicBase::ms_lpd3d11Context->ClearDepthStencilView(CGraphicBase::ms_lpd3d11DSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
	return true;
}

void CStateManager::EndScene()
{
	m_bScene = false;
	++g_frameNumber;
	if ((g_frameNumber % 120) == 0)
	{
		Tracenf("D3D11: Frame %d - draw calls = %d", g_frameNumber, g_frameDrawCount);
		Tracenf("  draws by stride: 16=%d 20=%d 24=%d 28=%d 32=%d 36=%d 40=%d",
			g_drawByStride[4], g_drawByStride[5], g_drawByStride[6], g_drawByStride[7],
			g_drawByStride[8], g_drawByStride[9], g_drawByStride[10]);
	}
	g_frameDrawCount = 0;
	for (int i = 0; i < 64; ++i) g_drawByStride[i] = 0;
}

CStateManager::CStateManager() : m_pD3D11Renderer(NULL)
{
	m_bScene = false;
	m_dwBestMinFilter = TF11_ANISOTROPIC;
	m_dwBestMagFilter = TF11_ANISOTROPIC;

	memset(gs_DefaultRenderStates, 0, sizeof(gs_DefaultRenderStates));

	SetDefaultState();

#ifdef _DEBUG
	m_iDrawCallCount = 0;
	m_iLastDrawCallCount = 0;
#endif
}

CStateManager::~CStateManager()
{
}

void CStateManager::SetBestFiltering(DWORD dwStage)
{
	SetSamplerState(dwStage, SS11_MINFILTER, m_dwBestMinFilter);
	SetSamplerState(dwStage, SS11_MAGFILTER, m_dwBestMagFilter);
	SetSamplerState(dwStage, SS11_MIPFILTER, TF11_LINEAR);
}

void CStateManager::Restore()
{
	int i, j;

	m_bForce = true;

	for (i = 0; i < RS11_MAX; ++i)
	{
		SetRenderState((ERenderState11)i, m_CurrentState.m_RenderStates[i]);
	}

	for (i = 0; i < STATEMANAGER_MAX_STAGES; ++i)
	{
		for (j = 0; j < STATEMANAGER_MAX_TEXTURESTATES; ++j)
		{
			SetTextureStageState(i, ETextureStageState11(j), m_CurrentState.m_TextureStates[i][j]);
		}

		for (j = 0; j < SS11_MAX; ++j)
		{
			SetSamplerState(i, (ESamplerStateType11)j, m_CurrentState.m_SamplerStates[i][j]);
		}
	}

	for (i = 0; i < STATEMANAGER_MAX_STAGES; ++i)
	{
		SetTexture(i, m_CurrentState.m_Textures[i]);
	}

	m_bForce = false;
}

void CStateManager::SetDefaultState()
{
	m_CurrentState.ResetState();
	m_CurrentState_Copy.ResetState();

	for (auto& stack : m_RenderStateStack)
		stack.clear();

	for (auto& stageStacks : m_SamplerStateStack)
		for (auto& stack : stageStacks)
			stack.clear();

	for (auto& stageStacks : m_TextureStageStateStack)
		for (auto& stack : stageStacks)
			stack.clear();

	for (auto& stack : m_TransformStack)
		stack.clear();

	for (auto& stack : m_TextureStack)
		stack.clear();

	for (auto& stack : m_StreamStack)
		stack.clear();

	m_MaterialStack.clear();

	m_VertexProcessingStack.clear();
	m_IndexStack.clear();

	m_bScene = false;
	m_bForce = true;

	D3DXMATRIX matIdentity;
	D3DXMatrixIdentity(&matIdentity);

	SetTransform(World, &matIdentity);
	SetTransform(View, &matIdentity);
	SetTransform(Projection, &matIdentity);

	D3DMATERIAL9 DefaultMat;
	ZeroMemory(&DefaultMat, sizeof(D3DMATERIAL9));

	DefaultMat.Diffuse.r = 1.0f;
	DefaultMat.Diffuse.g = 1.0f;
	DefaultMat.Diffuse.b = 1.0f;
	DefaultMat.Diffuse.a = 1.0f;
	DefaultMat.Ambient.r = 1.0f;
	DefaultMat.Ambient.g = 1.0f;
	DefaultMat.Ambient.b = 1.0f;
	DefaultMat.Ambient.a = 1.0f;
	DefaultMat.Emissive.r = 0.0f;
	DefaultMat.Emissive.g = 0.0f;
	DefaultMat.Emissive.b = 0.0f;
	DefaultMat.Emissive.a = 0.0f;
	DefaultMat.Specular.r = 0.0f;
	DefaultMat.Specular.g = 0.0f;
	DefaultMat.Specular.b = 0.0f;
	DefaultMat.Specular.a = 0.0f;
	DefaultMat.Power = 0.0f;

	SetMaterial(&DefaultMat);

	SetRenderState(RS11_ALPHAREF, 1);

	SetRenderState(RS11_FOGSTART, 0);
	SetRenderState(RS11_FOGEND, 0);

	SetRenderState(RS11_AMBIENT, 0x00000000);

	SetRenderState(RS11_SCISSORTESTENABLE, FALSE);

	SetRenderState(RS11_COLORWRITEENABLE, 0xFFFFFFFF);
	SetRenderState(RS11_FILLMODE, D3D11_FILL_SOLID);
	SetRenderState(RS11_CULLMODE, D3D11_CULL_FRONT);

	SetRenderState(RS11_ALPHABLENDENABLE, FALSE);
	SetRenderState(RS11_BLENDOP, D3D11_BLEND_OP_ADD);
	SetRenderState(RS11_SRCBLEND, D3D11_BLEND_SRC_ALPHA);
	SetRenderState(RS11_DESTBLEND, D3D11_BLEND_INV_SRC_ALPHA);

	SetRenderState(RS11_FOGENABLE, FALSE);
	SetRenderState(RS11_FOGCOLOR, 0xFF000000);

	SetRenderState(RS11_ZENABLE, TRUE);
	SetRenderState(RS11_ZFUNC, D3D11_COMPARISON_LESS_EQUAL);
	SetRenderState(RS11_ZWRITEENABLE, TRUE);

	SetRenderState(RS11_STENCILENABLE, FALSE);
	SetRenderState(RS11_ALPHATESTENABLE, FALSE);
	SetRenderState(RS11_LIGHTING, FALSE);

	SetTextureStageState(0, TSS11_COLOROP, TOP11_MODULATE);
	SetTextureStageState(0, TSS11_COLORARG1, TA11_TEXTURE);
	SetTextureStageState(0, TSS11_COLORARG2, TA11_CURRENT);
	SetTextureStageState(0, TSS11_ALPHAARG1, TA11_TEXTURE);
	SetTextureStageState(0, TSS11_ALPHAARG2, TA11_CURRENT);
	SetTextureStageState(0, TSS11_ALPHAOP, TOP11_SELECTARG1);

	for (DWORD i = 1; i < 8; ++i)
	{
		SetTextureStageState(i, TSS11_COLOROP, TOP11_DISABLE);
		SetTextureStageState(i, TSS11_COLORARG1, TA11_TEXTURE);
		SetTextureStageState(i, TSS11_COLORARG2, TA11_DIFFUSE);
		SetTextureStageState(i, TSS11_ALPHAOP, TOP11_DISABLE);
		SetTextureStageState(i, TSS11_ALPHAARG1, TA11_TEXTURE);
		SetTextureStageState(i, TSS11_ALPHAARG2, TA11_DIFFUSE);
	}

	for (DWORD i = 0; i < 8; ++i)
	{
		SetTextureStageState(i, TSS11_TEXCOORDINDEX, i);

		SetSamplerState(i, SS11_MINFILTER, TF11_LINEAR);
		SetSamplerState(i, SS11_MAGFILTER, TF11_LINEAR);
		SetSamplerState(i, SS11_MIPFILTER, TF11_LINEAR);

		SetSamplerState(i, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_WRAP);
		SetSamplerState(i, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_WRAP);
		SetTextureStageState(i, TSS11_TEXTURETRANSFORMFLAGS, 0);
		SetTexture(i, NULL);


	}

	D3DXVECTOR4 av4Null[STATEMANAGER_MAX_VCONSTANTS];
	memset(av4Null, 0, sizeof(av4Null));

	m_bForce = false;
}

// Material
void CStateManager::SaveMaterial()
{
	m_MaterialStack.push_back(m_CurrentState.m_D3DMaterial);
}

void CStateManager::SaveMaterial(const D3DMATERIAL9* pMaterial)
{
	m_MaterialStack.push_back(m_CurrentState.m_D3DMaterial);
	SetMaterial(pMaterial);
}

void CStateManager::RestoreMaterial()
{
	SetMaterial(&m_MaterialStack.back());
	m_MaterialStack.pop_back();
}

void CStateManager::SetMaterial(const D3DMATERIAL9* pMaterial)
{
	m_CurrentState.m_D3DMaterial = *pMaterial;

	if (m_pD3D11Renderer)
		m_pD3D11Renderer->GetCbMgr()->SetMaterial(pMaterial);
}

void CStateManager::GetMaterial(D3DMATERIAL9* pMaterial)
{
	// Set the renderstate and remember it.
	*pMaterial = m_CurrentState.m_D3DMaterial;
}

// Renderstates
DWORD CStateManager::GetRenderState(ERenderState11 Type)
{
	return m_CurrentState.m_RenderStates[Type];
}

void CStateManager::StateManager_Capture()
{
	m_CurrentState_Copy = m_CurrentState;
}

void CStateManager::StateManager_Apply()
{
	m_CurrentState = m_CurrentState_Copy;
}

void CStateManager::SetD3D11Renderer(CD3D11Renderer* pRenderer)
{
	m_pD3D11Renderer = pRenderer;
}

void CStateManager::SaveRenderState(ERenderState11 Type, DWORD dwValue)
{
	m_RenderStateStack[Type].push_back(m_CurrentState.m_RenderStates[Type]);
	SetRenderState(Type, dwValue);
}

void CStateManager::RestoreRenderState(ERenderState11 Type)
{
#ifdef _DEBUG
	if (m_RenderStateStack[Type].empty())
	{
		Tracef(" CStateManager::SaveRenderState - This render state was not saved [%d, %d]\n", Type);
		StateManager_Assert(!" This render state was not saved!");
	}
#endif _DEBUG

	SetRenderState(Type, m_RenderStateStack[Type].back());
	m_RenderStateStack[Type].pop_back();
}


void CStateManager::SetRenderState(ERenderState11 Type, DWORD Value)
{
	if (!m_bForce && m_CurrentState.m_RenderStates[Type] == Value)
		return;

	m_CurrentState.m_RenderStates[Type] = Value;

	if (m_pD3D11Renderer)
	{
		switch (Type)
		{
		case RS11_ALPHABLENDENABLE: m_pD3D11Renderer->SetAlphaBlendEnable(Value); break;
		case RS11_SRCBLEND:         m_pD3D11Renderer->SetSrcBlend((D3D11_BLEND)Value); break;
		case RS11_DESTBLEND:        m_pD3D11Renderer->SetDestBlend((D3D11_BLEND)Value); break;
		case RS11_BLENDOP:          m_pD3D11Renderer->SetBlendOp((D3D11_BLEND_OP)Value); break;

		case RS11_ALPHATESTENABLE:  m_pD3D11Renderer->SetAlphaTestEnable(Value); break;
		case RS11_ALPHAREF:         m_pD3D11Renderer->SetAlphaRef(Value); break;

		case RS11_ZENABLE:          m_pD3D11Renderer->SetZEnable(Value); break;
		case RS11_ZWRITEENABLE:     m_pD3D11Renderer->SetZWriteEnable(Value); break;
		case RS11_ZFUNC:            m_pD3D11Renderer->SetZFunc((D3D11_COMPARISON_FUNC)Value); break;

		case RS11_STENCILENABLE:    m_pD3D11Renderer->SetStencilEnable(Value); break;

		case RS11_CULLMODE:         m_pD3D11Renderer->SetCullMode((D3D11_CULL_MODE)Value); break;
		case RS11_FILLMODE:         m_pD3D11Renderer->SetFillMode((D3D11_FILL_MODE)Value); break;
		case RS11_SCISSORTESTENABLE:m_pD3D11Renderer->SetScissorEnable(Value); break;
		case RS11_COLORWRITEENABLE: m_pD3D11Renderer->SetColorWriteEnable((BYTE)Value); break;

		case RS11_LIGHTING:         m_pD3D11Renderer->GetCbMgr()->SetLightingEnable(Value); break;
		case RS11_AMBIENT:          m_pD3D11Renderer->GetCbMgr()->SetAmbient(Value); break;

		case RS11_FOGENABLE:        m_pD3D11Renderer->GetCbMgr()->SetFogEnable(Value); break;
		case RS11_FOGCOLOR:         m_pD3D11Renderer->GetCbMgr()->SetFogColor(Value); break;

		case RS11_FOGSTART:
		{
			float f; memcpy(&f, &Value, sizeof(float));
			m_pD3D11Renderer->GetCbMgr()->SetFogStart(f);
			break;
		}

		case RS11_FOGEND:
		{
			float f; memcpy(&f, &Value, sizeof(float));
			m_pD3D11Renderer->GetCbMgr()->SetFogEnd(f);
			break;
		}

		case RS11_TEXTUREFACTOR:    m_pD3D11Renderer->GetCbMgr()->SetTextureFactor(Value); break;
		}
	}
}

void CStateManager::GetRenderState(ERenderState11 Type, DWORD* pdwValue)
{
	*pdwValue = m_CurrentState.m_RenderStates[Type];
}

// Textures (D3D11 SRV)
void CStateManager::SaveTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV)
{
	m_TextureStack[dwStage].push_back(m_CurrentState.m_Textures[dwStage]);
	SetTexture(dwStage, pSRV);
}

void CStateManager::RestoreTexture(DWORD dwStage)
{
	SetTexture(dwStage, m_TextureStack[dwStage].back());
	m_TextureStack[dwStage].pop_back();
}

void CStateManager::SetTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV)
{
	// No cache check — D3D11 SRV pointers may be reused after destroy/recreate
	// causing stale bindings to silently fail
	m_CurrentState.m_Textures[dwStage] = pSRV;

	if (m_pD3D11Renderer)
		m_pD3D11Renderer->SetTexture(dwStage, pSRV);
}

void CStateManager::GetTexture(DWORD dwStage, ID3D11ShaderResourceView** ppSRV)
{
	*ppSRV = m_CurrentState.m_Textures[dwStage];
}

void CStateManager::SaveTextureStageState(DWORD dwStage, ETextureStageState11 Type, DWORD dwValue)
{
	m_TextureStageStateStack[dwStage][Type].push_back(m_CurrentState.m_TextureStates[dwStage][Type]);
	SetTextureStageState(dwStage, Type, dwValue);
}

void CStateManager::RestoreTextureStageState(DWORD dwStage, ETextureStageState11 Type)
{
#ifdef _DEBUG
	if (m_TextureStageStateStack[dwStage][Type].empty())
	{
		Tracef(" CStateManager::RestoreTextureStageState - This texture stage state was not saved [%d, %d]\n", dwStage, Type);
		StateManager_Assert(!" This texture stage state was not saved!");
		return;
	}
#endif
	SetTextureStageState(dwStage, Type, m_TextureStageStateStack[dwStage][Type].back());
	m_TextureStageStateStack[dwStage][Type].pop_back();
}

void CStateManager::SetTextureStageState(DWORD dwStage, ETextureStageState11 Type, DWORD dwValue)
{
	if (!m_bForce && m_CurrentState.m_TextureStates[dwStage][Type] == dwValue)
		return;

	m_CurrentState.m_TextureStates[dwStage][Type] = dwValue;

	if (m_pD3D11Renderer && dwStage < 2)
	{
		if (Type == TSS11_COLOROP || Type == TSS11_ALPHAOP)
		{
			int colorOp = MapTextureOp(m_CurrentState.m_TextureStates[dwStage][TSS11_COLOROP]);
			int alphaOp = MapTextureOp(m_CurrentState.m_TextureStates[dwStage][TSS11_ALPHAOP]);
			m_pD3D11Renderer->GetCbMgr()->SetTextureStageOp(dwStage, colorOp, alphaOp);
		}

		if (Type == TSS11_COLORARG1 || Type == TSS11_COLORARG2 ||
			Type == TSS11_ALPHAARG1 || Type == TSS11_ALPHAARG2)
		{
			int colorArg1 = (int)m_CurrentState.m_TextureStates[dwStage][TSS11_COLORARG1];
			int colorArg2 = (int)m_CurrentState.m_TextureStates[dwStage][TSS11_COLORARG2];
			int alphaArg1 = (int)m_CurrentState.m_TextureStates[dwStage][TSS11_ALPHAARG1];
			int alphaArg2 = (int)m_CurrentState.m_TextureStates[dwStage][TSS11_ALPHAARG2];
			m_pD3D11Renderer->GetCbMgr()->SetTextureStageArgs(dwStage, colorArg1, colorArg2, alphaArg1, alphaArg2);
		}

		if (Type == TSS11_TEXCOORDINDEX)
		{
			DWORD tciFlags = dwValue & 0xFFFF0000;
			int mode = 0;
			if (tciFlags == TSS11_TCI_CAMERASPACEPOSITION)
				mode = 1;
			else if (tciFlags == TSS11_TCI_CAMERASPACEREFLECTIONVECTOR)
				mode = 2;
			else if (tciFlags == TSS11_TCI_CAMERASPACENORMAL)
				mode = 3;
			m_pD3D11Renderer->GetCbMgr()->SetTexCoordGen(dwStage, mode);
		}
	}
}

void CStateManager::GetTextureStageState(DWORD dwStage, ETextureStageState11 Type, DWORD* pdwValue)
{
	*pdwValue = m_CurrentState.m_TextureStates[dwStage][Type];
}

// Sampler states
void CStateManager::SaveSamplerState(DWORD dwStage, ESamplerStateType11 Type, DWORD dwValue)
{
	m_SamplerStateStack[dwStage][Type].push_back(m_CurrentState.m_SamplerStates[dwStage][Type]);
	SetSamplerState(dwStage, Type, dwValue);
}

void CStateManager::RestoreSamplerState(DWORD dwStage, ESamplerStateType11 Type)
{
#ifdef _DEBUG
	if (m_SamplerStateStack[dwStage][Type].empty())
	{
		Tracenf("CStateManager::RestoreSamplerState - This sampler state was not saved [%u, %u]\n", dwStage, Type);
		StateManager_Assert(!"This sampler state was not saved!");
		return;
	}
#endif
	SetSamplerState(dwStage, Type, m_SamplerStateStack[dwStage][Type].back());
	m_SamplerStateStack[dwStage][Type].pop_back();
}

void CStateManager::SetSamplerState(DWORD dwStage, ESamplerStateType11 Type, DWORD dwValue)
{
	if (dwStage >= 2)
		return;

	if (!m_bForce && m_CurrentState.m_SamplerStates[dwStage][Type] == dwValue)
		return;

	m_CurrentState.m_SamplerStates[dwStage][Type] = dwValue;

	if (m_pD3D11Renderer)
	{
		DWORD minFilter = m_CurrentState.m_SamplerStates[dwStage][SS11_MINFILTER];
		DWORD magFilter = m_CurrentState.m_SamplerStates[dwStage][SS11_MAGFILTER];
		DWORD mipFilter = m_CurrentState.m_SamplerStates[dwStage][SS11_MIPFILTER];
		DWORD addrU = m_CurrentState.m_SamplerStates[dwStage][SS11_ADDRESSU];
		DWORD addrV = m_CurrentState.m_SamplerStates[dwStage][SS11_ADDRESSV];

		m_pD3D11Renderer->SetSamplerState(
			dwStage,
			MapSamplerFilter11(minFilter, magFilter, mipFilter),
			(D3D11_TEXTURE_ADDRESS_MODE)addrU,
			(D3D11_TEXTURE_ADDRESS_MODE)addrV);
	}
}

void CStateManager::GetSamplerState(DWORD dwStage, ESamplerStateType11 Type, DWORD* pdwValue)
{
	*pdwValue = m_CurrentState.m_SamplerStates[dwStage][Type];
}

// *** These states are cached, but not protected from multiple sends of the same value.
// Transform
void CStateManager::SaveTransform(ETransform Type, const D3DXMATRIX* pMatrix)
{
	m_TransformStack[Type].push_back(m_CurrentState.m_Matrices[Type]);
	SetTransform(Type, pMatrix);
}

void CStateManager::RestoreTransform(ETransform Type)
{
#ifdef _DEBUG
	if (m_TransformStack[Type].empty())
	{
		Tracef(" CStateManager::RestoreTransform - This transform was not saved [%d]\n", Type);
		StateManager_Assert(!" This render state was not saved!");
	}
#endif _DEBUG

	SetTransform(Type, &m_TransformStack[Type].back());
	m_TransformStack[Type].pop_back();
}

// Transforms — forward to D3D11 renderer constant buffer
void CStateManager::SetTransform(ETransform Type, const D3DXMATRIX* pMatrix)
{
	m_CurrentState.m_Matrices[Type] = *pMatrix;

	if (m_pD3D11Renderer)
	{
		if (Type == World)
			m_pD3D11Renderer->GetCbMgr()->SetWorldMatrix(*pMatrix);
		else if (Type == View)
			m_pD3D11Renderer->GetCbMgr()->SetViewMatrix(*pMatrix);
		else if (Type == Projection)
			m_pD3D11Renderer->GetCbMgr()->SetProjMatrix(*pMatrix);
		else if (Type == Texture0)
			m_pD3D11Renderer->GetCbMgr()->SetTexTransform(0, *pMatrix);
		else if (Type == Texture1)
			m_pD3D11Renderer->GetCbMgr()->SetTexTransform(1, *pMatrix);
	}
}

void CStateManager::GetTransform(ETransform Type, D3DXMATRIX* pOut)
{
	UINT idx = static_cast<UINT>(Type);
	if (idx >= SM_MAX_TRANSFORMS || !pOut)
		return;
	*pOut = m_CurrentState.m_Matrices[idx];
}

void CStateManager::SaveStreamSource(UINT StreamNumber, ID3D11Buffer* pStreamData, UINT Stride)
{
	m_StreamStack[StreamNumber].push_back(m_CurrentState.m_StreamData[StreamNumber]);
	SetStreamSource(StreamNumber, pStreamData, Stride);
}

void CStateManager::RestoreStreamSource(UINT StreamNumber)
{
	const auto& topStream = m_StreamStack[StreamNumber].back();
	SetStreamSource(StreamNumber,
		topStream.m_lpStreamData,
		topStream.m_Stride);
	m_StreamStack[StreamNumber].pop_back();
}

void CStateManager::SetStreamSource(UINT StreamNumber, ID3D11Buffer* pStreamData, UINT Stride)
{
	CStreamData kStreamData(pStreamData, Stride);

	if (!m_bForce && m_CurrentState.m_StreamData[StreamNumber] == kStreamData)
		return;

	m_CurrentState.m_StreamData[StreamNumber] = kStreamData;

	if (CGraphicBase::ms_lpd3d11Context)
	{
		UINT uStride = Stride;
		UINT uOffset = 0;
		CGraphicBase::ms_lpd3d11Context->IASetVertexBuffers(StreamNumber, 1, &pStreamData, &uStride, &uOffset);
	}
}

void CStateManager::SaveIndices(ID3D11Buffer* pIndexData, UINT BaseVertexIndex)
{
	m_IndexStack.push_back(m_CurrentState.m_IndexData);
	SetIndices(pIndexData, BaseVertexIndex);
}

void CStateManager::RestoreIndices()
{
	const auto& topIndex = m_IndexStack.back();
	SetIndices(topIndex.m_lpIndexData, topIndex.m_BaseVertexIndex);
	m_IndexStack.pop_back();
}

void CStateManager::SetIndices(ID3D11Buffer* pIndexData, UINT BaseVertexIndex)
{
	CIndexData kIndexData(pIndexData, BaseVertexIndex);

	if (!m_bForce && m_CurrentState.m_IndexData == kIndexData)
		return;

	m_CurrentState.m_IndexData = kIndexData;

	if (CGraphicBase::ms_lpd3d11Context)
	{
		CGraphicBase::ms_lpd3d11Context->IASetIndexBuffer(pIndexData, DXGI_FORMAT_R16_UINT, 0);
	}
}

static UINT GetPrimitiveVertexCount(D3D11_PRIMITIVE_TOPOLOGY topology, UINT primCount)
{
	switch (topology)
	{
	case D3D11_PRIMITIVE_TOPOLOGY_POINTLIST:		return primCount;
	case D3D11_PRIMITIVE_TOPOLOGY_LINELIST:			return primCount * 2;
	case D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP:		return primCount + 1;
	case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:		return primCount * 3;
	case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:	return primCount + 2;
	default:										return primCount * 3;
	}
}

HRESULT CStateManager::DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY Topology, UINT PrimitiveCount, UINT StartVertex_VertexStride, const void* pVertexData)
{
#ifdef _DEBUG
	++m_iDrawCallCount;
#endif

	if (!CGraphicBase::ms_lpd3d11Context || !m_pD3D11Renderer)
		return E_FAIL;

	UINT vertexCount = GetPrimitiveVertexCount(Topology, PrimitiveCount);

	ID3D11Buffer* vb = nullptr;

	if (pVertexData)
	{
		//StartVertex_VertexStride = stride
		UINT stride = StartVertex_VertexStride;

		if (!CGraphicBase::ms_lpd3d11Device || !stride)
			return E_FAIL;

		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = vertexCount * stride;
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = pVertexData;

		if (FAILED(CGraphicBase::ms_lpd3d11Device->CreateBuffer(&bd, &sd, &vb)))
			return E_FAIL;

		UINT offset = 0;
		CGraphicBase::ms_lpd3d11Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

		StartVertex_VertexStride = 0; // = startVertex
	}

	m_pD3D11Renderer->FlushAllState();
	CGraphicBase::ms_lpd3d11Context->IASetPrimitiveTopology(Topology);
	CGraphicBase::ms_lpd3d11Context->Draw(vertexCount, StartVertex_VertexStride);

	if (vb)
		vb->Release();

	return S_OK;
}

HRESULT CStateManager::DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY Topology, INT BaseVertexIndex, UINT StartIndex, UINT PrimitiveCount)
{
#ifdef _DEBUG
	++m_iDrawCallCount;
#endif

	++g_frameDrawCount;

	UINT stride = m_CurrentState.m_StreamData[0].m_Stride;
	if ((stride / 4) < 64)
		g_drawByStride[stride / 4]++;

	if (!CGraphicBase::ms_lpd3d11Context || !m_pD3D11Renderer)
		return E_FAIL;

	m_pD3D11Renderer->FlushAllState();
	CGraphicBase::ms_lpd3d11Context->IASetPrimitiveTopology(Topology);
	CGraphicBase::ms_lpd3d11Context->DrawIndexed(GetPrimitiveVertexCount(Topology, PrimitiveCount), StartIndex, BaseVertexIndex);

	return S_OK;
}

HRESULT CStateManager::DrawTriangleFan11(UINT PrimitiveCount, const void* pVertexData, UINT VertexStride)
{
#ifdef _DEBUG
	++m_iDrawCallCount;
#endif

	if (!CGraphicBase::ms_lpd3d11Context || !CGraphicBase::ms_lpd3d11Device || !m_pD3D11Renderer)
		return E_FAIL;

	if (!pVertexData || !VertexStride || !PrimitiveCount)
		return E_FAIL;

	UINT finalVertexCount = PrimitiveCount * 3;
	std::vector<BYTE> data(finalVertexCount * VertexStride);

	const BYTE* src = (const BYTE*)pVertexData;
	BYTE* dst = data.data();

	for (UINT i = 0; i < PrimitiveCount; ++i)
	{
		memcpy(dst, src, VertexStride);
		dst += VertexStride;
		memcpy(dst, src + (i + 1) * VertexStride, VertexStride);
		dst += VertexStride;
		memcpy(dst, src + (i + 2) * VertexStride, VertexStride);
		dst += VertexStride;
	}

	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = finalVertexCount * VertexStride;
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = data.data();

	ID3D11Buffer* vb = nullptr;
	if (FAILED(CGraphicBase::ms_lpd3d11Device->CreateBuffer(&bd, &sd, &vb)))
		return E_FAIL;

	UINT stride = VertexStride;
	UINT offset = 0;

	CGraphicBase::ms_lpd3d11Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

	m_pD3D11Renderer->FlushAllState();
	CGraphicBase::ms_lpd3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CGraphicBase::ms_lpd3d11Context->Draw(finalVertexCount, 0);

	vb->Release();
	return S_OK;
}

#ifdef _DEBUG
void CStateManager::ResetDrawCallCounter()
{
	m_iLastDrawCallCount = m_iDrawCallCount;
	m_iDrawCallCount = 0;
}

int CStateManager::GetDrawCallCount() const
{
	return m_iLastDrawCallCount;
}
#endif