#pragma once
#include "Core.h"

class ConstantBuffer
{
public:
    ConstantBuffer(ID3D11Device* device, ID3D11DeviceContext* context, UINT dataSize);

    template<class T>
    bool Update(const T& data)
    {
        return Update(&data, sizeof(T));
    }

private:
    bool Update(const void* data, UINT dataSize);

private:
    ComPtr<ID3D11Buffer> m_buffer;
    UINT m_bufferSize = 0;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;

    friend class DxManager; //to not make GetBuffer,GetSize 
};
