#pragma once

///////////////////////////////////////////////////////////////////////////////
// CD3D11Renderer — D3D11 rendering backend
//
// Owned by CStateManager. Translates D3D9-style state calls into D3D11
// context operations. Compiles and manages HLSL shaders, constant buffers,
// state objects, and input layouts for all vertex formats.
//
// The game codebase keeps using D3D9 enums (D3DRS_*, TSS11_*, D3DFVF_*)
// through StateManager's API. This class does the actual translation.
///////////////////////////////////////////////////////////////////////////////

#include <d3d11.h>
#include <d3dcompiler.h>
#include <unordered_map>
#include "GrpBase.h"
#include "StateManager.h"
#include "qMin32Lib/Core.h"

///////////////////////////////////////////////////////////////////////////////
// Constant buffer structures (must be 16-byte aligned)
///////////////////////////////////////////////////////////////////////////////

// b0: Transform matrices
struct CBPerFrame
{
	D3DXMATRIX matWorld;
	D3DXMATRIX matView;
	D3DXMATRIX matProj;
};

// b1: Material / texture stage state emulation
struct CBMaterial
{
	float textureFactor[4];

	int useTexture0;
	int useTexture1;
	int colorOp0;
	int alphaOp0;

	int colorOp1;
	int alphaOp1;
	int colorArg10;
	int colorArg20;

	int alphaArg10;
	int alphaArg20;
	int colorArg11;
	int colorArg21;

	int alphaArg11;
	int alphaArg21;
	int alphaTestEnable;
	int alphaRef;

	int texCoordGen1;   // 0=vertex UV, 1=camera-space position, 2=camera-space reflection
	int padMat1;
	int padMat2;
	int padMat3;
};

// b2: Lighting emulation
struct CBLighting
{
	float lightDir[4];
	float lightDiffuse[4];
	float lightAmbient[4];
	float matDiffuse[4];
	float matAmbient[4];
	float matEmissive[4];
	int lightingEnable;
	int pad0, pad1, pad2;
};

// b3: Texture coordinate transform
struct CBTexTransform
{
	D3DXMATRIX matTexTransform0;
	D3DXMATRIX matTexTransform1;
};

// b4: Fog
struct CBFog
{
	float fogColor[4];
	float fogStart;
	float fogEnd;
	int fogEnable;
	int fogPad;
};

// b5: Screen dimensions (for XYZRHW conversion)
struct CBScreenSize
{
	float screenWidth;
	float screenHeight;
	float pad0;
	float pad1;
};

static_assert((sizeof(CBMaterial) % 16) == 0, "CBMaterial must be 16-byte aligned");
static_assert((sizeof(CBTexTransform) % 16) == 0, "CBTexTransform must be 16-byte aligned");
static_assert((sizeof(CBFog) % 16) == 0, "CBFog must be 16-byte aligned");
static_assert((sizeof(CBScreenSize) % 16) == 0, "CBScreenSize must be 16-byte aligned");

///////////////////////////////////////////////////////////////////////////////
// Tracked D3D11 state (mirrors what we've set, avoids redundant API calls)
///////////////////////////////////////////////////////////////////////////////
struct SD3D11BlendKey
{
	BOOL blendEnable;
	D3D11_BLEND srcBlend;
	D3D11_BLEND destBlend;
	D3D11_BLEND_OP blendOp;
	D3D11_BLEND srcBlendAlpha;
	D3D11_BLEND destBlendAlpha;
	BYTE colorWriteMask;
	BYTE _pad[3];  // explicit padding for deterministic hash/comparison

	SD3D11BlendKey() { memset(this, 0, sizeof(*this)); }
	bool operator==(const SD3D11BlendKey& o) const { return memcmp(this, &o, sizeof(*this)) == 0; }
};

struct SD3D11DepthKey
{
	BOOL depthEnable;
	BOOL depthWrite;
	D3D11_COMPARISON_FUNC depthFunc;
	BOOL stencilEnable;

