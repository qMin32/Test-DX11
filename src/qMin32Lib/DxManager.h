#pragma once
#include "Core.h"
#include <string>

class DxManager
{
public:
	DxManager(ID3D11Device* device, ID3D11DeviceContext* context);

	void SetShader(ED3D11VertexFormat format);

	void CreateConstantBuffer(CBufferPtr& outBuffer, UINT dataSize);
	void SetConstantBuffer(const CBufferPtr& buffer, UINT slotIndex);

	void CreateIndexBuffer(IBufferPtr& outBuffer, const void* data, UINT indexCount, bool dynamic = false, DXGI_FORMAT format = DXGI_FORMAT_R16_UINT);
	void SetIndexBuffer(const IBufferPtr& buffer);

private:
	void CreateShader(ShaderPtr& outShader, const std::string& vsName, const std::string& psName);
	void SetShader(const ShaderPtr& shader);

private:
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;
	ShadersContainerPtr m_shaderContainer;
	friend class ShadersContainer;
};
