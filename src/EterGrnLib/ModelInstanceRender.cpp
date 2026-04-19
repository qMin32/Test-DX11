#include "StdAfx.h"
#include "Eterlib/StateManager.h"
#include "ModelInstance.h"
#include "Model.h"
#include "qMin32Lib/All.h"

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

	// DELETED
	//m_pgrnWorldPose = m_pgrnWorldPoseReal;
	///////////////////////////////
	
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
	// FIXME : Deform, Render, BlendRender를 묶어 상위에서 걸러주는 것이 더 나을 듯 - [levites]
	if (IsEmpty())
		return;

#ifdef _TEST
	Granny_RenderBoxBones(GrannyGetSourceSkeleton(m_pgrnModelInstance), m_pgrnWorldPose, TEST_matWorld);
	if (GetAsyncKeyState('P'))
		Tracef("render %x", m_pgrnModelInstance);	
	return;
#endif

	_mgr->SetShader(VF_PNT);
	// WORK
	ID3D11Buffer* lpd3dDeformPNTVtxBuf = __GetDeformableD3DVertexBufferPtr();
	// END_OF_WORK

	ID3D11Buffer* lpd3dRigidPNTVtxBuf = m_pModel->GetPNTD3DVertexBuffer();

	if (lpd3dDeformPNTVtxBuf)
	{
		STATEMANAGER.SetStreamSource(0, lpd3dDeformPNTVtxBuf, sizeof(TPNTVertex));
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
	if (lpd3dRigidPNTVtxBuf)
	{
		STATEMANAGER.SetStreamSource(0, lpd3dRigidPNTVtxBuf, sizeof(TPNTVertex));
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
}

void CGrannyModelInstance::BlendRenderWithOneTexture()
{
	if (IsEmpty())
		return;

	// WORK
	ID3D11Buffer* lpd3dDeformPNTVtxBuf = __GetDeformableD3DVertexBufferPtr();
	// END_OF_WORK
	ID3D11Buffer* lpd3dRigidPNTVtxBuf = m_pModel->GetPNTD3DVertexBuffer();

	_mgr->SetShader(VF_PNT);

	if (lpd3dDeformPNTVtxBuf)
	{
		STATEMANAGER.SetStreamSource(0, lpd3dDeformPNTVtxBuf, sizeof(TPNTVertex));
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	if (lpd3dRigidPNTVtxBuf)
	{
		STATEMANAGER.SetStreamSource(0, lpd3dRigidPNTVtxBuf, sizeof(TPNTVertex));
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

// With Two Texture
void CGrannyModelInstance::RenderWithTwoTexture()
{
	// FIXME : Deform, Render, BlendRender를 묶어 상위에서 걸러주는 것이 더 나을 듯 - [levites]
	if (IsEmpty())
		return;

	_mgr->SetShader(VF_PNT2);

	// WORK
	ID3D11Buffer* lpd3dDeformPNTVtxBuf = __GetDeformableD3DVertexBufferPtr();
	// END_OF_WORK
	ID3D11Buffer* lpd3dRigidPNTVtxBuf = m_pModel->GetPNTD3DVertexBuffer();

	if (lpd3dDeformPNTVtxBuf)
	{
		STATEMANAGER.SetStreamSource(0, lpd3dDeformPNTVtxBuf, sizeof(TPNT2Vertex));
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
	if (lpd3dRigidPNTVtxBuf)
	{
		STATEMANAGER.SetStreamSource(0, lpd3dRigidPNTVtxBuf, sizeof(TPNT2Vertex));
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
}

void CGrannyModelInstance::BlendRenderWithTwoTexture()
{
	if (IsEmpty())
		return;

	// WORK
	ID3D11Buffer* lpd3dDeformPNTVtxBuf = __GetDeformableD3DVertexBufferPtr();
	// END_OF_WORK
	ID3D11Buffer* lpd3dRigidPNTVtxBuf = m_pModel->GetPNTD3DVertexBuffer();

	_mgr->SetShader(VF_PNT);

	if (lpd3dDeformPNTVtxBuf)
	{
		STATEMANAGER.SetStreamSource(0, lpd3dDeformPNTVtxBuf, sizeof(TPNT2Vertex));
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	if (lpd3dRigidPNTVtxBuf)
	{
		STATEMANAGER.SetStreamSource(0, lpd3dRigidPNTVtxBuf, sizeof(TPNT2Vertex));
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

void CGrannyModelInstance::RenderWithoutTexture()
{
	if (IsEmpty())
		return;

	_mgr->SetShader(VF_PNT);
	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);

	// WORK
	ID3D11Buffer* lpd3dDeformPNTVtxBuf = __GetDeformableD3DVertexBufferPtr();
	// END_OF_WORK
	ID3D11Buffer* lpd3dRigidPNTVtxBuf = m_pModel->GetPNTD3DVertexBuffer();

	if (lpd3dDeformPNTVtxBuf)
	{
		STATEMANAGER.SetStreamSource(0, lpd3dDeformPNTVtxBuf, sizeof(TPNTVertex));
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	if (lpd3dRigidPNTVtxBuf)
	{
		STATEMANAGER.SetStreamSource(0, lpd3dRigidPNTVtxBuf, sizeof(TPNTVertex));
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
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

		/////
		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList(eMtrlType);
		int vtxCount = pMesh->GetVertexCount();

		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;

			// MR-12: Fix specular isolation issue
			CGrannyMaterial& rkMtrl = m_kMtrlPal.GetMaterialRef(pTriGroupNode->mtrlIndex);

			if (!material_data_.pImage)
			{
				if (std::fabs(rkMtrl.GetSpecularPower() - material_data_.fSpecularPower) >= std::numeric_limits<float>::epsilon())
					rkMtrl.SetSpecularInfo(material_data_.isSpecularEnable, material_data_.fSpecularPower, material_data_.bSphereMapIndex);
			}
			// MR-12: -- END OF -- Fix specular isolation issue

			rkMtrl.ApplyRenderState();
			STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vtxMeshBasePos, 0, vtxCount, pTriGroupNode->idxPos, pTriGroupNode->triCount);
			rkMtrl.RestoreRenderState();
			
			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}
		/////

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

		/////
		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList(eMtrlType);
		int vtxCount = pMesh->GetVertexCount();
		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;

			const CGrannyMaterial& rkMtrl=m_kMtrlPal.GetMaterialRef(pTriGroupNode->mtrlIndex);
			STATEMANAGER.SetTexture(0, rkMtrl.GetSRV(0));
			STATEMANAGER.SetTexture(1, rkMtrl.GetSRV(1));
			STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vtxMeshBasePos, 0, vtxCount, pTriGroupNode->idxPos, pTriGroupNode->triCount);
			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}
		/////

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

		/////
		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList(eMtrlType);
		int vtxCount = pMesh->GetVertexCount();

		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;
			STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vtxMeshBasePos, 0, vtxCount, pTriGroupNode->idxPos, pTriGroupNode->triCount);
			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}
		/////

		pMeshNode = pMeshNode->pNextMeshNode;
	}
}

