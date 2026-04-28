#include "StdAfx.h"
#include "Eterlib/StateManager.h"
#include "ModelInstance.h"
#include "Model.h"
#include "qMin32Lib/All.h"
#include <qMin32Lib/ConstantBufferManager.h>
#include <algorithm>

#ifdef _TEST

#include "Eterlib/GrpScreen.h"

void Granny_RenderBoxBones(const granny_skeleton* pkGrnSkeleton, const granny_world_pose* pkGrnWorldPose, const D3DXMATRIX& matBase)
{
	D3DXMATRIX matWorld;
	CScreen screen;	
	for (int iBone = 0; iBone != pkGrnSkeleton->BoneCount; ++iBone)
	{
		const granny_bone& rkGrnBone = pkGrnSkeleton->Bones[iBone];				
		const D3DXMATRIX* c_matBone=(const D3DXMATRIX*)GrannyGetWorldPose4x4(pkGrnWorldPose, iBone);
		
		D3DXMatrixMultiply(&matWorld, c_matBone, &matBase);
		
		STATEMANAGER.SetTransform(World, &matWorld);
		screen.RenderBox3d(-5.0f, -5.0f, -5.0f, 5.0f, 5.0f, 5.0f);
	}
}

#endif


void CGrannyModelInstance::DeformNoSkin(const D3DXMATRIX * c_pWorldMatrix)
{
	if (IsEmpty())
		return;

	UpdateWorldPose();
	UpdateWorldMatrices(c_pWorldMatrix);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Render
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// With One Texture
void CGrannyModelInstance::RenderWithOneTexture()
{
	if (IsEmpty())
		return;

#ifdef _TEST
	Granny_RenderBoxBones(GrannyGetSourceSkeleton(m_pgrnModelInstance), m_pgrnWorldPose, TEST_matWorld);
	if (GetAsyncKeyState('P'))
		Tracef("render %x", m_pgrnModelInstance);
	return;
#endif

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();

	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, IS_SKINNED);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNTVertex));
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
}

void CGrannyModelInstance::BlendRenderWithOneTexture()
{
	if (IsEmpty())
		return;

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();

	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, IS_SKINNED);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNTVertex));
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

void CGrannyModelInstance::RenderWithTwoTexture()
{
	if (IsEmpty())
		return;

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();

	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, HAS_TEX2 | IS_SKINNED);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH, HAS_TEX2);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNT2Vertex));
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
}

void CGrannyModelInstance::BlendRenderWithTwoTexture()
{
	if (IsEmpty())
		return;

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();

	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, HAS_TEX2 | IS_SKINNED);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH, HAS_TEX2);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNT2Vertex));
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

void CGrannyModelInstance::RenderWithoutTexture()
{
	if (IsEmpty())
		return;

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);

	auto skinnedVB = m_pModel->GetSkinnedVertexBuffer();
	auto rigidVB = m_pModel->GetVertexBuffer();

	if (skinnedVB)
	{
		_mgr->SetShader(VF_MESH, IS_SKINNED);
		_mgr->SetVertexBuffer(skinnedVB, m_pModel->GetSkinnedVertexStride());
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	if (rigidVB)
	{
		_mgr->SetShader(VF_MESH);
		_mgr->SetVertexBuffer(rigidVB, sizeof(TPNTVertex));
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

bool CGrannyModelInstance::UploadMeshBonePaletteToShader(int iMesh)
{
	if (!m_pModel || !__GetWorldPosePtr())
		return false;
	if (iMesh < 0 || iMesh >= (int)m_vct_pgrnMeshBinding.size())
		return false;

	int* boneIndices = __GetMeshBoneIndices((unsigned int)iMesh);
	if (!boneIndices)
		return false;

	const CGrannyMesh* pMesh = m_pModel->GetMeshPointer(iMesh);
	if (!pMesh || !pMesh->GetGrannyMeshPointer())
		return false;

	const int meshBoneCount = pMesh->GetGrannyMeshPointer()->BoneBindingCount;
	const D3DXMATRIX* composite = (const D3DXMATRIX*)GrannyGetWorldPoseComposite4x4Array(__GetWorldPosePtr());
	DirectX::XMFLOAT4X4 palette[GRANNY_DX11_MAX_BONES];
	const int count = std::min(meshBoneCount, GRANNY_DX11_MAX_BONES);

	for (int i = 0; i < count; ++i)
	{
		D3DXMATRIX m = composite[boneIndices[i]];
		memcpy(&palette[i], &m, sizeof(DirectX::XMFLOAT4X4));
	}

	return _mgr->GetCbMgr()->UploadBonePalette(palette, count);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Render Mesh List
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// With One Texture
void CGrannyModelInstance::RenderMeshNodeListWithOneTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	assert(m_pModel != NULL);

	auto lpd3dIdxBuf = m_pModel->GetIndexBuffer();
	assert(lpd3dIdxBuf != NULL);

	const CGrannyModel::TMeshNode * pMeshNode = m_pModel->GetMeshNodeList(eMeshType, eMtrlType);

	while (pMeshNode)
	{
		const CGrannyMesh * pMesh = pMeshNode->pMesh;
		int vtxMeshBasePos = pMesh->GetVertexBasePosition();

		_mgr->SetIndexBuffer(lpd3dIdxBuf);
		STATEMANAGER.SetTransform(World, &m_meshMatrices[pMeshNode->iMesh]);
		if (eMeshType == CGrannyMesh::TYPE_DEFORM)
			UploadMeshBonePaletteToShader(pMeshNode->iMesh);

		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList(eMtrlType);
		int vtxCount = pMesh->GetVertexCount();

		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;

			CGrannyMaterial& rkMtrl = m_kMtrlPal.GetMaterialRef(pTriGroupNode->mtrlIndex);

			if (!material_data_.pImage)
			{
				if (std::fabs(rkMtrl.GetSpecularPower() - material_data_.fSpecularPower) >= std::numeric_limits<float>::epsilon())
					rkMtrl.SetSpecularInfo(material_data_.isSpecularEnable, material_data_.fSpecularPower, material_data_.bSphereMapIndex);
			}

			rkMtrl.ApplyRenderState();
			STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vtxMeshBasePos, pTriGroupNode->idxPos, pTriGroupNode->triCount);
			rkMtrl.RestoreRenderState();
			
			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}

		pMeshNode = pMeshNode->pNextMeshNode;
	}
}

