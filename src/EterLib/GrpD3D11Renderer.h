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

struct SBlendKeyHash { size_t operator()(const SD3D11BlendKey& k) const { size_t h = 0; auto p = (const char*)&k; for (size_t i = 0; i < sizeof(k); ++i) h = h * 131 + p[i]; return h; } };
struct SDepthKeyHash { size_t operator()(const SD3D11DepthKey& k) const { size_t h = 0; auto p = (const char*)&k; for (size_t i = 0; i < sizeof(k); ++i) h = h * 131 + p[i]; return h; } };
struct SRasterKeyHash { size_t operator()(const SD3D11RasterKey& k) const { size_t h = 0; auto p = (const char*)&k; for (size_t i = 0; i < sizeof(k); ++i) h = h * 131 + p[i]; return h; } };
struct SSamplerKeyHash { size_t operator()(const SD3D11SamplerKey& k) const { size_t h = 0; auto p = (const char*)&k; for (size_t i = 0; i < sizeof(k); ++i) h = h * 131 + p[i]; return h; } };

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

	CBManagerPtr GetCbMgr();

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

	// Texture binding → PSSetShaderResources
	void SetTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV);

	// Vertex format → selects shader + input layout
	void SetVertexFormat(ED3D11VertexFormat eFormat);
	ED3D11VertexFormat GetCurrentFormat() const { return m_eCurrentFormat; }

	// Sampler state
	void SetSamplerState(DWORD dwStage, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addrU, D3D11_TEXTURE_ADDRESS_MODE addrV);

	// Screen size (for XYZRHW conversion)
	void SetScreenSize(float width, float height);

	// --- Draw ---
	void FlushAllState();

private:
	bool CreateConstantBuffers();

	ID3D11BlendState* GetOrCreateBlendState(const SD3D11BlendKey& key);
	ID3D11DepthStencilState* GetOrCreateDepthState(const SD3D11DepthKey& key);
	ID3D11RasterizerState* GetOrCreateRasterState(const SD3D11RasterKey& key);
	ID3D11SamplerState* GetOrCreateSamplerState(const SD3D11SamplerKey& key);

	ComPtr<ID3D11Device>			m_pDevice = nullptr;
	ComPtr<ID3D11DeviceContext>		m_pContext = nullptr;

	// State object caches (created on demand, never destroyed until shutdown)
	std::unordered_map<SD3D11BlendKey, ID3D11BlendState*, SBlendKeyHash>  m_mapBlendStates;
	std::unordered_map<SD3D11DepthKey, ID3D11DepthStencilState*, SDepthKeyHash>  m_mapDepthStates;
	std::unordered_map<SD3D11RasterKey, ID3D11RasterizerState*, SRasterKeyHash> m_mapRasterStates;
	std::unordered_map<SD3D11SamplerKey, ID3D11SamplerState*, SSamplerKeyHash> m_mapSamplerStates;

	// Default samplers
	ComPtr<ID3D11SamplerState>		m_pDefaultSampler = nullptr;


	// Current state tracking
	ED3D11VertexFormat		m_eCurrentFormat = VF_PDT;

	SD3D11BlendKey			m_curBlendKey;
	SD3D11DepthKey			m_curDepthKey;
	SD3D11RasterKey			m_curRasterKey;

	bool					m_bBlendDirty = true;
	bool					m_bDepthDirty = true;
	bool					m_bRasterDirty = true;
};
