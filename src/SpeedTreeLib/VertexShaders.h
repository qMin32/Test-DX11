#pragma once
#include "SpeedTreeConfig.h"
#include "EterLib/D3D9Compat.h"
#include "EterLib/D3DXMathCompat.h"
#include "qMin32Lib/DxManager.h"
#include "EterBase/Debug.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <memory>
#include <cstdint>

#pragma comment(lib, "d3dcompiler.lib")

#include "EterLib/GrpBase.h"

struct SFVFBranchVertex
{
	D3DXVECTOR3 m_vPosition;
#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
	D3DXVECTOR3 m_vNormal;
#else
	DWORD m_dwDiffuseColor;
#endif
	FLOAT m_fTexCoords[2];
#ifdef WRAPPER_RENDER_SELF_SHADOWS
	FLOAT m_fShadowCoords[2];
#endif
#ifdef WRAPPER_USE_GPU_WIND
	FLOAT m_fWindIndex;
	FLOAT m_fWindWeight;
#endif
};

struct SFVFLeafVertex
{
	D3DXVECTOR3 m_vPosition;
#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
	D3DXVECTOR3 m_vNormal;
#else
	DWORD m_dwDiffuseColor;
#endif
	FLOAT m_fTexCoords[2];
#if defined WRAPPER_USE_GPU_WIND || defined WRAPPER_USE_GPU_LEAF_PLACEMENT
	FLOAT m_fWindIndex;
	FLOAT m_fWindWeight;
	FLOAT m_fLeafPlacementIndex;
	FLOAT m_fLeafScalarValue;
#endif
};

class CSpeedTreeShader
{
public:
	bool Create(const char* vsEntry, const D3D11_INPUT_ELEMENT_DESC* layout, UINT layoutCount)
	{
		if (!_mgr || !_mgr->GetDevice())
			return false;

		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> err;

		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		flags |= D3DCOMPILE_DEBUG;
#endif

		HRESULT hr = D3DCompile(GetSource(), strlen(GetSource()), "SpeedTreeDX11", nullptr, nullptr, vsEntry, "vs_4_0", flags, 0, vsBlob.GetAddressOf(), err.GetAddressOf());
		if (FAILED(hr))
		{
			TraceError("SpeedTree VS compile failed: %s", err ? (const char*)err->GetBufferPointer() : "unknown");
			return false;
		}

		hr = D3DCompile(GetSource(), strlen(GetSource()), "SpeedTreeDX11", nullptr, nullptr, "PSMain", "ps_4_0", flags, 0, psBlob.GetAddressOf(), err.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			TraceError("SpeedTree PS compile failed: %s", err ? (const char*)err->GetBufferPointer() : "unknown");
			return false;
		}

		hr = _mgr->GetDevice()->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vs.GetAddressOf());
		if (FAILED(hr))
			return false;

		hr = _mgr->GetDevice()->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_ps.GetAddressOf());
		if (FAILED(hr))
			return false;

		hr = _mgr->GetDevice()->CreateInputLayout(layout, layoutCount, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_layout.GetAddressOf());
		return SUCCEEDED(hr);
	}

	void Set() const
	{
		if (!_mgr || !_mgr->GetDeviceContext())
			return;

		ID3D11DeviceContext* ctx = _mgr->GetDeviceContext();
		ctx->IASetInputLayout(m_layout.Get());
		ctx->VSSetShader(m_vs.Get(), nullptr, 0);
		ctx->PSSetShader(m_ps.Get(), nullptr, 0);
	}

	bool IsValid() const { return m_vs && m_ps && m_layout; }

