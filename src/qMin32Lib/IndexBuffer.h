#pragma once
#include "Core.h"

class IndexBuffer
{
public:
    IndexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, const void* data, UINT indexCount, DXGI_FORMAT format, bool dynamic = false);

    void SetIndexBuffer();
    bool Update(const void* data, UINT dataSize);

private:
    ComPtr<ID3D11Buffer> m_buffer;
    UINT m_bufferSize = 0;
    UINT m_indexCount = 0;
    bool m_dynamic = false;
    DXGI_FORMAT m_format = DXGI_FORMAT_R32_UINT;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    friend class DxManager;
};