// With Two Texture
void CGrannyModelInstance::RenderMeshNodeListWithTwoTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	assert(m_pModel != NULL);

	auto lpd3dIdxBuf = m_pModel->GetIndexBuffer();
	assert(lpd3dIdxBuf != NULL);

	const CGrannyModel::TMeshNode * pMeshNode = m_pModel->GetMeshNodeList(eMeshType, eMtrlType);

	while (pMeshNode)
	{
		const CGrannyMesh * pMesh = pMeshNode->pMesh;
		int vtxMeshBasePos = pMesh->GetVertexBasePosition();

		_mgr->SetIndexBuffer(lpd3dIdxBuf);
		STATEMANAGER.SetTransform(World, &m_meshMatrices[pMeshNode->iMesh]);
		if (eMeshType == CGrannyMesh::TYPE_DEFORM)
			UploadMeshBonePaletteToShader(pMeshNode->iMesh);

		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList(eMtrlType);
		int vtxCount = pMesh->GetVertexCount();
		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;

			const CGrannyMaterial& rkMtrl=m_kMtrlPal.GetMaterialRef(pTriGroupNode->mtrlIndex);
			STATEMANAGER.SetTexture(0, rkMtrl.GetSRV(0));
			STATEMANAGER.SetTexture(1, rkMtrl.GetSRV(1));
			STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vtxMeshBasePos, pTriGroupNode->idxPos, pTriGroupNode->triCount);
			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}

		pMeshNode = pMeshNode->pNextMeshNode;
	}
}

// Without Texture
void CGrannyModelInstance::RenderMeshNodeListWithoutTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	assert(m_pModel != NULL);

	auto lpd3dIdxBuf = m_pModel->GetIndexBuffer();
	assert(lpd3dIdxBuf != NULL);

	const CGrannyModel::TMeshNode * pMeshNode = m_pModel->GetMeshNodeList(eMeshType, eMtrlType);

	while (pMeshNode)
	{
		const CGrannyMesh * pMesh = pMeshNode->pMesh;
		int vtxMeshBasePos = pMesh->GetVertexBasePosition();

		_mgr->SetIndexBuffer(lpd3dIdxBuf);
		STATEMANAGER.SetTransform(World, &m_meshMatrices[pMeshNode->iMesh]);
		if (eMeshType == CGrannyMesh::TYPE_DEFORM)
			UploadMeshBonePaletteToShader(pMeshNode->iMesh);

		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList(eMtrlType);
		int vtxCount = pMesh->GetVertexCount();

		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;
			STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vtxMeshBasePos, pTriGroupNode->idxPos, pTriGroupNode->triCount);
			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}

		pMeshNode = pMeshNode->pNextMeshNode;
	}
}