private:
	static const char* GetSource()
	{
		return R"(
cbuffer CBPerFrame : register(b0)
{
	row_major float4x4 gWorld;
	row_major float4x4 gView;
	row_major float4x4 gProj;
};

cbuffer CBMaterial : register(b1)
{
	float4 gTextureFactor;
	int gUseTexture0;
	int gUseTexture1;
	int gColorOp0;
	int gAlphaOp0;
	int gColorOp1;
	int gAlphaOp1;
	int gColorArg10;
	int gColorArg20;
	int gAlphaArg10;
	int gAlphaArg20;
	int gColorArg11;
	int gColorArg21;
	int gAlphaArg11;
	int gAlphaArg21;
	int gAlphaTestEnable;
	int gAlphaRef;
	int gTexCoordGen1;
	int gPadMat1;
	int gPadMat2;
	int gPadMat3;
};

cbuffer CBSpeedTree : register(b7)
{
	row_major float4x4 gSpeedTreeCompound;
	float4 gTreePos;
	float4 gSpeedTreeFog;
	float4 gSpeedTreeLightDir;
	float4 gSpeedTreeLightAmbient;
	float4 gSpeedTreeLightDiffuse;
	float4 gSpeedTreeMaterialDiffuse;
	float4 gSpeedTreeMaterialAmbient;
	float4 gLeafLightingAdjustment;
	float4 gLeafTable[1024];
};

Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);
SamplerState samp0 : register(s0);
SamplerState samp1 : register(s1);

struct VSInBranch
{
	float3 pos : POSITION;
	float4 color : COLOR0;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

struct VSInLeaf
{
	float3 pos : POSITION;
	float4 color : COLOR0;
	float2 uv0 : TEXCOORD0;
	float4 leafData : TEXCOORD2;
};

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
	float fog : TEXCOORD2;
};

VSOut FinishVertex(float3 localPos, float4 color, float2 uv0, float2 uv1)
{
	VSOut o;
	float4 worldPos = float4(localPos + gTreePos.xyz, 1.0f);
	o.pos = mul(worldPos, gSpeedTreeCompound);
	o.color = color;
	o.uv0 = uv0;
	o.uv1 = uv1;
	float dist = mul(worldPos, gView).z;
	o.fog = saturate((gSpeedTreeFog.y - dist) * gSpeedTreeFog.z);
	return o;
}

VSOut VSBranch(VSInBranch v)
{
	return FinishVertex(v.pos, v.color, v.uv0, v.uv1);
}

VSOut VSLeaf(VSInLeaf v)
{
	float3 p = v.pos;

	uint tableIndex = (uint)(v.leafData.z + 0.5f);
	float side = v.leafData.x;

	if (tableIndex < 1024)
	{
		float3 o = gLeafTable[tableIndex].xyz;

		if (side > 0.5f)
			o = float3(-o.y, o.x, o.z);

		p += o * v.leafData.w;
	}

	return FinishVertex(p, v.color, v.uv0, float2(0.0f, 0.0f));
}

float4 PSMain(VSOut i) : SV_Target
{
	float4 texColor = tex0.Sample(samp0, i.uv0);

	float3 light = gSpeedTreeLightAmbient.rgb + gSpeedTreeLightDiffuse.rgb;
	light = saturate(light);

	float4 baseColor = texColor * i.color;
	baseColor.rgb *= light;

	if (gAlphaTestEnable != 0)
	{
		float alphaRef = (gAlphaRef > 1) ? ((float)gAlphaRef / 255.0f) : (float)gAlphaRef;
		clip(baseColor.a - alphaRef);
	}

	return baseColor;
}
)";
	}

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_layout;
};

using SpeedTreeShaderPtr = std::shared_ptr<CSpeedTreeShader>;

static SpeedTreeShaderPtr LoadBranchShader()
{
	D3D11_INPUT_ELEMENT_DESC desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#else
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#endif
	};

	SpeedTreeShaderPtr shader = std::make_shared<CSpeedTreeShader>();
	if (!shader->Create("VSBranch", desc, _countof(desc)))
		shader.reset();
	return shader;
}

static SpeedTreeShaderPtr LoadLeafShader()
{
	D3D11_INPUT_ELEMENT_DESC desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#if defined WRAPPER_USE_GPU_WIND || defined WRAPPER_USE_GPU_LEAF_PLACEMENT
		{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#endif
#else
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#if defined WRAPPER_USE_GPU_WIND || defined WRAPPER_USE_GPU_LEAF_PLACEMENT
		{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#endif
#endif
	};

	SpeedTreeShaderPtr shader = std::make_shared<CSpeedTreeShader>();
	if (!shader->Create("VSLeaf", desc, _countof(desc)))
		shader.reset();
	return shader;
}
