#pragma once
#include "Core.h"
#include <vector>

class VertexBuffer
{
public:
    VertexBuffer() = default;
    VertexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, const void* data, UINT vertexCount, UINT vertexStride, bool dynamic = false);

	UINT GetVertexCount() const { return m_vertexCount; }

    bool Update(const void* data, UINT dataSize);

    template <class T> bool Update(const std::vector<T>& data)
    {
        if (data.empty())
            return false;

        if (data.size() != m_vertexCount)
            return false;

        return Update(data.data(), static_cast<UINT>(data.size() * sizeof(T)));
    }

private:
    ComPtr<ID3D11Buffer> m_buffer;
    UINT m_bufferSize = 0;
    UINT m_vertexCount = 0;
    UINT m_vertexStride = 0;
    bool m_dynamic = false;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
	friend class DxManager;
};