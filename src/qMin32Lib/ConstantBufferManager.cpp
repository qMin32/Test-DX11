#include "pch.h"
#include "DxManager.h"
#include "ConstantBuffer.h"
#include "ConstantBufferManager.h"

CBManager::CBManager(DxManager* manager)
{
	manager->CreateConstantBuffer(m_pCBPerFrame, sizeof(CBPerFrame));
	manager->CreateConstantBuffer(m_pCBMaterial, sizeof(CBMaterial));
	manager->CreateConstantBuffer(m_pCBLighting, sizeof(CBLighting));
	manager->CreateConstantBuffer(m_pCBTexTransform, sizeof(CBTexTransform));
	manager->CreateConstantBuffer(m_pCBFog, sizeof(CBFog));
	manager->CreateConstantBuffer(m_pCBScreenSize, sizeof(CBScreenSize));
	manager->CreateConstantBuffer(m_pCBBonePalette, sizeof(SGrannyBonePalette)); // b6
	manager->CreateConstantBuffer(m_pCBSpeedTree, sizeof(CBSpeedTree)); // b7
}

void CBManager::SetWorldMatrix(const D3DXMATRIX& mat)
{
	m_cbPerFrame.matWorld = mat;
	m_bTransformDirty = true;
}

void CBManager::SetViewMatrix(const D3DXMATRIX& mat)
{
	m_cbPerFrame.matView = mat;
	m_bTransformDirty = true;
}

void CBManager::SetProjMatrix(const D3DXMATRIX& mat)
{
	m_cbPerFrame.matProj = mat;
	m_bTransformDirty = true;
}

void CBManager::FlushTransforms()
{
	if (!m_bTransformDirty)
		return;

	m_pCBPerFrame->Update(m_cbPerFrame);

	m_bTransformDirty = false;
}

void CBManager::SetTexTransform(DWORD dwStage, const D3DXMATRIX& mat)
{
	if (dwStage == 0)
		m_cbTexTransform.matTexTransform0 = mat;
	else if (dwStage == 1)
		m_cbTexTransform.matTexTransform1 = mat;
	else
		return;

	m_pCBTexTransform->Update(m_cbTexTransform);
}



void CBManager::SetLightingEnable(BOOL bEnable)
{
	m_cbLighting.lightingEnable = bEnable ? 1 : 0;
	m_bLightingDirty = true;
}

void CBManager::SetLight(DWORD index, const D3DLIGHT9* pLight)
{
	if (index > 0 || !pLight) return; // Only support light 0 for now

	m_cbLighting.lightDir[0] = pLight->Direction.x;
	m_cbLighting.lightDir[1] = pLight->Direction.y;
	m_cbLighting.lightDir[2] = pLight->Direction.z;
	m_cbLighting.lightDir[3] = 0.0f;

	m_cbLighting.lightDiffuse[0] = pLight->Diffuse.r;
	m_cbLighting.lightDiffuse[1] = pLight->Diffuse.g;
	m_cbLighting.lightDiffuse[2] = pLight->Diffuse.b;
	m_cbLighting.lightDiffuse[3] = pLight->Diffuse.a;

	m_bLightingDirty = true;
}

void CBManager::SetMaterial(const D3DMATERIAL9* pMaterial)
{
	m_cbLighting.matDiffuse[0] = pMaterial->Diffuse.r;
	m_cbLighting.matDiffuse[1] = pMaterial->Diffuse.g;
	m_cbLighting.matDiffuse[2] = pMaterial->Diffuse.b;
	m_cbLighting.matDiffuse[3] = pMaterial->Diffuse.a;

	m_cbLighting.matAmbient[0] = pMaterial->Ambient.r;
	m_cbLighting.matAmbient[1] = pMaterial->Ambient.g;
	m_cbLighting.matAmbient[2] = pMaterial->Ambient.b;
	m_cbLighting.matAmbient[3] = pMaterial->Ambient.a;

	m_cbLighting.matEmissive[0] = pMaterial->Emissive.r;
	m_cbLighting.matEmissive[1] = pMaterial->Emissive.g;
	m_cbLighting.matEmissive[2] = pMaterial->Emissive.b;
	m_cbLighting.matEmissive[3] = pMaterial->Emissive.a;

	m_bLightingDirty = true;
}

