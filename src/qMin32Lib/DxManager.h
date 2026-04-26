#pragma once
#include "Core.h"
#include <string>
#include "ShadersHelper.h"

class DxManager
{
public:
	DxManager(ID3D11Device* device, ID3D11DeviceContext* context);

	CBManagerPtr GetCbMgr();
	ID3D11Device* GetDevice() { return m_device; }
	ID3D11DeviceContext* GetDeviceContext() { return m_context; }

	void SetShader(ED3D11VertexFormat format, uint32_t flags = SHADER_NONE);


	void CreateConstantBuffer(CBufferPtr& outBuffer, UINT dataSize);
	void SetConstantBuffer(const CBufferPtr& buffer, UINT slotIndex);

	void CreateIndexBuffer(IBufferPtr& outBuffer, const void* data, UINT indexCount, bool dynamic = false, DXGI_FORMAT format = DXGI_FORMAT_R16_UINT);
	void SetIndexBuffer(const IBufferPtr& buffer);

	void CreateVertexBuffer(VBufferPtr& outBuffer, const void* data, UINT vertexCount, UINT vertexStride, bool dynamic = false);
	void SetVertexBuffer(const VBufferPtr& buffer, UINT stride = 0);

private:
	void CreateShader(ShaderPtr& outShader, const std::string& vsName, const std::string& psName, uint32_t shaderFlags = SHADER_NONE);
	void SetShader(const ShaderPtr& shader);

private:
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;
	ShadersContainerPtr m_shaderContainer;
	CBManagerPtr m_cbManager;
	friend class ShadersContainer;
};
