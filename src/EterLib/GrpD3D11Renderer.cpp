#include "StdAfx.h"
#include "GrpD3D11Renderer.h"
#include "GrpD3D11Shaders.h"
#include "EterBase/Stl.h"
#include "EterBase/Debug.h"
#include "qMin32Lib/ConstantBuffer.h"
#include "qMin32Lib/DxManager.h"
#include "GrpBase.h"

CD3D11Renderer::CD3D11Renderer()
{
	memset(&m_curBlendKey, 0, sizeof(m_curBlendKey));
	memset(&m_curDepthKey, 0, sizeof(m_curDepthKey));
	memset(&m_curRasterKey, 0, sizeof(m_curRasterKey));
}

CD3D11Renderer::~CD3D11Renderer()
{
	Destroy();
}

bool CD3D11Renderer::CreateConstantBuffers()
{
	_mgr->CreateConstantBuffer(m_pCBPerFrame, sizeof(CBPerFrame));
	_mgr->CreateConstantBuffer(m_pCBMaterial, sizeof(CBMaterial));
	_mgr->CreateConstantBuffer(m_pCBLighting, sizeof(CBLighting));
	_mgr->CreateConstantBuffer(m_pCBTexTransform, sizeof(CBTexTransform));
	_mgr->CreateConstantBuffer(m_pCBFog, sizeof(CBFog));
	_mgr->CreateConstantBuffer(m_pCBScreenSize, sizeof(CBScreenSize));

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Initialize — compile shaders, create buffers, set defaults
///////////////////////////////////////////////////////////////////////////////
bool CD3D11Renderer::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	m_pDevice = pDevice;
	m_pContext = pContext;

	if (!CreateConstantBuffers())
	{
		Tracenf("D3D11Renderer: Failed constant buffers"); return false;
	}

	// Create default sampler (linear wrap)
	D3D11_SAMPLER_DESC sd = {};
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.MaxAnisotropy = 1;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.MaxLOD = D3D11_FLOAT32_MAX;
	if (FAILED(m_pDevice->CreateSamplerState(&sd, m_pDefaultSampler.GetAddressOf())))
	{
		Tracenf("D3D11Renderer: Failed default sampler"); return false;
	}

	// Set default state
	m_eCurrentFormat = VF_PDT;
	SetVertexFormat(VF_PDT);

	m_pContext->PSSetSamplers(0, 1, m_pDefaultSampler.GetAddressOf());
	m_pContext->PSSetSamplers(1, 1, m_pDefaultSampler.GetAddressOf());

	// Default blend: alpha blend off
	m_curBlendKey.blendEnable = FALSE;
	m_curBlendKey.srcBlend = D3D11_BLEND_SRC_ALPHA;
	m_curBlendKey.destBlend = D3D11_BLEND_INV_SRC_ALPHA;
	m_curBlendKey.blendOp = D3D11_BLEND_OP_ADD;
	m_curBlendKey.srcBlendAlpha = D3D11_BLEND_ONE;
	m_curBlendKey.destBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	m_curBlendKey.colorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	// Default depth: enabled, write, LESSEQUAL
	m_curDepthKey.depthEnable = TRUE;
	m_curDepthKey.depthWrite = TRUE;
	m_curDepthKey.depthFunc = D3D11_COMPARISON_LESS_EQUAL;
	m_curDepthKey.stencilEnable = FALSE;

	// Default raster: solid, NO CULL (debug — was D3D11_CULL_BACK)
	m_curRasterKey.fillMode = D3D11_FILL_SOLID;
	m_curRasterKey.cullMode = D3D11_CULL_NONE;
	m_curRasterKey.scissorEnable = FALSE;

	// Default material CB
	m_cbMaterial.textureFactor[0] = m_cbMaterial.textureFactor[1] = m_cbMaterial.textureFactor[2] = m_cbMaterial.textureFactor[3] = 1.0f;
	m_cbMaterial.useTexture0 = 1;
	m_cbMaterial.useTexture1 = 0;
	m_cbMaterial.colorOp0 = 0;
	m_cbMaterial.alphaOp0 = 1;
	m_cbMaterial.colorOp1 = -1;
	m_cbMaterial.alphaOp1 = -1;
	m_cbMaterial.colorArg10 = TA11_TEXTURE;
	m_cbMaterial.colorArg20 = TA11_DIFFUSE;
	m_cbMaterial.alphaArg10 = TA11_TEXTURE;
	m_cbMaterial.alphaArg20 = TA11_DIFFUSE;
	m_cbMaterial.colorArg11 = TA11_TEXTURE;
	m_cbMaterial.colorArg21 = TA11_CURRENT;
	m_cbMaterial.alphaArg11 = TA11_TEXTURE;
	m_cbMaterial.alphaArg21 = TA11_CURRENT;
	m_cbMaterial.texCoordGen1 = 0;

	// Default lighting
	m_cbLighting.lightAmbient[0] = m_cbLighting.lightAmbient[1] = m_cbLighting.lightAmbient[2] = 0.0f;
	m_cbLighting.lightAmbient[3] = 1.0f;
	m_cbLighting.matDiffuse[0] = m_cbLighting.matDiffuse[1] = m_cbLighting.matDiffuse[2] = m_cbLighting.matDiffuse[3] = 1.0f;
	m_cbLighting.matAmbient[0] = m_cbLighting.matAmbient[1] = m_cbLighting.matAmbient[2] = m_cbLighting.matAmbient[3] = 1.0f;

	// Default transforms — identity
	D3DXMatrixIdentity(&m_cbPerFrame.matWorld);
	D3DXMatrixIdentity(&m_cbPerFrame.matView);
	D3DXMatrixIdentity(&m_cbPerFrame.matProj);
	D3DXMatrixIdentity(&m_cbTexTransform.matTexTransform0);
	D3DXMatrixIdentity(&m_cbTexTransform.matTexTransform1);

	// Flush everything
	m_bTransformDirty = m_bMaterialDirty = m_bLightingDirty = m_bFogDirty = true;
	m_bBlendDirty = m_bDepthDirty = m_bRasterDirty = true;
	FlushAllState();

	// Bind constant buffers to all shader stages
	_mgr->SetConstantBuffer(m_pCBPerFrame, 0);
	_mgr->SetConstantBuffer(m_pCBMaterial, 1);
	_mgr->SetConstantBuffer(m_pCBLighting, 2);
	_mgr->SetConstantBuffer(m_pCBTexTransform, 3);
	_mgr->SetConstantBuffer(m_pCBFog, 4);
	_mgr->SetConstantBuffer(m_pCBScreenSize, 5);

	Tracenf("D3D11Renderer: Initialization complete");
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Vertex format selection
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetVertexFormat(ED3D11VertexFormat eFormat)
{
	if (eFormat >= VF_COUNT || !m_pContext)
		return;

	m_eCurrentFormat = eFormat;

	_mgr->SetShader(eFormat);
}

ED3D11VertexFormat CD3D11Renderer::DetectVertexFormat(DWORD dwFVF)
{
	// D3DFVF_XYZRHW = pre-transformed
	if (dwFVF & D3DFVF_XYZRHW)
		return VF_SCREEN;

	bool hasNormal = (dwFVF & D3DFVF_NORMAL) != 0;
	bool hasDiffuse = (dwFVF & D3DFVF_DIFFUSE) != 0;
	int texCount = (dwFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

	if (hasNormal && texCount >= 2)
		return VF_PNT2;
	if (hasNormal && texCount >= 1)
		return VF_PNT;
	if (hasNormal)
		return VF_PN;
	if (hasDiffuse && texCount >= 2)
		return VF_PDT2;
	if (hasDiffuse && texCount >= 1)
		return VF_PDT;
	if (hasDiffuse)
		return VF_PD;
	if (texCount >= 1)
		return VF_PT;	// Position + TexCoord (no normal, no diffuse)

	// Default
	return VF_PDT;
}

///////////////////////////////////////////////////////////////////////////////
// Transform updates
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetWorldMatrix(const D3DXMATRIX& mat)
{
	m_cbPerFrame.matWorld = mat;
	m_bTransformDirty = true;
}

void CD3D11Renderer::SetViewMatrix(const D3DXMATRIX& mat)
{
	m_cbPerFrame.matView = mat;
	m_bTransformDirty = true;
}

void CD3D11Renderer::SetProjMatrix(const D3DXMATRIX& mat)
{
	m_cbPerFrame.matProj = mat;
	m_bTransformDirty = true;
}

void CD3D11Renderer::FlushTransforms()
{
	if (!m_bTransformDirty)
		return;

	m_pCBPerFrame->Update(m_cbPerFrame);

	m_bTransformDirty = false;
}

///////////////////////////////////////////////////////////////////////////////
// Blend state
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetAlphaBlendEnable(BOOL bEnable) { m_curBlendKey.blendEnable = bEnable; m_bBlendDirty = true; }
void CD3D11Renderer::SetSrcBlend(D3D11_BLEND blend) { m_curBlendKey.srcBlend = blend; m_bBlendDirty = true; }
void CD3D11Renderer::SetDestBlend(D3D11_BLEND blend) { m_curBlendKey.destBlend = blend; m_bBlendDirty = true; }
void CD3D11Renderer::SetBlendOp(D3D11_BLEND_OP op) { m_curBlendKey.blendOp = op; m_bBlendDirty = true; }
void CD3D11Renderer::SetColorWriteEnable(BYTE mask) { m_curBlendKey.colorWriteMask = mask & 0x0F; m_bBlendDirty = true; }

ID3D11BlendState* CD3D11Renderer::GetOrCreateBlendState(const SD3D11BlendKey& key)
{
	auto it = m_mapBlendStates.find(key);
	if (it != m_mapBlendStates.end())
		return it->second;

	D3D11_BLEND_DESC bd = {};
	bd.RenderTarget[0].BlendEnable = key.blendEnable;
	bd.RenderTarget[0].SrcBlend = key.srcBlend;
	bd.RenderTarget[0].DestBlend = key.destBlend;
	bd.RenderTarget[0].BlendOp = key.blendOp;
	bd.RenderTarget[0].SrcBlendAlpha = key.srcBlendAlpha;
	bd.RenderTarget[0].DestBlendAlpha = key.destBlendAlpha;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	// Use the tracked write mask. If it comes in as 0 (e.g. from an untranslated
	// D3D9 stencil-only pass) default to ALL so the screen is never silently blank.
	bd.RenderTarget[0].RenderTargetWriteMask = (key.colorWriteMask != 0)
		? key.colorWriteMask
		: D3D11_COLOR_WRITE_ENABLE_ALL;

	ID3D11BlendState* pState = NULL;
	m_pDevice->CreateBlendState(&bd, &pState);
	m_mapBlendStates[key] = pState;
	return pState;
}

void CD3D11Renderer::FlushBlendState()
{
	if (!m_bBlendDirty) return;
	ID3D11BlendState* pState = GetOrCreateBlendState(m_curBlendKey);
	if (pState)
	{
		float blendFactor[4] = { 0, 0, 0, 0 };
		m_pContext->OMSetBlendState(pState, blendFactor, 0xFFFFFFFF);
	}
	m_bBlendDirty = false;
}

///////////////////////////////////////////////////////////////////////////////
// Depth stencil state
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetZEnable(BOOL bEnable) { m_curDepthKey.depthEnable = bEnable; m_bDepthDirty = true; }
void CD3D11Renderer::SetZWriteEnable(BOOL bEnable) { m_curDepthKey.depthWrite = bEnable; m_bDepthDirty = true; }
void CD3D11Renderer::SetZFunc(D3D11_COMPARISON_FUNC func) { m_curDepthKey.depthFunc = func; m_bDepthDirty = true; }
void CD3D11Renderer::SetStencilEnable(BOOL bEnable) { m_curDepthKey.stencilEnable = bEnable; m_bDepthDirty = true; }

ID3D11DepthStencilState* CD3D11Renderer::GetOrCreateDepthState(const SD3D11DepthKey& key)
{
	auto it = m_mapDepthStates.find(key);
	if (it != m_mapDepthStates.end())
		return it->second;

	D3D11_DEPTH_STENCIL_DESC dd = {};
	dd.DepthEnable = key.depthEnable;
	dd.DepthWriteMask = key.depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	dd.DepthFunc = key.depthFunc;
	dd.StencilEnable = key.stencilEnable;

	ID3D11DepthStencilState* pState = NULL;
	m_pDevice->CreateDepthStencilState(&dd, &pState);
	m_mapDepthStates[key] = pState;
	return pState;
}

void CD3D11Renderer::FlushDepthState()
{
	if (!m_bDepthDirty) return;
	ID3D11DepthStencilState* pState = GetOrCreateDepthState(m_curDepthKey);
	if (pState)
		m_pContext->OMSetDepthStencilState(pState, 0);
	m_bDepthDirty = false;
}

///////////////////////////////////////////////////////////////////////////////
// Rasterizer state
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetCullMode(D3D11_CULL_MODE mode) { m_curRasterKey.cullMode = mode; m_bRasterDirty = true; }
void CD3D11Renderer::SetFillMode(D3D11_FILL_MODE mode) { m_curRasterKey.fillMode = mode; m_bRasterDirty = true; }
void CD3D11Renderer::SetScissorEnable(BOOL bEnable) { m_curRasterKey.scissorEnable = bEnable; m_bRasterDirty = true; }

ID3D11RasterizerState* CD3D11Renderer::GetOrCreateRasterState(const SD3D11RasterKey& key)
{
	auto it = m_mapRasterStates.find(key);
	if (it != m_mapRasterStates.end())
		return it->second;

	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = key.fillMode;
	rd.CullMode = key.cullMode;
	rd.FrontCounterClockwise = FALSE;
	rd.DepthClipEnable = TRUE;
	rd.ScissorEnable = key.scissorEnable;

	ID3D11RasterizerState* pState = NULL;
	m_pDevice->CreateRasterizerState(&rd, &pState);
	m_mapRasterStates[key] = pState;
	return pState;
}

void CD3D11Renderer::FlushRasterState()
{
	if (!m_bRasterDirty) return;
	ID3D11RasterizerState* pState = GetOrCreateRasterState(m_curRasterKey);
	if (pState)
		m_pContext->RSSetState(pState);
	m_bRasterDirty = false;
}

///////////////////////////////////////////////////////////////////////////////
// Alpha test (shader-based in D3D11)
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetAlphaTestEnable(BOOL bEnable)
{
	m_cbMaterial.alphaTestEnable = bEnable ? 1 : 0;
	m_bMaterialDirty = true;
}

void CD3D11Renderer::SetAlphaRef(DWORD dwRef)
{
	m_cbMaterial.alphaRef = (int)(dwRef & 0xFF);
	m_bMaterialDirty = true;
}

///////////////////////////////////////////////////////////////////////////////
// Lighting
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetLightingEnable(BOOL bEnable)
{
	m_cbLighting.lightingEnable = bEnable ? 1 : 0;
	m_bLightingDirty = true;
}

void CD3D11Renderer::SetLight(DWORD index, const D3DLIGHT9* pLight)
{
	if (index > 0 || !pLight) return; // Only support light 0 for now

	m_cbLighting.lightDir[0] = pLight->Direction.x;
	m_cbLighting.lightDir[1] = pLight->Direction.y;
	m_cbLighting.lightDir[2] = pLight->Direction.z;
	m_cbLighting.lightDir[3] = 0.0f;

	m_cbLighting.lightDiffuse[0] = pLight->Diffuse.r;
	m_cbLighting.lightDiffuse[1] = pLight->Diffuse.g;
	m_cbLighting.lightDiffuse[2] = pLight->Diffuse.b;
	m_cbLighting.lightDiffuse[3] = pLight->Diffuse.a;

	m_bLightingDirty = true;
}

void CD3D11Renderer::SetMaterial(const D3DMATERIAL9* pMaterial)
{
	m_cbLighting.matDiffuse[0] = pMaterial->Diffuse.r;
	m_cbLighting.matDiffuse[1] = pMaterial->Diffuse.g;
	m_cbLighting.matDiffuse[2] = pMaterial->Diffuse.b;
	m_cbLighting.matDiffuse[3] = pMaterial->Diffuse.a;

	m_cbLighting.matAmbient[0] = pMaterial->Ambient.r;
	m_cbLighting.matAmbient[1] = pMaterial->Ambient.g;
	m_cbLighting.matAmbient[2] = pMaterial->Ambient.b;
	m_cbLighting.matAmbient[3] = pMaterial->Ambient.a;

	m_cbLighting.matEmissive[0] = pMaterial->Emissive.r;
	m_cbLighting.matEmissive[1] = pMaterial->Emissive.g;
	m_cbLighting.matEmissive[2] = pMaterial->Emissive.b;
	m_cbLighting.matEmissive[3] = pMaterial->Emissive.a;

	m_bLightingDirty = true;
}

void CD3D11Renderer::SetAmbient(DWORD dwColor)
{
	m_cbLighting.lightAmbient[0] = ((dwColor >> 16) & 0xFF) / 255.0f;
	m_cbLighting.lightAmbient[1] = ((dwColor >> 8) & 0xFF) / 255.0f;
	m_cbLighting.lightAmbient[2] = (dwColor & 0xFF) / 255.0f;
	m_cbLighting.lightAmbient[3] = ((dwColor >> 24) & 0xFF) / 255.0f;
	m_bLightingDirty = true;
}

void CD3D11Renderer::FlushLighting()
{
	if (!m_bLightingDirty) return;

	m_pCBLighting->Update(m_cbLighting);

	m_bLightingDirty = false;
}

///////////////////////////////////////////////////////////////////////////////
// Fog
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetFogEnable(BOOL bEnable) { m_cbFog.fogEnable = bEnable ? 1 : 0; m_bFogDirty = true; }
void CD3D11Renderer::SetFogColor(DWORD dwColor)
{
	// D3D9 DWORD color format is 0xAARRGGBB
	m_cbFog.fogColor[0] = ((dwColor >> 16) & 0xFF) / 255.0f;	// R
	m_cbFog.fogColor[1] = ((dwColor >> 8) & 0xFF) / 255.0f;	// G
	m_cbFog.fogColor[2] = (dwColor & 0xFF) / 255.0f;	// B
	m_cbFog.fogColor[3] = 1.0f;
	m_bFogDirty = true;
}
void CD3D11Renderer::SetFogStart(float fStart) { m_cbFog.fogStart = fStart; m_bFogDirty = true; }
void CD3D11Renderer::SetFogEnd(float fEnd) { m_cbFog.fogEnd = fEnd; m_bFogDirty = true; }

void CD3D11Renderer::FlushFog()
{
	if (!m_bFogDirty) return;
	m_pCBFog->Update(m_cbFog);

	m_bFogDirty = false;
}

///////////////////////////////////////////////////////////////////////////////
// Material / texture stage state emulation
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetTextureFactor(DWORD dwFactor)
{
	// D3D9 DWORD color format is 0xAARRGGBB
	m_cbMaterial.textureFactor[0] = ((dwFactor >> 16) & 0xFF) / 255.0f;	// R
	m_cbMaterial.textureFactor[1] = ((dwFactor >> 8) & 0xFF) / 255.0f;	// G
	m_cbMaterial.textureFactor[2] = (dwFactor & 0xFF) / 255.0f;	// B
	m_cbMaterial.textureFactor[3] = ((dwFactor >> 24) & 0xFF) / 255.0f;	// A
	m_bMaterialDirty = true;
}

void CD3D11Renderer::SetTextureStageOp(DWORD dwStage, int colorOp, int alphaOp)
{
	if (dwStage == 0) { m_cbMaterial.colorOp0 = colorOp; m_cbMaterial.alphaOp0 = alphaOp; }
	else if (dwStage == 1) { m_cbMaterial.colorOp1 = colorOp; m_cbMaterial.alphaOp1 = alphaOp; }
	m_bMaterialDirty = true;
}

void CD3D11Renderer::SetTextureStageArgs(DWORD dwStage, int colorArg1, int colorArg2, int alphaArg1, int alphaArg2)
{
	if (dwStage == 0)
	{
		m_cbMaterial.colorArg10 = colorArg1;
		m_cbMaterial.colorArg20 = colorArg2;
		m_cbMaterial.alphaArg10 = alphaArg1;
		m_cbMaterial.alphaArg20 = alphaArg2;
	}
	else if (dwStage == 1)
	{
		m_cbMaterial.colorArg11 = colorArg1;
		m_cbMaterial.colorArg21 = colorArg2;
		m_cbMaterial.alphaArg11 = alphaArg1;
		m_cbMaterial.alphaArg21 = alphaArg2;
	}
	m_bMaterialDirty = true;
}

void CD3D11Renderer::SetTexCoordGen(DWORD dwStage, int mode)
{
	if (dwStage == 1)
	{
		m_cbMaterial.texCoordGen1 = mode;
		m_bMaterialDirty = true;
	}
}

void CD3D11Renderer::FlushMaterial()
{
	if (!m_bMaterialDirty) return;

	m_pCBMaterial->Update(m_cbMaterial);

	m_bMaterialDirty = false;
}

///////////////////////////////////////////////////////////////////////////////
// Texture binding
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV)
{
	if (dwStage == 0)
		m_cbMaterial.useTexture0 = pSRV ? 1 : 0;
	else if (dwStage == 1)
		m_cbMaterial.useTexture1 = pSRV ? 1 : 0;
	m_bMaterialDirty = true;

	if (!pSRV)
	{
		ID3D11ShaderResourceView* pNull = NULL;
		m_pContext->PSSetShaderResources(dwStage, 1, &pNull);
		return;
	}

	m_pContext->PSSetShaderResources(dwStage, 1, &pSRV);
}

///////////////////////////////////////////////////////////////////////////////
// Texture transform
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetTexTransform(DWORD dwStage, const D3DXMATRIX& mat)
{
	if (dwStage == 0)
		m_cbTexTransform.matTexTransform0 = mat;
	else if (dwStage == 1)
		m_cbTexTransform.matTexTransform1 = mat;
	else
		return;

	m_pCBTexTransform->Update(m_cbTexTransform);
}

void CD3D11Renderer::SetTexTransform(const D3DXMATRIX& mat)
{
	SetTexTransform(0, mat);
}

///////////////////////////////////////////////////////////////////////////////
// Sampler state
///////////////////////////////////////////////////////////////////////////////
ID3D11SamplerState* CD3D11Renderer::GetOrCreateSamplerState(const SD3D11SamplerKey& key)
{
	auto it = m_mapSamplerStates.find(key);
	if (it != m_mapSamplerStates.end())
		return it->second;

	D3D11_SAMPLER_DESC sd = {};
	sd.Filter = key.filter;
	sd.AddressU = key.addrU;
	sd.AddressV = key.addrV;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.MaxAnisotropy = (key.filter == D3D11_FILTER_ANISOTROPIC) ? 8 : 1;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.MinLOD = 0.0f;
	sd.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* pState = NULL;
	m_pDevice->CreateSamplerState(&sd, &pState);
	m_mapSamplerStates[key] = pState;
	return pState;
}

void CD3D11Renderer::SetSamplerState(DWORD dwStage, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addrU, D3D11_TEXTURE_ADDRESS_MODE addrV)
{
	if (!m_pDevice || !m_pContext)
		return;

	SD3D11SamplerKey key;
	key.filter = filter;
	key.addrU = addrU;
	key.addrV = addrV;

	ID3D11SamplerState* pSampler = GetOrCreateSamplerState(key);
	if (pSampler)
		m_pContext->PSSetSamplers(dwStage, 1, &pSampler);
	else
		m_pContext->PSSetSamplers(dwStage, 1, m_pDefaultSampler.GetAddressOf());
}

///////////////////////////////////////////////////////////////////////////////
// Screen size (for XYZRHW conversion)
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::SetScreenSize(float width, float height)
{
	m_cbScreenSize.screenWidth = width;
	m_cbScreenSize.screenHeight = height;

	m_pCBScreenSize->Update(m_cbScreenSize);
}

///////////////////////////////////////////////////////////////////////////////
// Flush all dirty state before a draw call
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::FlushAllState()
{
	FlushTransforms();
	FlushMaterial();
	FlushLighting();
	FlushFog();
	FlushBlendState();
	FlushDepthState();
	FlushRasterState();
}

///////////////////////////////////////////////////////////////////////////////
// Destroy
///////////////////////////////////////////////////////////////////////////////
void CD3D11Renderer::Destroy()
{
	for (auto& pair : m_mapBlendStates)  safe_release(pair.second);
	m_mapBlendStates.clear();

	for (auto& pair : m_mapDepthStates)  safe_release(pair.second);
	m_mapDepthStates.clear();

	for (auto& pair : m_mapRasterStates) safe_release(pair.second);
	m_mapRasterStates.clear();

	for (auto& pair : m_mapSamplerStates) safe_release(pair.second);
	m_mapSamplerStates.clear();
}