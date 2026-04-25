#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#ifndef GRANNY_DX11_MAX_BONES
#define GRANNY_DX11_MAX_BONES 256
#endif

struct SGrannyBonePalette
{
    DirectX::XMFLOAT4X4 Bone[GRANNY_DX11_MAX_BONES];
};

bool GrannySkinning_Initialize(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* shaderDirectory = L"shaders\\debug\\");
void GrannySkinning_Destroy();
bool GrannySkinning_IsReady();
bool GrannySkinning_BindPNT(bool skinned);
bool GrannySkinning_BindPNT2(bool skinned);
bool GrannySkinning_UploadBonePalette(const DirectX::XMFLOAT4X4* bones, unsigned int count);
ID3D11Buffer* GrannySkinning_GetBonePaletteBuffer();
