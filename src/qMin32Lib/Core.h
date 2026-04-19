#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <memory>

#include <wrl/client.h>
using namespace Microsoft::WRL;

class ConstantBuffer;
using CBufferPtr = std::shared_ptr<ConstantBuffer>;

class IndexBuffer;
using IBufferPtr = std::shared_ptr<IndexBuffer>;

class VertexBuffer;
using VBufferPtr = std::shared_ptr<VertexBuffer>;

class CShaders;
using ShaderPtr = std::shared_ptr<CShaders>;

class ShadersContainer;
using ShadersContainerPtr = std::shared_ptr<ShadersContainer>;

class CBManager;
using CBManagerPtr = std::shared_ptr<CBManager>;

class DxManager;
template <typename T> using RefPtr = std::shared_ptr<T>;
template <typename T> using UniquePtr = std::unique_ptr<T>;

// Vertex format enum matching the game's vertex types
enum ED3D11VertexFormat
{
	VF_PDT,		// Position + Diffuse + TexCoord
	VF_PNT,		// Position + Normal + TexCoord
	VF_PD,		// Position + Diffuse
	VF_PNT2,	// Position + Normal + TexCoord + TexCoord2
	VF_PN,		// Position + Normal only (terrain HTP)
	VF_SCREEN,	// Pre-transformed: XYZRHW + Diffuse + Specular + Tex2
	VF_PT,		// Position + TexCoord (effects, snow, minimap)
	VF_PDT2,	// Position + Diffuse + TexCoord + TexCoord2 (SpeedTree branches/fronds)

	VF_COUNT
};