void CBManager::SetAmbient(DWORD dwColor)
{
	m_cbLighting.lightAmbient[0] = ((dwColor >> 16) & 0xFF) / 255.0f;
	m_cbLighting.lightAmbient[1] = ((dwColor >> 8) & 0xFF) / 255.0f;
	m_cbLighting.lightAmbient[2] = (dwColor & 0xFF) / 255.0f;
	m_cbLighting.lightAmbient[3] = ((dwColor >> 24) & 0xFF) / 255.0f;
	m_bLightingDirty = true;
}

void CBManager::FlushLighting()
{
	if (!m_bLightingDirty) return;

	m_pCBLighting->Update(m_cbLighting);

	m_bLightingDirty = false;
}

void CBManager::SetFogEnable(BOOL bEnable)
{
	m_cbFog.fogEnable = bEnable ? 1 : 0;
	m_bFogDirty = true;
}
void CBManager::SetFogColor(DWORD dwColor)
{
	m_cbFog.fogColor[0] = ((dwColor >> 16) & 0xFF) / 255.0f;	// R
	m_cbFog.fogColor[1] = ((dwColor >> 8) & 0xFF) / 255.0f;	// G
	m_cbFog.fogColor[2] = (dwColor & 0xFF) / 255.0f;	// B
	m_cbFog.fogColor[3] = 1.0f;
	m_bFogDirty = true;
}
void CBManager::SetFogStart(float fStart)
{
	m_cbFog.fogStart = fStart;
	m_bFogDirty = true;
}
void CBManager::SetFogEnd(float fEnd)
{
	m_cbFog.fogEnd = fEnd;
	m_bFogDirty = true;
}

void CBManager::FlushFog()
{
	if (!m_bFogDirty) return;
	m_pCBFog->Update(m_cbFog);

	m_bFogDirty = false;
}

void CBManager::SetTextureFactor(DWORD dwFactor)
{
	// D3D9 DWORD color format is 0xAARRGGBB
	m_cbMaterial.textureFactor[0] = ((dwFactor >> 16) & 0xFF) / 255.0f;	// R
	m_cbMaterial.textureFactor[1] = ((dwFactor >> 8) & 0xFF) / 255.0f;	// G
	m_cbMaterial.textureFactor[2] = (dwFactor & 0xFF) / 255.0f;	// B
	m_cbMaterial.textureFactor[3] = ((dwFactor >> 24) & 0xFF) / 255.0f;	// A
	m_bMaterialDirty = true;
}

void CBManager::SetTextureStageOp(DWORD dwStage, int colorOp, int alphaOp)
{
	if (dwStage == 0) { m_cbMaterial.colorOp0 = colorOp; m_cbMaterial.alphaOp0 = alphaOp; }
	else if (dwStage == 1) { m_cbMaterial.colorOp1 = colorOp; m_cbMaterial.alphaOp1 = alphaOp; }
	m_bMaterialDirty = true;
}

void CBManager::SetTextureStageArgs(DWORD dwStage, int colorArg1, int colorArg2, int alphaArg1, int alphaArg2)
{
	if (dwStage == 0)
	{
		m_cbMaterial.colorArg10 = colorArg1;
		m_cbMaterial.colorArg20 = colorArg2;
		m_cbMaterial.alphaArg10 = alphaArg1;
		m_cbMaterial.alphaArg20 = alphaArg2;
	}
	else if (dwStage == 1)
	{
		m_cbMaterial.colorArg11 = colorArg1;
		m_cbMaterial.colorArg21 = colorArg2;
		m_cbMaterial.alphaArg11 = alphaArg1;
		m_cbMaterial.alphaArg21 = alphaArg2;
	}
	m_bMaterialDirty = true;
}

void CBManager::SetTexCoordGen(DWORD dwStage, int mode)
{
	if (dwStage == 1)
	{
		m_cbMaterial.texCoordGen1 = mode;
		m_bMaterialDirty = true;
	}
}

