#include "pch.h"
#include "DxManager.h"
#include "ConstantBuffer.h"
#include "IndexBuffer.h"
#include "Shaders.h"
#include "ShaderContainer.h"

#include "EterBase/Debug.h"

DxManager::DxManager(ID3D11Device* device, ID3D11DeviceContext* context)
	: m_device(device), m_context(context)
{
	m_shaderContainer = std::make_shared<ShadersContainer>(this);
}

void DxManager::SetShader(ED3D11VertexFormat format)
{
	if (format >= VF_COUNT || !m_shaderContainer)
		return;

	SetShader(m_shaderContainer->GetShader(format));
}

void DxManager::CreateConstantBuffer(CBufferPtr& outBuffer, UINT dataSize)
{
	outBuffer = std::make_shared<ConstantBuffer>(m_device, m_context, dataSize);
}

void DxManager::SetConstantBuffer(const CBufferPtr& buffer, UINT slotIndex)
{
	if (buffer)
	{
		m_context->VSSetConstantBuffers(slotIndex, 1, buffer->m_buffer.GetAddressOf());
		m_context->PSSetConstantBuffers(slotIndex, 1, buffer->m_buffer.GetAddressOf());
	}
	else
	{
		m_context->VSSetConstantBuffers(slotIndex, 1, nullptr);
		m_context->PSSetConstantBuffers(slotIndex, 1, nullptr);
	}
}

void DxManager::CreateIndexBuffer(IBufferPtr& outBuffer, const void* data, UINT indexCount, bool dynamic, DXGI_FORMAT format)
{
	outBuffer = std::make_shared<IndexBuffer>(m_device, m_context, data, indexCount, format, dynamic);
}

void DxManager::SetIndexBuffer(const IBufferPtr& buffer)
{
	m_context->IASetIndexBuffer(buffer ? buffer->m_buffer.Get() : nullptr, buffer ? buffer->m_format : DXGI_FORMAT_R16_UINT, 0);
}

void DxManager::CreateShader(ShaderPtr& outShader, const std::string& vsName, const std::string& psName)
{
	outShader = std::make_shared<CShaders>(m_device, vsName, psName);
}

void DxManager::SetShader(const ShaderPtr& shader)
{
	if (!shader)
	{
		m_context->IASetInputLayout(nullptr);
		m_context->VSSetShader(nullptr, nullptr, 0);
		m_context->PSSetShader(nullptr, nullptr, 0);
		return;
	}

	shader->Set(m_context);
}
