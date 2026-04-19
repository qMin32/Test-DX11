#include "pch.h"
#include "VertexBuffer.h"
#include <EterBase/Debug.h>

VertexBuffer::VertexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, const void* data, UINT vertexCount, UINT vertexStride, bool dynamic)
    : m_device(device), m_context(context), m_vertexCount(vertexCount), m_vertexStride(vertexStride), m_dynamic(dynamic)
{
    if (!device || !context || !vertexCount || !vertexStride)
        return;

    if(m_buffer.Get() != nullptr)
		m_buffer.Reset();

    m_bufferSize = vertexCount * vertexStride;

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = m_bufferSize;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

    D3D11_SUBRESOURCE_DATA init{};
    D3D11_SUBRESOURCE_DATA* pInit = nullptr;

    if (data)
    {
        init.pSysMem = data;
        pInit = &init;
    }

    if (FAILED(device->CreateBuffer(&desc, pInit, m_buffer.GetAddressOf())))
    {
        m_buffer.Reset();
    }
}

bool VertexBuffer::Update(const void* data) 
{
    if (!data || !m_dynamic || !m_buffer)
        return false;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = m_context->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) 
    {
        TraceError("Map FAILED: 0x%X", hr);
        return false;
    }

    if (!mapped.pData)
    { 
        m_context->Unmap(m_buffer.Get(), 0);
        return false;
    }

    memcpy(mapped.pData, data, m_bufferSize);
    m_context->Unmap(m_buffer.Get(), 0);
    return true;
}

bool VertexBuffer::Update(const void* data, UINT vertexCount)
{
    if (!data || !m_dynamic || !m_buffer)
        return false;

    UINT newSize = vertexCount * m_vertexStride;

    if (newSize == 0)
        newSize = m_bufferSize;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = m_context->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr))
    {
        TraceError("Map FAILED: 0x%X", hr);
        return false;
    }

    if (!mapped.pData)
    {
        m_context->Unmap(m_buffer.Get(), 0);
        return false;
    }

    memcpy(mapped.pData, data, newSize);
    m_context->Unmap(m_buffer.Get(), 0);
    return true;
}
