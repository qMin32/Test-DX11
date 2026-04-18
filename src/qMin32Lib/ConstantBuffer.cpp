#include "pch.h"
#include "ConstantBuffer.h"

ConstantBuffer::ConstantBuffer(ID3D11Device* device, ID3D11DeviceContext* context, UINT dataSize)
	: m_device(device), m_context(context), m_bufferSize(dataSize)
{
	if (!m_device || !m_context || dataSize == 0)
		return;

    m_buffer.Reset();

    m_bufferSize = (dataSize + 15) & ~15;

    D3D11_BUFFER_DESC desc{};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.ByteWidth = m_bufferSize;

    HRESULT hr = m_device->CreateBuffer(&desc, nullptr, m_buffer.GetAddressOf());
    if (FAILED(hr))
        m_buffer.Reset();
}

bool ConstantBuffer::Update(const void* data, UINT dataSize)
{
    if (!m_buffer || !m_context || !data)
        return false;

    if (dataSize > m_bufferSize)
        return false;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = m_context->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    memcpy(mapped.pData, data, dataSize);
    m_context->Unmap(m_buffer.Get(), 0);
    return true;
}