	bool operator==(const SD3D11DepthKey& o) const { return memcmp(this, &o, sizeof(*this)) == 0; }
};

struct SD3D11RasterKey
{
	D3D11_FILL_MODE fillMode;
	D3D11_CULL_MODE cullMode;
	BOOL scissorEnable;

	bool operator==(const SD3D11RasterKey& o) const { return memcmp(this, &o, sizeof(*this)) == 0; }
};

struct SD3D11SamplerKey
{
	D3D11_FILTER filter;
	D3D11_TEXTURE_ADDRESS_MODE addrU;
	D3D11_TEXTURE_ADDRESS_MODE addrV;

	bool operator==(const SD3D11SamplerKey& o) const { return memcmp(this, &o, sizeof(*this)) == 0; }
};

struct SBlendKeyHash  { size_t operator()(const SD3D11BlendKey&  k) const { size_t h = 0; auto p = (const char*)&k; for (size_t i = 0; i < sizeof(k); ++i) h = h * 131 + p[i]; return h; } };
struct SDepthKeyHash  { size_t operator()(const SD3D11DepthKey&  k) const { size_t h = 0; auto p = (const char*)&k; for (size_t i = 0; i < sizeof(k); ++i) h = h * 131 + p[i]; return h; } };
struct SRasterKeyHash { size_t operator()(const SD3D11RasterKey& k) const { size_t h = 0; auto p = (const char*)&k; for (size_t i = 0; i < sizeof(k); ++i) h = h * 131 + p[i]; return h; } };
struct SSamplerKeyHash{ size_t operator()(const SD3D11SamplerKey& k) const { size_t h = 0; auto p = (const char*)&k; for (size_t i = 0; i < sizeof(k); ++i) h = h * 131 + p[i]; return h; } };

///////////////////////////////////////////////////////////////////////////////
// CD3D11Renderer
///////////////////////////////////////////////////////////////////////////////
class CD3D11Renderer
{
public:
	CD3D11Renderer();
	~CD3D11Renderer();

	bool Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Destroy();

	// --- State translation (called by StateManager) ---

	// Transforms → constant buffer b0
	void SetWorldMatrix(const D3DXMATRIX& mat);
	void SetViewMatrix(const D3DXMATRIX& mat);
	void SetProjMatrix(const D3DXMATRIX& mat);
	void FlushTransforms();

	// Render states → blend/depth/rasterizer state objects
	void SetAlphaBlendEnable(BOOL bEnable);
	void SetSrcBlend(D3D11_BLEND blend);
	void SetDestBlend(D3D11_BLEND blend);
	void SetBlendOp(D3D11_BLEND_OP op);
	void SetAlphaTestEnable(BOOL bEnable);
	void SetAlphaRef(DWORD dwRef);
	void SetZEnable(BOOL bEnable);
	void SetZWriteEnable(BOOL bEnable);
	void SetZFunc(D3D11_COMPARISON_FUNC func);
	void SetStencilEnable(BOOL bEnable);
	void SetCullMode(D3D11_CULL_MODE mode);
	void SetFillMode(D3D11_FILL_MODE mode);
	void SetScissorEnable(BOOL bEnable);
	void SetColorWriteEnable(BYTE mask);
	void FlushBlendState();
	void FlushDepthState();
	void FlushRasterState();

	// Lighting → constant buffer b2
	void SetLightingEnable(BOOL bEnable);
	void SetLight(DWORD index, const D3DLIGHT9* pLight);
	void SetMaterial(const D3DMATERIAL9* pMaterial);
	void SetAmbient(DWORD dwColor);
	void FlushLighting();

	// Fog → constant buffer b4
	void SetFogEnable(BOOL bEnable);
	void SetFogColor(DWORD dwColor);
	void SetFogStart(float fStart);
	void SetFogEnd(float fEnd);
	void FlushFog();

