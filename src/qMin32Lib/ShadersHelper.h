#pragma once
#include "Core.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>

enum ShaderFlags : uint32_t
{
	SHADER_NONE = 0,
	HAS_TEX2 = 1 << 0,
	IS_SKINNED = 1 << 1,
};

static std::string BuildShaderPath(const std::string& basePath)
{
#ifdef _DEBUG
	return "shaders/debug/" + basePath + ".hlsl";
#else
	return "shaders/release/" + basePath + ".cso";
#endif
}

static std::wstring StringToWString(const std::string& s)
{
	return std::wstring(s.begin(), s.end());
}

static std::vector<D3D_SHADER_MACRO> BuildDefinesFromFlags(uint32_t flags)
{
	std::vector<D3D_SHADER_MACRO> out;

	if (flags & HAS_TEX2)         
		out.push_back({ "HAS_TEX2", "1" });
	if (flags & IS_SKINNED)
		out.push_back({ "IS_SKINNED", "1" });

	out.push_back({ nullptr, nullptr });
	return out;
}

static DXGI_FORMAT GetFloatFormat(UINT mask)
{
	if (mask == 1) return DXGI_FORMAT_R32_FLOAT;
	if (mask <= 3) return DXGI_FORMAT_R32G32_FLOAT;
	if (mask <= 7) return DXGI_FORMAT_R32G32B32_FLOAT;
	if (mask <= 15) return DXGI_FORMAT_R32G32B32A32_FLOAT;
	return DXGI_FORMAT_UNKNOWN;/*s*/
}

static DXGI_FORMAT GetUIntFormat(UINT mask)
{
	if (mask == 1) return DXGI_FORMAT_R32_UINT;
	if (mask <= 3) return DXGI_FORMAT_R32G32_UINT;
	if (mask <= 7) return DXGI_FORMAT_R32G32B32_UINT;
	if (mask <= 15) return DXGI_FORMAT_R32G32B32A32_UINT;
	return DXGI_FORMAT_UNKNOWN;
}

static DXGI_FORMAT GetSIntFormat(UINT mask)
{
	if (mask == 1) return DXGI_FORMAT_R32_SINT;
	if (mask <= 3) return DXGI_FORMAT_R32G32_SINT;
	if (mask <= 7) return DXGI_FORMAT_R32G32B32_SINT;
	if (mask <= 15) return DXGI_FORMAT_R32G32B32A32_SINT;
	return DXGI_FORMAT_UNKNOWN;
}

static DXGI_FORMAT GetDXGIFormat(const D3D11_SIGNATURE_PARAMETER_DESC& desc)
{
	if (strcmp(desc.SemanticName, "COLOR") == 0)
		return DXGI_FORMAT_B8G8R8A8_UNORM;

	switch (desc.ComponentType)
	{
	case D3D_REGISTER_COMPONENT_FLOAT32: return GetFloatFormat(desc.Mask);
	case D3D_REGISTER_COMPONENT_UINT32:  return GetUIntFormat(desc.Mask);
	case D3D_REGISTER_COMPONENT_SINT32:  return GetSIntFormat(desc.Mask);
	default: return DXGI_FORMAT_UNKNOWN;
	}
}

static bool CompileShaderFromFile(const std::string& path, const char* profile, ComPtr<ID3DBlob>& outBlob, uint32_t shaderFlags)
{

	DWORD attr = GetFileAttributesA(path.c_str());
	if (attr == INVALID_FILE_ATTRIBUTES)
		return false;

	UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

	std::vector<D3D_SHADER_MACRO> defines = BuildDefinesFromFlags(shaderFlags);

	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3DCompileFromFile(StringToWString(path).c_str(), shaderFlags ? defines.data() : nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
		profile,
		compileFlags,
		0,
		outBlob.GetAddressOf(),
		errorBlob.GetAddressOf());

	if (FAILED(hr))
		return false;

	if (!outBlob || outBlob->GetBufferSize() == 0)
		return false;

	return true;
}

static bool LoadVertexShader(ID3D11Device* m_device, const std::string& name, const char* profile, ComPtr<ID3D11VertexShader>& vsOut, ComPtr<ID3D11InputLayout>& layoutOut, uint32_t shaderFlags)
{
	if (!m_device)
		return false;

	vsOut.Reset();
	layoutOut.Reset();

	const std::string path = BuildShaderPath(name);
	ComPtr<ID3DBlob> vsBlob;
	HRESULT hr = S_OK;

#ifdef _DEBUG
	if (!CompileShaderFromFile(path, profile, vsBlob, shaderFlags))
		return false;
#else
	hr = D3DReadFileToBlob(StringToWString(path).c_str(), vsBlob.GetAddressOf());
	if (FAILED(hr))
		return false;
#endif

	if (!vsBlob || vsBlob->GetBufferSize() == 0)
		return false;

	ComPtr<ID3D11ShaderReflection> reflection;
	hr = D3DReflect(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), IID_PPV_ARGS(reflection.GetAddressOf()));
	if (FAILED(hr)) return false;

	D3D11_SHADER_DESC shaderDesc{};
	hr = reflection->GetDesc(&shaderDesc);
	if (FAILED(hr)) return false;

	std::vector<std::string> semanticStorage;
	std::vector<D3D11_INPUT_ELEMENT_DESC> layoutDesc;

	semanticStorage.reserve(shaderDesc.InputParameters);
	layoutDesc.reserve(shaderDesc.InputParameters);

	for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc{};
		hr = reflection->GetInputParameterDesc(i, &paramDesc);
		if (FAILED(hr))
			return false;

		semanticStorage.emplace_back(paramDesc.SemanticName);

		D3D11_INPUT_ELEMENT_DESC elem = {};
		elem.SemanticName = semanticStorage.back().c_str();
		elem.SemanticIndex = paramDesc.SemanticIndex;
		elem.Format = GetDXGIFormat(paramDesc);
		elem.InputSlot = 0;
		elem.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elem.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elem.InstanceDataStepRate = 0;

		if (elem.Format == DXGI_FORMAT_UNKNOWN) 
			return false;

		layoutDesc.push_back(elem);
	}

	ComPtr<ID3D11VertexShader> newVS;
	hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, newVS.GetAddressOf());
	if (FAILED(hr)) {
		//OutputDebugStringA(("CreateVertexShader FAILED for: " + name + "\n").c_str());
		return false;
	}

	ComPtr<ID3D11InputLayout> newLayout;
	hr = m_device->CreateInputLayout(layoutDesc.data(), (UINT)layoutDesc.size(),
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), newLayout.GetAddressOf());
	if (FAILED(hr)) return false;

	vsOut = newVS;
	layoutOut = newLayout;
	return true;
}

static bool LoadPixelShader(ID3D11Device* m_device, const std::string& name, const char* profile, ComPtr<ID3D11PixelShader>& psOut, uint32_t shaderFlags)
{
	if (!m_device)
		return false;

	psOut.Reset();

	const std::string path = BuildShaderPath(name);

#ifdef _DEBUG
	ComPtr<ID3DBlob> psBlob;
	if (!CompileShaderFromFile(path, profile, psBlob, shaderFlags))
		return false;
#else
	ComPtr<ID3DBlob> psBlob;
	HRESULT hr = D3DReadFileToBlob(StringToWString(path).c_str(), psBlob.GetAddressOf());
	if (FAILED(hr))
		return false;
#endif

	ComPtr<ID3D11PixelShader> newPS;
	HRESULT hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, newPS.GetAddressOf());
	if (FAILED(hr))
		return false;

	psOut = newPS;
	return true;
}