void CBManager::FlushMaterial()
{
	if (!m_bMaterialDirty) return;

	m_pCBMaterial->Update(m_cbMaterial);

	m_bMaterialDirty = false;
}

void CBManager::SetScreenSize(float width, float height)
{
	m_cbScreenSize.screenWidth = width;
	m_cbScreenSize.screenHeight = height;

	m_pCBScreenSize->Update(m_cbScreenSize);
}

bool CBManager::UploadBonePalette(const DirectX::XMFLOAT4X4* bones, unsigned int count)
{
	if (!bones || !m_pCBBonePalette)
		return false;

	if (count > GRANNY_DX11_MAX_BONES)
		count = GRANNY_DX11_MAX_BONES;

	SGrannyBonePalette palette = {};
	memcpy(palette.Bone, bones, sizeof(DirectX::XMFLOAT4X4) * count);

	if (!m_pCBBonePalette->Update(palette))
		return false;

	return true;
}


void CBManager::SetSpeedTreeCompoundMatrix(const D3DXMATRIX& mat)
{
	m_cbSpeedTree.matCompound = mat;
	m_bSpeedTreeDirty = true;
}

void CBManager::SetSpeedTreeTreePosition(const D3DXVECTOR4& pos)
{
	m_cbSpeedTree.treePos[0] = pos.x;
	m_cbSpeedTree.treePos[1] = pos.y;
	m_cbSpeedTree.treePos[2] = pos.z;
	m_cbSpeedTree.treePos[3] = pos.w;
	m_bSpeedTreeDirty = true;
}

void CBManager::SetSpeedTreeFog(const float* fog)
{
	if (!fog)
		return;
	memcpy(m_cbSpeedTree.fog, fog, sizeof(float) * 4);
	m_bSpeedTreeDirty = true;
}

void CBManager::SetSpeedTreeLight(const float* light)
{
	if (!light)
		return;
	memcpy(m_cbSpeedTree.lightDir, light + 0, sizeof(float) * 4);
	memcpy(m_cbSpeedTree.lightAmbient, light + 4, sizeof(float) * 4);
	memcpy(m_cbSpeedTree.lightDiffuse, light + 8, sizeof(float) * 4);
	m_bSpeedTreeDirty = true;
}

void CBManager::SetSpeedTreeMaterialConstants(const float* material, float leafLightingAdjustment)
{
	if (!material)
		return;
	m_cbSpeedTree.materialDiffuse[0] = material[0];
	m_cbSpeedTree.materialDiffuse[1] = material[1];
	m_cbSpeedTree.materialDiffuse[2] = material[2];
	m_cbSpeedTree.materialDiffuse[3] = 1.0f;
	m_cbSpeedTree.materialAmbient[0] = material[3];
	m_cbSpeedTree.materialAmbient[1] = material[4];
	m_cbSpeedTree.materialAmbient[2] = material[5];
	m_cbSpeedTree.materialAmbient[3] = 1.0f;
	m_cbSpeedTree.leafLightingAdjustment[0] = leafLightingAdjustment;
	m_cbSpeedTree.leafLightingAdjustment[1] = 0.0f;
	m_cbSpeedTree.leafLightingAdjustment[2] = 0.0f;
	m_cbSpeedTree.leafLightingAdjustment[3] = 0.0f;
	m_bSpeedTreeDirty = true;
}

void CBManager::SetSpeedTreeLeafTables(const float* table, UINT float4Count)
{
	if (!table || float4Count == 0)
		return;
	if (float4Count > SPEEDTREE_MAX_LEAF_TABLE_FLOAT4)
		float4Count = SPEEDTREE_MAX_LEAF_TABLE_FLOAT4;
	memcpy(m_cbSpeedTree.leafTable, table, float4Count * sizeof(float) * 4);
	m_bSpeedTreeDirty = true;
}

void CBManager::FlushSpeedTree()
{
	if (!m_bSpeedTreeDirty)
		return;
	m_pCBSpeedTree->Update(m_cbSpeedTree);
	m_bSpeedTreeDirty = false;
}
