#include "pch.h"
#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, const void* data, UINT vertexCount, UINT vertexStride, bool dynamic)
    : m_device(device), m_context(context), m_vertexCount(vertexCount), m_vertexStride(vertexStride), m_dynamic(dynamic)
{
    if (!m_device || !m_context || !data || !vertexCount || !vertexStride)
        return;

    m_bufferSize = m_vertexCount * m_vertexStride;

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = m_bufferSize;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    if (m_dynamic)
    {
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
    }

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = data;

    HRESULT hr = m_device->CreateBuffer(&desc, &initData, m_buffer.GetAddressOf());
    if (FAILED(hr))
        m_buffer.Reset();
}

bool VertexBuffer::Update(const void* data, UINT dataSize)
{
    if (!m_dynamic)
        return false;

    if (!m_buffer || !m_context || !data)
        return false;

    if (dataSize > m_bufferSize)
        return false;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = m_context->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr))
        return false;

    memcpy(mapped.pData, data, dataSize);
    m_context->Unmap(m_buffer.Get(), 0);
    return true;
}
