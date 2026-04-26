#pragma once
#include "Core.h"
#include "EterLib/D3D9Compat.h"
#include "EterLib/D3DXMathCompat.h"

struct CBPerFrame
{
	D3DXMATRIX matWorld;
	D3DXMATRIX matView;
	D3DXMATRIX matProj;
};

// b1: Material / texture stage state emulation
struct CBMaterial
{
	float textureFactor[4];

	int useTexture0;
	int useTexture1;
	int colorOp0;
	int alphaOp0;

	int colorOp1;
	int alphaOp1;
	int colorArg10;
	int colorArg20;

	int alphaArg10;
	int alphaArg20;
	int colorArg11;
	int colorArg21;

	int alphaArg11;
	int alphaArg21;
	int alphaTestEnable;
	int alphaRef;

	int texCoordGen1;   // 0=vertex UV, 1=camera-space position, 2=camera-space reflection
	int padMat1;
	int padMat2;
	int padMat3;
};

// b2: Lighting emulation
struct CBLighting
{
	float lightDir[4];
	float lightDiffuse[4];
	float lightAmbient[4];
	float matDiffuse[4];
	float matAmbient[4];
	float matEmissive[4];
	int lightingEnable;
	int pad0, pad1, pad2;
};

// b3: Texture coordinate transform
struct CBTexTransform
{
	D3DXMATRIX matTexTransform0;
	D3DXMATRIX matTexTransform1;
};

// b4: Fog
struct CBFog
{
	float fogColor[4];
	float fogStart;
	float fogEnd;
	int fogEnable;
	int fogPad;
};

// b5: Screen dimensions (for XYZRHW conversion)
struct CBScreenSize
{
	float screenWidth;
	float screenHeight;
	float pad0;
	float pad1;
};

#ifndef GRANNY_DX11_MAX_BONES
#define GRANNY_DX11_MAX_BONES 256
#endif

struct SGrannyBonePalette
{
	DirectX::XMFLOAT4X4 Bone[GRANNY_DX11_MAX_BONES];
};

class CBManager
{
public:
	CBManager(DxManager* manager);

	// Transforms → constant buffer b0
	void SetWorldMatrix(const D3DXMATRIX& mat);
	void SetViewMatrix(const D3DXMATRIX& mat);
	void SetProjMatrix(const D3DXMATRIX& mat);
	void FlushTransforms();
	// Texture coordinate transform → constant buffer b3
	void SetTexTransform(DWORD dwStage, const D3DXMATRIX& mat);

	// Lighting → constant buffer b2
	void SetLightingEnable(BOOL bEnable);
	void SetLight(DWORD index, const D3DLIGHT9* pLight);
	void SetMaterial(const D3DMATERIAL9* pMaterial);
	void SetAmbient(DWORD dwColor);
	void FlushLighting();

	// Fog → constant buffer b4
	void SetFogEnable(BOOL bEnable);
	void SetFogColor(DWORD dwColor);
	void SetFogStart(float fStart);
	void SetFogEnd(float fEnd);
	void FlushFog();

	// Texture stage states → constant buffer b1
	void SetTextureFactor(DWORD dwFactor);
	void SetTextureStageOp(DWORD dwStage, int colorOp, int alphaOp);
	void SetTextureStageArgs(DWORD dwStage, int colorArg1, int colorArg2, int alphaArg1, int alphaArg2);
	void SetTexCoordGen(DWORD dwStage, int mode);  // 0=vertex UV, 1=cam pos, 2=cam reflection
	void FlushMaterial();

	void SetScreenSize(float width, float height);

	//bone mesh
	bool UploadBonePalette(const DirectX::XMFLOAT4X4* bones, unsigned int count);

private:
	CBufferPtr				m_pCBPerFrame;
	CBufferPtr				m_pCBMaterial;
	CBufferPtr				m_pCBLighting;
	CBufferPtr				m_pCBTexTransform;
	CBufferPtr				m_pCBFog;
	CBufferPtr				m_pCBScreenSize;
	CBufferPtr				m_pCBBonePalette;

	CBPerFrame				m_cbPerFrame = {};
	CBMaterial				m_cbMaterial = {};
	CBLighting				m_cbLighting = {};
	CBTexTransform			m_cbTexTransform = {};
	CBFog					m_cbFog = {};
	CBScreenSize			m_cbScreenSize = {};

	bool					m_bTransformDirty = true;
	bool					m_bMaterialDirty = true;
	bool					m_bLightingDirty = true;
	bool					m_bFogDirty = true;



	friend class CD3D11Renderer;
};