	// Texture stage states → constant buffer b1
	void SetTextureFactor(DWORD dwFactor);
	void SetTextureStageOp(DWORD dwStage, int colorOp, int alphaOp);
	void SetTextureStageArgs(DWORD dwStage, int colorArg1, int colorArg2, int alphaArg1, int alphaArg2);
	void SetTexCoordGen(DWORD dwStage, int mode);  // 0=vertex UV, 1=cam pos, 2=cam reflection
	void FlushMaterial();

	// Texture binding → PSSetShaderResources
	void SetTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV);

	// Texture coordinate transform → constant buffer b3
	void SetTexTransform(DWORD dwStage, const D3DXMATRIX& mat);
	void SetTexTransform(const D3DXMATRIX& mat);

	// Vertex format → selects shader + input layout
	void SetVertexFormat(ED3D11VertexFormat eFormat);
	ED3D11VertexFormat DetectVertexFormat(DWORD dwFVF);
	ED3D11VertexFormat GetCurrentFormat() const { return m_eCurrentFormat; }

	// Sampler state
	void SetSamplerState(DWORD dwStage, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addrU, D3D11_TEXTURE_ADDRESS_MODE addrV);

	// Screen size (for XYZRHW conversion)
	void SetScreenSize(float width, float height);

	// --- Draw ---
	void FlushAllState();

private:
	bool CreateConstantBuffers();

	ID3D11BlendState*        GetOrCreateBlendState(const SD3D11BlendKey& key);
	ID3D11DepthStencilState* GetOrCreateDepthState(const SD3D11DepthKey& key);
	ID3D11RasterizerState*   GetOrCreateRasterState(const SD3D11RasterKey& key);
	ID3D11SamplerState*      GetOrCreateSamplerState(const SD3D11SamplerKey& key);

	ComPtr<ID3D11Device>			m_pDevice = nullptr;
	ComPtr<ID3D11DeviceContext>		m_pContext = nullptr;

	CBufferPtr			m_pCBPerFrame;
	CBufferPtr			m_pCBMaterial;
	CBufferPtr			m_pCBLighting;
	CBufferPtr			m_pCBTexTransform;
	CBufferPtr			m_pCBFog;
	CBufferPtr			m_pCBScreenSize;

	// State object caches (created on demand, never destroyed until shutdown)
	std::unordered_map<SD3D11BlendKey,   ID3D11BlendState*,        SBlendKeyHash>  m_mapBlendStates;
	std::unordered_map<SD3D11DepthKey,   ID3D11DepthStencilState*, SDepthKeyHash>  m_mapDepthStates;
	std::unordered_map<SD3D11RasterKey,  ID3D11RasterizerState*,   SRasterKeyHash> m_mapRasterStates;
	std::unordered_map<SD3D11SamplerKey, ID3D11SamplerState*,      SSamplerKeyHash> m_mapSamplerStates;

	// Default samplers
	ComPtr<ID3D11SamplerState>		m_pDefaultSampler = nullptr;


	// Current state tracking
	ED3D11VertexFormat		m_eCurrentFormat = VF_PDT;

	CBPerFrame				m_cbPerFrame = {};
	CBMaterial				m_cbMaterial = {};
	CBLighting				m_cbLighting = {};
	CBTexTransform			m_cbTexTransform = {};
	CBFog					m_cbFog = {};
	CBScreenSize			m_cbScreenSize = {};

	SD3D11BlendKey			m_curBlendKey;
	SD3D11DepthKey			m_curDepthKey;
	SD3D11RasterKey			m_curRasterKey;
	bool					m_bTransformDirty = true;
	bool					m_bMaterialDirty = true;
	bool					m_bLightingDirty = true;
	bool					m_bFogDirty = true;
	bool					m_bBlendDirty = true;
	bool					m_bDepthDirty = true;
	bool					m_bRasterDirty = true;
};