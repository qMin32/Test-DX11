#include "StdAfx.h"
#include "GrannySkinning.h"
#include "Eterbase/Debug.h"

#pragma comment(lib, "d3dcompiler.lib")

static ID3D11Device* gs_device = nullptr;
static ID3D11DeviceContext* gs_context = nullptr;
static ID3D11Buffer* gs_bonePaletteCB = nullptr;
static ID3D11VertexShader* gs_vsSkinnedPNT = nullptr;
static ID3D11VertexShader* gs_vsSkinnedPNT2 = nullptr;
static ID3D11PixelShader* gs_psPNT = nullptr;
static ID3D11PixelShader* gs_psPNT2 = nullptr;
static ID3D11InputLayout* gs_layoutSkinnedPNT = nullptr;
static ID3D11InputLayout* gs_layoutSkinnedPNT2 = nullptr;

static void SafeReleaseIUnknown(IUnknown*& p)
{
    if (p)
    {
        p->Release();
        p = nullptr;
    }
}

static bool CompileShader(const wchar_t* fileName, const char* entry, const char* target, ID3DBlob** blob)
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ID3DBlob* errors = nullptr;
    HRESULT hr = D3DCompileFromFile(fileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, target, flags, 0, blob, &errors);
    if (FAILED(hr))
    {
        if (errors)
        {
            TraceError("D3DCompileFromFile failed: %S: %s", fileName, (const char*)errors->GetBufferPointer());
            errors->Release();
        }
        return false;
    }
    if (errors)
        errors->Release();
    return true;
}

static bool CreateDynamicConstantBuffer(UINT byteWidth, ID3D11Buffer** buffer)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = byteWidth;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    return SUCCEEDED(gs_device->CreateBuffer(&desc, nullptr, buffer));
}

static bool LoadVertexShader(const wchar_t* fileName, ID3D11VertexShader** vs, ID3D11InputLayout** layout, const D3D11_INPUT_ELEMENT_DESC* elems, UINT elemCount)
{
    ID3DBlob* blob = nullptr;
    if (!CompileShader(fileName, "main", "vs_5_0", &blob))
        return false;
    HRESULT hr = gs_device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, vs);
    if (SUCCEEDED(hr))
        hr = gs_device->CreateInputLayout(elems, elemCount, blob->GetBufferPointer(), blob->GetBufferSize(), layout);
    blob->Release();
    return SUCCEEDED(hr);
}

static bool LoadPixelShader(const wchar_t* fileName, ID3D11PixelShader** ps)
{
    ID3DBlob* blob = nullptr;
    if (!CompileShader(fileName, "main", "ps_5_0", &blob))
        return false;
    HRESULT hr = gs_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, ps);
    blob->Release();
    return SUCCEEDED(hr);
}

bool GrannySkinning_Initialize(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* shaderDirectory)
{
    GrannySkinning_Destroy();
    if (!device || !context || !shaderDirectory)
        return false;
    gs_device = device;
    gs_context = context;
    gs_device->AddRef();
    gs_context->AddRef();
    if (!CreateDynamicConstantBuffer(sizeof(SGrannyBonePalette), &gs_bonePaletteCB))
        return false;
    wchar_t pathPNTVS[MAX_PATH];
    wchar_t pathPNT2VS[MAX_PATH];
    wchar_t pathPNTPs[MAX_PATH];
    wchar_t pathPNT2Ps[MAX_PATH];
    swprintf_s(pathPNTVS, L"%s\\PNT_Skinned_Vs.hlsl", shaderDirectory);
    swprintf_s(pathPNT2VS, L"%s\\PNT2_Skinned_Vs.hlsl", shaderDirectory);
    swprintf_s(pathPNTPs, L"%s\\PNT_Ps.hlsl", shaderDirectory);
    swprintf_s(pathPNT2Ps, L"%s\\PNT2_Ps.hlsl", shaderDirectory);
    const D3D11_INPUT_ELEMENT_DESC pntElems[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    const D3D11_INPUT_ELEMENT_DESC pnt2Elems[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    if (!LoadVertexShader(pathPNTVS, &gs_vsSkinnedPNT, &gs_layoutSkinnedPNT, pntElems, _countof(pntElems)))
        return false;
    if (!LoadVertexShader(pathPNT2VS, &gs_vsSkinnedPNT2, &gs_layoutSkinnedPNT2, pnt2Elems, _countof(pnt2Elems)))
        return false;
    if (!LoadPixelShader(pathPNTPs, &gs_psPNT))
        return false;
    if (!LoadPixelShader(pathPNT2Ps, &gs_psPNT2))
        return false;
    return true;
}

void GrannySkinning_Destroy()
{
    SafeReleaseIUnknown((IUnknown*&)gs_layoutSkinnedPNT);
    SafeReleaseIUnknown((IUnknown*&)gs_layoutSkinnedPNT2);
    SafeReleaseIUnknown((IUnknown*&)gs_vsSkinnedPNT);
    SafeReleaseIUnknown((IUnknown*&)gs_vsSkinnedPNT2);
    SafeReleaseIUnknown((IUnknown*&)gs_psPNT);
    SafeReleaseIUnknown((IUnknown*&)gs_psPNT2);
    SafeReleaseIUnknown((IUnknown*&)gs_bonePaletteCB);
    SafeReleaseIUnknown((IUnknown*&)gs_context);
    SafeReleaseIUnknown((IUnknown*&)gs_device);
}

bool GrannySkinning_IsReady()
{
    return gs_device && gs_context && gs_bonePaletteCB && gs_vsSkinnedPNT && gs_vsSkinnedPNT2;
}

bool GrannySkinning_BindPNT(bool skinned)
{
    if (!skinned || !GrannySkinning_IsReady())
        return false;
    gs_context->IASetInputLayout(gs_layoutSkinnedPNT);
    gs_context->VSSetShader(gs_vsSkinnedPNT, nullptr, 0);
    gs_context->PSSetShader(gs_psPNT, nullptr, 0);
    gs_context->VSSetConstantBuffers(8, 1, &gs_bonePaletteCB);
    return true;
}

bool GrannySkinning_BindPNT2(bool skinned)
{
    if (!skinned || !GrannySkinning_IsReady())
        return false;
    gs_context->IASetInputLayout(gs_layoutSkinnedPNT2);
    gs_context->VSSetShader(gs_vsSkinnedPNT2, nullptr, 0);
    gs_context->PSSetShader(gs_psPNT2, nullptr, 0);
    gs_context->VSSetConstantBuffers(8, 1, &gs_bonePaletteCB);
    return true;
}

bool GrannySkinning_UploadBonePalette(const DirectX::XMFLOAT4X4* bones, unsigned int count)
{
    if (!GrannySkinning_IsReady() || !bones)
        return false;
    if (count > GRANNY_DX11_MAX_BONES)
        count = GRANNY_DX11_MAX_BONES;
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = gs_context->Map(gs_bonePaletteCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr))
        return false;
    SGrannyBonePalette* dst = reinterpret_cast<SGrannyBonePalette*>(mapped.pData);
    memset(dst, 0, sizeof(SGrannyBonePalette));
    memcpy(dst->Bone, bones, sizeof(DirectX::XMFLOAT4X4) * count);
    gs_context->Unmap(gs_bonePaletteCB, 0);
    return true;
}

ID3D11Buffer* GrannySkinning_GetBonePaletteBuffer()
{
    return gs_bonePaletteCB;
}
