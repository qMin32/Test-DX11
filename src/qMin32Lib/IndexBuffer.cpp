#include "pch.h"
#include "IndexBuffer.h"
#include "EterLib/Statemanager.h"

static UINT GetFormatSize(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R16_UINT: return sizeof(WORD);
    case DXGI_FORMAT_R32_UINT: return sizeof(DWORD);
    default: return 0;
    }
}

IndexBuffer::IndexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, const void* data, UINT indexCount, DXGI_FORMAT format, bool dynamic)
    : m_device(device), m_context(context), m_indexCount(indexCount), m_format(format), m_dynamic(dynamic)
{
    if (!m_device || !m_context || !data || !indexCount)
        return;

    m_buffer.Reset();

    UINT stride = GetFormatSize(m_format);
    if (stride == 0)
        return;

    m_bufferSize = stride * m_indexCount;

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = m_bufferSize;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    if (m_dynamic)
    {
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.CPUAccessFlags = 0;
    }

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = data;

    HRESULT hr = m_device->CreateBuffer(&desc, &initData, m_buffer.GetAddressOf());
    if (FAILED(hr))
        m_buffer.Reset();
}

void IndexBuffer::SetIndexBuffer()
{
    if (!m_buffer || !m_context)
        return;

	STATEMANAGER.SetIndices(m_buffer.Get(), 0);
}

bool IndexBuffer::Update(const void* data, UINT dataSize)
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