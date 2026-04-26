///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper Class
//
//	(c) 2003 IDV, Inc.
//
//	This class is provided to illustrate one way to incorporate
//	SpeedTreeRT into an OpenGL application.  All of the SpeedTreeRT
//	calls that must be made on a per tree basis are done by this class.
//	Calls that apply to all trees (i.e. static SpeedTreeRT functions)
//	are made in the functions in main.cpp.
//
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//
//		IDV, Inc.
//		1233 Washington St. Suite 610
//		Columbia, SC 29201
//		Voice: (803) 799-1699
//		Fax:   (803) 931-0320
//		Web:   http://www.idvinc.com
//

#pragma warning(disable:4786)

///////////////////////////////////////////////////////////////////////  
//	Include Files
#include "StdAfx.h"

#include <stdlib.h>
#include <stdio.h>
#include "EterBase/Debug.h"
#include "EterBase/Timer.h"
#include "EterBase/Filename.h"
#include "EterLib/ResourceManager.h"
#include "EterLib/Camera.h"
#include "EterLib/StateManager.h"

#include "SpeedTreeConfig.h"
#include "SpeedTreeForestDirectX.h"
#include "SpeedTreeWrapper.h"
#include "VertexShaders.h"
#include "qMin32Lib/ConstantBufferManager.h"
#include "qMin32Lib/DxManager.h"

//

#include <filesystem>

using namespace std;

SpeedTreeShaderPtr CSpeedTreeWrapper::ms_pBranchShader;
SpeedTreeShaderPtr CSpeedTreeWrapper::ms_pLeafShader;
bool CSpeedTreeWrapper::ms_bSelfShadowOn = true;

///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::CSpeedTreeWrapper
CSpeedTreeWrapper::CSpeedTreeWrapper() :
m_pSpeedTree(new CSpeedTreeRT),
m_bIsInstance(false),
m_pInstanceOf(NULL),
m_pGeometryCache(NULL),
m_usNumLeafLods(0),
m_pBranchIndexBuffer(NULL),
m_pBranchVertexBuffer(NULL),
m_pFrondIndexBuffer(NULL),
m_pFrondVertexBuffer(NULL),
m_pLeafVertexBuffer(NULL),
m_pLeavesUpdatedByCpu(NULL),
m_unBranchVertexCount(0),
m_unFrondVertexCount(0),
m_pTextureInfo(NULL)
{
	// set initial position
	m_afPos[0] = m_afPos[1] = m_afPos[2] = 0.0f;
	
	m_pSpeedTree->SetWindStrength(1.0f);
	m_pSpeedTree->SetLocalMatrices(0, 4);
}

void CSpeedTreeWrapper::SetVertexShaders(const SpeedTreeShaderPtr& pBranchShader, const SpeedTreeShaderPtr& pLeafShader)
{
	ms_pBranchShader = pBranchShader;
	ms_pLeafShader = pLeafShader;
}

void CSpeedTreeWrapper::OnRenderPCBlocker()
{
	if (!ms_pBranchShader || !ms_pLeafShader)
		CSpeedTreeForestDirectX::Instance().EnsureVertexShaders();

	if (!ms_pBranchShader) CSpeedTreeForestDirectX::Instance().EnsureVertexShaders();
	
	CSpeedTreeForestDirectX::Instance().UpdateSystem(ELTimer_GetMSec() / 1000.0f);
	
	m_pSpeedTree->SetLodLevel(1.0f);
	//Advance();
	
	CSpeedTreeForestDirectX::Instance().UpdateCompundMatrix(CCameraManager::Instance().GetCurrentCamera()->GetEye(), ms_matView, ms_matProj);
	
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1,	TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG2,	TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP,	TOP11_MODULATE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1,	TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG2,	TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP,	TOP11_MODULATE);

	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAARG2, TA11_CURRENT);
	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP, TOP11_MODULATE);

	DWORD dwLighting = STATEMANAGER.GetRenderState(RS11_LIGHTING);
	DWORD dwFogEnable = STATEMANAGER.GetRenderState(RS11_FOGENABLE);
	DWORD dwAlphaBlendEnable = STATEMANAGER.GetRenderState(RS11_ALPHABLENDENABLE);
 	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);
	STATEMANAGER.SaveRenderState(RS11_LIGHTING, TRUE);
    STATEMANAGER.SetRenderState(RS11_ALPHABLENDENABLE, TRUE);
    STATEMANAGER.SaveRenderState(RS11_ALPHATESTENABLE, TRUE);
    STATEMANAGER.SaveRenderState(RS11_ZFUNC, D3D11_COMPARISON_LESS_EQUAL);
	STATEMANAGER.SaveRenderState(RS11_CULLMODE, D3D11_CULL_NONE);
 	STATEMANAGER.SetRenderState(RS11_FOGENABLE, FALSE);
	
	// choose fixed function pipeline or custom shader for fronds and branches
	if (ms_pBranchShader) ms_pBranchShader->Set();
	
// 	SetupBranchForTreeType();
	{
		// update the branch geometry for CPU wind
#ifdef WRAPPER_USE_CPU_WIND
		m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_BranchGeometry);
		
		if (m_pGeometryCache->m_sBranches.m_usNumStrips > 0)
		{
			// update the vertex array
			SFVFBranchVertex* pVertexBuffer = NULL;
			m_pBranchVertexBuffer->Lock(0, 0, reinterpret_cast<BYTE**>(&pVertexBuffer), D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
			for (UINT i = 0; i < m_unBranchVertexCount; ++i)
			{
				memcpy(&(pVertexBuffer[i].m_vPosition), &(m_pGeometryCache->m_sBranches.m_pCoords[i * 3]), 3 * sizeof(float));
			}
			m_pBranchVertexBuffer->Unlock();
		}
#endif
		
		ID3D11ShaderResourceView* lpd3dTexture;
		
		// set texture map
		if ((lpd3dTexture = m_BranchImageInstance.GetTextureReference().GetSRV()))
			STATEMANAGER.SetTexture(0, lpd3dTexture);
		
		if (m_pGeometryCache->m_sBranches.m_usVertexCount > 0)
		{
			// activate the branch vertex buffer
			_mgr->SetVertexBuffer(m_pBranchVertexBuffer, sizeof(SFVFBranchVertex));
			// set the index buffer
			_mgr->SetIndexBuffer(m_pBranchIndexBuffer);
		}
	}

	RenderBranches();
	
	STATEMANAGER.SetTexture(0, m_CompositeImageInstance.GetTextureReference().GetSRV());
	STATEMANAGER.SetRenderState(RS11_CULLMODE, D3D11_CULL_NONE);
	
// 	SetupFrondForTreeType();
	{
		// update the frond geometry for CPU wind
#ifdef WRAPPER_USE_CPU_WIND
		m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_FrondGeometry);
		if (m_pGeometryCache->m_sFronds.m_usNumStrips > 0)
		{
			// update the vertex array
			SFVFBranchVertex * pVertexBuffer = NULL;
			m_pFrondVertexBuffer->Lock(0, 0, reinterpret_cast<BYTE**>(&pVertexBuffer), D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
			for (UINT i = 0; i < m_unFrondVertexCount; ++i)
			{
				memcpy(&(pVertexBuffer[i].m_vPosition), &(m_pGeometryCache->m_sFronds.m_pCoords[i * 3]), 3 * sizeof(float));
			}
			m_pFrondVertexBuffer->Unlock();
		}
#endif
		
		if (!m_CompositeImageInstance.IsEmpty())
			STATEMANAGER.SetTexture(0, m_CompositeImageInstance.GetTextureReference().GetSRV());
		
		if (m_pGeometryCache->m_sFronds.m_usVertexCount > 0)
		{
			// activate the frond vertex buffer
			_mgr->SetVertexBuffer(m_pFrondVertexBuffer, sizeof(SFVFBranchVertex));
			// set the index buffer
			_mgr->SetIndexBuffer(m_pFrondIndexBuffer);
		}
	}
	RenderFronds();
	
	if (ms_pLeafShader)
	{
		ms_pLeafShader->Set();
		
// 	SetupLeafForTreeType();
		{
			// pass leaf tables to shader
#ifdef WRAPPER_USE_GPU_LEAF_PLACEMENT
			UploadLeafTables(c_nVertexShader_LeafTables);
#endif
			
			if (!m_CompositeImageInstance.IsEmpty())
				STATEMANAGER.SetTexture(0, m_CompositeImageInstance.GetTextureReference().GetSRV());
		}
		RenderLeaves();
	}
	EndLeafForTreeType();
	
	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);
	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);
	RenderBillboards();
	
	STATEMANAGER.RestoreRenderState(RS11_LIGHTING);
	STATEMANAGER.RestoreRenderState(RS11_CULLMODE);
	STATEMANAGER.RestoreRenderState(RS11_ALPHATESTENABLE);
	STATEMANAGER.RestoreRenderState(RS11_ZFUNC);
	STATEMANAGER.SetRenderState(RS11_ALPHABLENDENABLE, dwAlphaBlendEnable);
	STATEMANAGER.SetRenderState(RS11_LIGHTING, dwLighting);
 	STATEMANAGER.SetRenderState(RS11_FOGENABLE, dwFogEnable);

	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP, TOP11_SELECTARG1);
}

void CSpeedTreeWrapper::OnRender()
{
	if (!ms_pBranchShader || !ms_pLeafShader)
		CSpeedTreeForestDirectX::Instance().EnsureVertexShaders();

	if (!ms_pBranchShader) CSpeedTreeForestDirectX::Instance().EnsureVertexShaders();
	
	CSpeedTreeForestDirectX::Instance().UpdateSystem(ELTimer_GetMSec() / 1000.0f);
	
	// ï¿½Ï³ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ LOD ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
	m_pSpeedTree->SetLodLevel(1.0f);
	//Advance();
	
	CSpeedTreeForestDirectX::Instance().UpdateCompundMatrix(CCameraManager::Instance().GetCurrentCamera()->GetEye(), ms_matView, ms_matProj);
	
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1,	TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG2,	TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP,	TOP11_MODULATE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1,	TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG2,	TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP,	TOP11_MODULATE);
	
	STATEMANAGER.SetTextureStageState(1, TSS11_COLOROP, TOP11_MODULATE);
	STATEMANAGER.SetTextureStageState(1, TSS11_COLORARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(1, TSS11_COLORARG2, TA11_CURRENT);
	STATEMANAGER.SetSamplerState(1, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_WRAP);
	STATEMANAGER.SetSamplerState(1, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_WRAP);
	
	STATEMANAGER.SaveRenderState(RS11_LIGHTING, FALSE);
	STATEMANAGER.SaveRenderState(RS11_LIGHTING, TRUE);
    STATEMANAGER.SaveRenderState(RS11_ALPHATESTENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_ZFUNC, D3D11_COMPARISON_LESS_EQUAL);
	STATEMANAGER.SaveRenderState(RS11_CULLMODE, D3D11_CULL_NONE);
	STATEMANAGER.SaveRenderState(RS11_FOGENABLE, FALSE);
	
	// choose fixed function pipeline or custom shader for fronds and branches
	if (ms_pBranchShader) ms_pBranchShader->Set();
	
	SetupBranchForTreeType();
	RenderBranches();
	
	STATEMANAGER.SetTexture(0, m_CompositeImageInstance.GetTextureReference().GetSRV());
	STATEMANAGER.SetRenderState(RS11_CULLMODE, D3D11_CULL_NONE);
	
	SetupFrondForTreeType();
	RenderFronds();
	
	if (ms_pLeafShader)
	{
		ms_pLeafShader->Set();
		
		SetupLeafForTreeType();
		RenderLeaves();
	}
	EndLeafForTreeType();
	
	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);
	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);
	RenderBillboards();
	
	STATEMANAGER.RestoreRenderState(RS11_LIGHTING);
	STATEMANAGER.RestoreRenderState(RS11_LIGHTING);
    STATEMANAGER.RestoreRenderState(RS11_ALPHATESTENABLE);
	STATEMANAGER.RestoreRenderState(RS11_ZFUNC);
	STATEMANAGER.RestoreRenderState(RS11_CULLMODE);
	STATEMANAGER.RestoreRenderState(RS11_FOGENABLE);
}

///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::~CSpeedTreeWrapper

CSpeedTreeWrapper::~CSpeedTreeWrapper()
{
	// if this is not an instance, clean up
	if (!m_bIsInstance)
	{
		if (m_unBranchVertexCount > 0)
		{
			m_pBranchVertexBuffer.reset();
			m_pBranchIndexBuffer.reset();
		}
		
		if (m_unFrondVertexCount > 0)
		{	
			m_pFrondVertexBuffer.reset();
			m_pFrondIndexBuffer.reset();
		}
		
		for (short i = 0; i < m_usNumLeafLods; ++i)
		{			
			m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_LeafGeometry, -1, -1, i);
			
			if (m_pGeometryCache->m_sLeaves0.m_usLeafCount > 0)
				m_pLeafVertexBuffer[i].reset();
		}
		
		SAFE_DELETE_ARRAY(m_pLeavesUpdatedByCpu);
		SAFE_DELETE_ARRAY(m_pLeafVertexBuffer);
		
		SAFE_DELETE(m_pTextureInfo);

		SAFE_DELETE(m_pGeometryCache);
	}
	
	// always delete the speedtree
	SAFE_DELETE(m_pSpeedTree);

	Clear();
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::LoadTree
bool CSpeedTreeWrapper::LoadTree(const char * pszSptFile, const BYTE * c_pbBlock, unsigned int uiBlockSize, UINT nSeed, float fSize, float fSizeVariance)
{
    bool bSuccess = false;
	
	// directx, so allow for flipping of the texture coordinate
#ifdef WRAPPER_FLIP_T_TEXCOORD
	m_pSpeedTree->SetTextureFlip(true);
#endif
	
	// load the tree file
	if (!m_pSpeedTree->LoadTree(c_pbBlock, uiBlockSize))
	{
		if (!m_pSpeedTree->LoadTree(pszSptFile))
		{
			TraceError("SpeedTreeRT Error: %s", CSpeedTreeRT::GetCurrentError());
			return false;
		}
	}
		
	// override the lighting method stored in the spt file
#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
	m_pSpeedTree->SetBranchLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);
	m_pSpeedTree->SetLeafLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);
	m_pSpeedTree->SetFrondLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);
#else
	m_pSpeedTree->SetBranchLightingMethod(CSpeedTreeRT::LIGHT_STATIC);
	m_pSpeedTree->SetLeafLightingMethod(CSpeedTreeRT::LIGHT_STATIC);
	m_pSpeedTree->SetFrondLightingMethod(CSpeedTreeRT::LIGHT_STATIC);
#endif
	
	// set the wind method
#ifdef WRAPPER_USE_GPU_WIND
	m_pSpeedTree->SetBranchWindMethod(CSpeedTreeRT::WIND_GPU);
	m_pSpeedTree->SetLeafWindMethod(CSpeedTreeRT::WIND_GPU);
	m_pSpeedTree->SetFrondWindMethod(CSpeedTreeRT::WIND_GPU);
#endif
#ifdef WRAPPER_USE_CPU_WIND
	m_pSpeedTree->SetBranchWindMethod(CSpeedTreeRT::WIND_CPU);
	m_pSpeedTree->SetLeafWindMethod(CSpeedTreeRT::WIND_CPU);
	m_pSpeedTree->SetFrondWindMethod(CSpeedTreeRT::WIND_CPU);
#endif
#ifdef WRAPPER_USE_NO_WIND
	m_pSpeedTree->SetBranchWindMethod(CSpeedTreeRT::WIND_NONE);
	m_pSpeedTree->SetLeafWindMethod(CSpeedTreeRT::WIND_NONE);
	m_pSpeedTree->SetFrondWindMethod(CSpeedTreeRT::WIND_NONE);
#endif
	
	m_pSpeedTree->SetNumLeafRockingGroups(1);
	
	// override the size, if necessary
	if (fSize >= 0.0f && fSizeVariance >= 0.0f)
		m_pSpeedTree->SetTreeSize(fSize, fSizeVariance);
	
	// generate tree geometry
	if (m_pSpeedTree->Compute(NULL, nSeed, false))
	{
		// get the dimensions
		m_pSpeedTree->GetBoundingBox(m_afBoundingBox);
		
		// make the leaves rock in the wind
		m_pSpeedTree->SetLeafRockingState(true);
		
		// billboard setup
#ifdef WRAPPER_NO_BILLBOARD_MODE
		CSpeedTreeRT::SetDropToBillboard(false);
#else
		CSpeedTreeRT::SetDropToBillboard(true);
#endif
		
		// query & set materials
		m_cBranchMaterial.Set(m_pSpeedTree->GetBranchMaterial());
		m_cFrondMaterial.Set(m_pSpeedTree->GetFrondMaterial());
		m_cLeafMaterial.Set(m_pSpeedTree->GetLeafMaterial());
		
		// adjust lod distances
		float fHeight = m_afBoundingBox[5] - m_afBoundingBox[2];
		m_pSpeedTree->SetLodLimits(fHeight * c_fNearLodFactor, fHeight * c_fFarLodFactor);
		
		// query textures
		m_pTextureInfo = new CSpeedTreeRT::STextures;
		m_pSpeedTree->GetTextures(*m_pTextureInfo);
		
		std::filesystem::path path = pszSptFile;
		path = path.parent_path();

		auto branchTexture = path / m_pTextureInfo->m_pBranchTextureFilename;
		branchTexture.replace_extension(".dds");

		// load branch textures
		LoadTexture(branchTexture.generic_string().c_str(), m_BranchImageInstance);
		
#ifdef WRAPPER_RENDER_SELF_SHADOWS
		auto selfShadowTexture = path / m_pTextureInfo->m_pSelfShadowFilename;
		selfShadowTexture.replace_extension(".dds");

		if (m_pTextureInfo->m_pSelfShadowFilename != NULL)
			LoadTexture(selfShadowTexture.generic_string().c_str(), m_ShadowImageInstance);
#endif

		auto compositeTexture = path / m_pTextureInfo->m_pCompositeFilename;
		compositeTexture.replace_extension(".dds");

		if (m_pTextureInfo->m_pCompositeFilename)
			LoadTexture(compositeTexture.generic_string().c_str(), m_CompositeImageInstance);
		
		// setup the index and vertex buffers
		SetupBuffers();
		
		// everything appeared to go well
		bSuccess = true;
	}
	else // tree failed to compute
		fprintf(stderr, "\nFatal Error, cannot compute tree [%s]\n\n", CSpeedTreeRT::GetCurrentError());
	
    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::SetupBuffers

void CSpeedTreeWrapper::SetupBuffers(void)
{
	// read all the geometry for highest LOD into the geometry cache (just a precaution, it's updated later)
	m_pSpeedTree->SetLodLevel(1.0f);
	
	if (m_pGeometryCache == NULL)
		m_pGeometryCache = new CSpeedTreeRT::SGeometry;
	
	m_pSpeedTree->GetGeometry(*m_pGeometryCache);
	
	// setup the buffers for each part
	SetupBranchBuffers();
	SetupFrondBuffers();
	SetupLeafBuffers();
}

///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::SetupBranchBuffers

void CSpeedTreeWrapper::SetupBranchBuffers(void)
{
	CSpeedTreeRT::SGeometry::SIndexed* pBranches = &(m_pGeometryCache->m_sBranches);
	m_unBranchVertexCount = pBranches->m_usVertexCount;

	if (m_unBranchVertexCount <= 1 || !_mgr)
		return;

	std::vector<SFVFBranchVertex> vertices(m_unBranchVertexCount);
	for (UINT i = 0; i < m_unBranchVertexCount; ++i)
	{
		SFVFBranchVertex& v = vertices[i];
		memcpy(&v.m_vPosition, &(pBranches->m_pCoords[i * 3]), 3 * sizeof(float));
#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
		memcpy(&v.m_vNormal, &(pBranches->m_pNormals[i * 3]), 3 * sizeof(float));
#else
		v.m_dwDiffuseColor = pBranches->m_pColors[i];
#endif
		v.m_fTexCoords[0] = pBranches->m_pTexCoords0[i * 2];
		v.m_fTexCoords[1] = pBranches->m_pTexCoords0[i * 2 + 1];
#ifdef WRAPPER_RENDER_SELF_SHADOWS
		v.m_fShadowCoords[0] = pBranches->m_pTexCoords1[i * 2];
		v.m_fShadowCoords[1] = pBranches->m_pTexCoords1[i * 2 + 1];
#endif
#ifdef WRAPPER_USE_GPU_WIND
		v.m_fWindIndex = 4.0f * pBranches->m_pWindMatrixIndices[i];
		v.m_fWindWeight = pBranches->m_pWindWeights[i];
#endif
	}

	_mgr->CreateVertexBuffer(m_pBranchVertexBuffer, vertices.data(), (UINT)vertices.size(), sizeof(SFVFBranchVertex),
#ifdef WRAPPER_USE_CPU_WIND
		true
#else
		false
#endif
	);

	const uint32_t unNumLodLevels = m_pSpeedTree->GetNumBranchLodLevels();
	m_branchStripOffsets.clear();
	m_branchStripLengths.clear();
	if (unNumLodLevels > 0)
		m_branchStripLengths.resize(unNumLodLevels);

	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_BranchGeometry, 0);
	const uint32_t stripCount = pBranches->m_usNumStrips;
	uint32_t totalIndexCount = 0;
	m_branchStripOffsets.resize(stripCount);
	for (uint32_t s = 0; s < stripCount; ++s)
	{
		m_branchStripOffsets[s] = totalIndexCount;
		totalIndexCount += pBranches->m_pStripLengths[s];
	}

	for (uint32_t lod = 0; lod < unNumLodLevels; ++lod)
	{
		m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_BranchGeometry, lod);
		auto& lengths = m_branchStripLengths[lod];
		lengths.assign(stripCount, 0);
		for (uint32_t st = 0; st < stripCount && st < pBranches->m_usNumStrips; ++st)
			lengths[st] = pBranches->m_pStripLengths[st];
	}

	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_BranchGeometry, 0);

	if (totalIndexCount > 0)
	{
		std::vector<uint16_t> indices(totalIndexCount);
		uint32_t cursor = 0;
		for (uint32_t st = 0; st < stripCount; ++st)
		{
			const uint32_t length = pBranches->m_pStripLengths[st];
			memcpy(indices.data() + cursor, pBranches->m_pStrips[st], length * sizeof(uint16_t));
			cursor += length;
		}
		_mgr->CreateIndexBuffer(m_pBranchIndexBuffer, indices.data(), (UINT)indices.size(), false, DXGI_FORMAT_R16_UINT);
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::SetupFrondBuffers

void CSpeedTreeWrapper::SetupFrondBuffers(void)
{
	CSpeedTreeRT::SGeometry::SIndexed* pFronds = &(m_pGeometryCache->m_sFronds);
	m_unFrondVertexCount = pFronds->m_usVertexCount;

	if (m_unFrondVertexCount <= 1 || !_mgr)
		return;

	std::vector<SFVFBranchVertex> vertices(m_unFrondVertexCount);
	for (UINT i = 0; i < m_unFrondVertexCount; ++i)
	{
		SFVFBranchVertex& v = vertices[i];
		memcpy(&v.m_vPosition, &(pFronds->m_pCoords[i * 3]), 3 * sizeof(float));
#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
		memcpy(&v.m_vNormal, &(pFronds->m_pNormals[i * 3]), 3 * sizeof(float));
#else
		v.m_dwDiffuseColor = pFronds->m_pColors[i];
#endif
		v.m_fTexCoords[0] = pFronds->m_pTexCoords0[i * 2];
		v.m_fTexCoords[1] = pFronds->m_pTexCoords0[i * 2 + 1];
#ifdef WRAPPER_RENDER_SELF_SHADOWS
		v.m_fShadowCoords[0] = pFronds->m_pTexCoords1[i * 2];
		v.m_fShadowCoords[1] = pFronds->m_pTexCoords1[i * 2 + 1];
#endif
#ifdef WRAPPER_USE_GPU_WIND
		v.m_fWindIndex = 4.0f * pFronds->m_pWindMatrixIndices[i];
		v.m_fWindWeight = pFronds->m_pWindWeights[i];
#endif
	}

	_mgr->CreateVertexBuffer(m_pFrondVertexBuffer, vertices.data(), (UINT)vertices.size(), sizeof(SFVFBranchVertex),
#ifdef WRAPPER_USE_CPU_WIND
		true
#else
		false
#endif
	);

	const uint32_t unNumLodLevels = m_pSpeedTree->GetNumFrondLodLevels();
	m_frondStripOffsets.clear();
	m_frondStripLengths.clear();
	if (unNumLodLevels > 0)
		m_frondStripLengths.resize(unNumLodLevels);

	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_FrondGeometry, -1, 0);
	const uint32_t stripCount = pFronds->m_usNumStrips;
	uint32_t totalIndexCount = 0;
	m_frondStripOffsets.resize(stripCount);
	for (uint32_t s = 0; s < stripCount; ++s)
	{
		m_frondStripOffsets[s] = totalIndexCount;
		totalIndexCount += pFronds->m_pStripLengths[s];
	}

	for (uint32_t lod = 0; lod < unNumLodLevels; ++lod)
	{
		m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_FrondGeometry, -1, lod);
		auto& lengths = m_frondStripLengths[lod];
		lengths.assign(stripCount, 0);
		for (uint32_t st = 0; st < stripCount && st < pFronds->m_usNumStrips; ++st)
			lengths[st] = pFronds->m_pStripLengths[st];
	}

	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_FrondGeometry, -1, 0);

	if (totalIndexCount > 0)
	{
		std::vector<uint16_t> indices(totalIndexCount);
		uint32_t cursor = 0;
		for (uint32_t st = 0; st < stripCount; ++st)
		{
			const uint32_t length = pFronds->m_pStripLengths[st];
			memcpy(indices.data() + cursor, pFronds->m_pStrips[st], length * sizeof(uint16_t));
			cursor += length;
		}
		_mgr->CreateIndexBuffer(m_pFrondIndexBuffer, indices.data(), (UINT)indices.size(), false, DXGI_FORMAT_R16_UINT);
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::SetupLeafBuffers

void CSpeedTreeWrapper::SetupLeafBuffers(void)
{
	const short anVertexIndices[6] = { 0, 1, 2, 0, 2, 3 };

	m_usNumLeafLods = m_pSpeedTree->GetNumLeafLodLevels();
	m_pLeafVertexBuffer = new VBufferPtr[m_usNumLeafLods];
	m_pLeavesUpdatedByCpu = new bool[m_usNumLeafLods];

	for (UINT unLod = 0; unLod < m_usNumLeafLods; ++unLod)
	{
		m_pLeavesUpdatedByCpu[unLod] = false;
		m_pLeafVertexBuffer[unLod].reset();

		const CSpeedTreeRT::SGeometry::SLeaf* pLeaf = &(m_pGeometryCache->m_sLeaves0);
		unsigned short usLeafCount = pLeaf->m_usLeafCount;
		if (usLeafCount < 1 || !_mgr)
			continue;

		std::vector<SFVFLeafVertex> vertices(usLeafCount * 6);
		SFVFLeafVertex* pVertex = vertices.data();
		for (UINT unLeaf = 0; unLeaf < usLeafCount; ++unLeaf)
		{
			for (UINT unVert = 0; unVert < 6; ++unVert)
			{
				memcpy(&pVertex->m_vPosition, &(pLeaf->m_pCenterCoords[unLeaf * 3]), 3 * sizeof(float));
#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
				memcpy(&pVertex->m_vNormal, &(pLeaf->m_pNormals[unLeaf * 3]), 3 * sizeof(float));
#else
				pVertex->m_dwDiffuseColor = pLeaf->m_pColors[unLeaf];
#endif
				memcpy(pVertex->m_fTexCoords, &(pLeaf->m_pLeafMapTexCoords[unLeaf][anVertexIndices[unVert] * 2]), 2 * sizeof(float));
#ifdef WRAPPER_USE_GPU_WIND
				pVertex->m_fWindIndex = 4.0f * pLeaf->m_pWindMatrixIndices[unLeaf];
				pVertex->m_fWindWeight = pLeaf->m_pWindWeights[unLeaf];
#endif
#ifdef WRAPPER_USE_GPU_LEAF_PLACEMENT
				pVertex->m_fLeafPlacementIndex = c_nVertexShader_LeafTables + pLeaf->m_pLeafClusterIndices[unLeaf] * 4.0f + anVertexIndices[unVert];
				pVertex->m_fLeafScalarValue = m_pSpeedTree->GetLeafLodSizeAdjustments()[unLod];
#endif
				++pVertex;
			}
		}

		_mgr->CreateVertexBuffer(m_pLeafVertexBuffer[unLod], vertices.data(), (UINT)vertices.size(), sizeof(SFVFLeafVertex),
#if defined(WRAPPER_USE_CPU_LEAF_PLACEMENT) || !defined(WRAPPER_USE_GPU_LEAF_PLACEMENT)
			true
#else
			false
#endif
		);
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::Advance

void CSpeedTreeWrapper::Advance(void)
{
	// compute LOD level (based on distance from camera)
	m_pSpeedTree->ComputeLodLevel();
	m_pSpeedTree->SetLodLevel(1.0f);
	
	// compute wind
#ifdef WRAPPER_USE_CPU_WIND
	m_pSpeedTree->ComputeWindEffects(true, true, true);
#endif
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::MakeInstance
CSpeedTreeWrapper::SpeedTreeWrapperPtr CSpeedTreeWrapper::MakeInstance()
{
	auto spInstance = std::make_shared<CSpeedTreeWrapper>();
	
	// make an instance of this object's SpeedTree
	spInstance->m_bIsInstance = true;

	SAFE_DELETE(spInstance->m_pSpeedTree);
	spInstance->m_pSpeedTree = m_pSpeedTree->MakeInstance();
	
	if (spInstance->m_pSpeedTree)
    {
		// use the same materials
		spInstance->m_cBranchMaterial = m_cBranchMaterial;
		spInstance->m_cLeafMaterial = m_cLeafMaterial;
		spInstance->m_cFrondMaterial = m_cFrondMaterial;
		spInstance->m_CompositeImageInstance.SetImagePointer(m_CompositeImageInstance.GetGraphicImagePointer());
		spInstance->m_BranchImageInstance.SetImagePointer(m_BranchImageInstance.GetGraphicImagePointer());
		
		if (!m_ShadowImageInstance.IsEmpty())
			spInstance->m_ShadowImageInstance.SetImagePointer(m_ShadowImageInstance.GetGraphicImagePointer());
		
		spInstance->m_pTextureInfo = m_pTextureInfo;
		
		// use the same geometry cache
		spInstance->m_pGeometryCache = m_pGeometryCache;
		
		// use the same buffers
		spInstance->m_pBranchIndexBuffer = m_pBranchIndexBuffer;
		spInstance->m_branchStripOffsets = m_branchStripOffsets;
		spInstance->m_branchStripLengths = m_branchStripLengths;
		spInstance->m_pBranchVertexBuffer = m_pBranchVertexBuffer;
		spInstance->m_unBranchVertexCount = m_unBranchVertexCount;
		
		spInstance->m_pFrondIndexBuffer = m_pFrondIndexBuffer;
		spInstance->m_frondStripOffsets = m_frondStripOffsets;
		spInstance->m_frondStripLengths = m_frondStripLengths;
		spInstance->m_pFrondVertexBuffer = m_pFrondVertexBuffer;
		spInstance->m_unFrondVertexCount = m_unFrondVertexCount;
		
		spInstance->m_pLeafVertexBuffer = m_pLeafVertexBuffer;
		spInstance->m_usNumLeafLods = m_usNumLeafLods;
		spInstance->m_pLeavesUpdatedByCpu = m_pLeavesUpdatedByCpu;
		
		// new stuff
		memcpy(spInstance->m_afPos, m_afPos, 3 * sizeof(float));
		memcpy(spInstance->m_afBoundingBox, m_afBoundingBox, 6 * sizeof(float));
		spInstance->m_pInstanceOf = shared_from_this();
		m_vInstances.push_back(spInstance);
    }
    else
	{
		fprintf(stderr, "SpeedTreeRT Error: %s\n", m_pSpeedTree->GetCurrentError());
		spInstance.reset();
	}
	
	return spInstance;
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::GetInstances
std::vector <CSpeedTreeWrapper::SpeedTreeWrapperPtr> CSpeedTreeWrapper::GetInstances(UINT& nCount)
{
	std::vector <SpeedTreeWrapperPtr> kResult;

	nCount = m_vInstances.size();
	if (nCount)
	{
		for (auto it : m_vInstances)
		{
			kResult.push_back(it);
		}
	}

	return kResult;
}

void CSpeedTreeWrapper::DeleteInstance(SpeedTreeWrapperPtr pInstance)
{
	auto itor = m_vInstances.begin();
	
	while (itor != m_vInstances.end())
	{
		if (*itor == pInstance)
		{
			itor = m_vInstances.erase(itor);
		}
		else
			++itor;
	}
}

///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::SetupBranchForTreeType

void CSpeedTreeWrapper::SetupBranchForTreeType(void) const
{
#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
	// set lighting material
	STATEMANAGER.SetMaterial(m_cBranchMaterial.Get());
	SetShaderConstants(m_pSpeedTree->GetBranchMaterial());
#endif
	
	// update the branch geometry for CPU wind
#ifdef WRAPPER_USE_CPU_WIND
	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_BranchGeometry);
	
	if (m_pGeometryCache->m_sBranches.m_usNumStrips > 0)
	{
		// update the vertex array
		SFVFBranchVertex* pVertexBuffer = NULL;
		m_pBranchVertexBuffer->Lock(0, 0, reinterpret_cast<BYTE**>(&pVertexBuffer), D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
		for (UINT i = 0; i < m_unBranchVertexCount; ++i)
		{
			memcpy(&(pVertexBuffer[i].m_vPosition), &(m_pGeometryCache->m_sBranches.m_pCoords[i * 3]), 3 * sizeof(float));
		}
		m_pBranchVertexBuffer->Unlock();
	}
#endif
	
	ID3D11ShaderResourceView* lpd3dTexture;
	
    // set texture map
    if ((lpd3dTexture = m_BranchImageInstance.GetTextureReference().GetSRV()))
        STATEMANAGER.SetTexture(0, lpd3dTexture);
	
	// bind shadow texture
#ifdef WRAPPER_RENDER_SELF_SHADOWS
	if (ms_bSelfShadowOn && (lpd3dTexture = m_ShadowImageInstance.GetTextureReference().GetSRV()))
		STATEMANAGER.SetTexture(1, NULL);
	else
		STATEMANAGER.SetTexture(1, NULL);
#endif
	
	if (m_pGeometryCache->m_sBranches.m_usVertexCount > 0)
	{
		// activate the branch vertex buffer
		_mgr->SetVertexBuffer(m_pBranchVertexBuffer, sizeof(SFVFBranchVertex));
		// set the index buffer
		_mgr->SetIndexBuffer(m_pBranchIndexBuffer);
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::RenderBranches

void CSpeedTreeWrapper::RenderBranches(void) const
{
	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_BranchGeometry);
	
	if (m_pGeometryCache->m_sBranches.m_usVertexCount > 0 && m_pBranchIndexBuffer && !m_branchStripLengths.empty() && !m_branchStripOffsets.empty())
	{
		const int lod = m_pGeometryCache->m_sBranches.m_nDiscreteLodLevel;
		if (lod < 0 || static_cast<size_t>(lod) >= m_branchStripLengths.size())
			return;

		PositionTree();
		
		// set alpha test value
		STATEMANAGER.SetRenderState(RS11_ALPHAREF, DWORD(m_pGeometryCache->m_fBranchAlphaTestValue));
		
		const auto& lengths = m_branchStripLengths[lod];
		const size_t stripCount = lengths.size() < m_branchStripOffsets.size() ? lengths.size() : m_branchStripOffsets.size();
		for (size_t s = 0; s < stripCount; ++s)
		{
			const uint16_t stripLength = lengths[s];
			if (stripLength > 2)
			{
				ms_faceCount += stripLength - 2;
				STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, m_pGeometryCache->m_sBranches.m_usVertexCount, m_branchStripOffsets[s], stripLength - 2);
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::SetupFrondForTreeType

void CSpeedTreeWrapper::SetupFrondForTreeType(void) const
{
#ifdef SPEEDTREE_LIGHTING_DYNAMIC
	// set lighting material
	STATEMANAGER.SetMaterial(m_cFrondMaterial.Get());
	SetShaderConstants(m_pSpeedTree->GetFrondMaterial());
#endif
	
	// update the frond geometry for CPU wind
#ifdef WRAPPER_USE_CPU_WIND
	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_FrondGeometry);
	if (m_pGeometryCache->m_sFronds.m_usNumStrips > 0)
	{
		// update the vertex array
		SFVFBranchVertex * pVertexBuffer = NULL;
		m_pFrondVertexBuffer->Lock(0, 0, reinterpret_cast<BYTE**>(&pVertexBuffer), D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
		for (UINT i = 0; i < m_unFrondVertexCount; ++i)
		{
			memcpy(&(pVertexBuffer[i].m_vPosition), &(m_pGeometryCache->m_sFronds.m_pCoords[i * 3]), 3 * sizeof(float));
		}
		m_pFrondVertexBuffer->Unlock();
	}
#endif
	
	if (!m_CompositeImageInstance.IsEmpty())
		STATEMANAGER.SetTexture(0, m_CompositeImageInstance.GetTextureReference().GetSRV());
	
	// bind shadow texture
#ifdef WRAPPER_RENDER_SELF_SHADOWS
	ID3D11ShaderResourceView* lpd3dTexture;
	
	if ((lpd3dTexture = m_ShadowImageInstance.GetTextureReference().GetSRV()))
		STATEMANAGER.SetTexture(1, NULL);
#endif
	
	if (m_pGeometryCache->m_sFronds.m_usVertexCount > 0)
	{
		// activate the frond vertex buffer
		_mgr->SetVertexBuffer(m_pFrondVertexBuffer, sizeof(SFVFBranchVertex));
		// set the index buffer
		_mgr->SetIndexBuffer(m_pFrondIndexBuffer);
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::RenderFronds

void CSpeedTreeWrapper::RenderFronds(void) const
{
	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_FrondGeometry);
	
	if (m_pGeometryCache->m_sFronds.m_usVertexCount > 0 && m_pFrondIndexBuffer && !m_frondStripLengths.empty() && !m_frondStripOffsets.empty())
	{
		const int lod = m_pGeometryCache->m_sFronds.m_nDiscreteLodLevel;
		if (lod < 0 || static_cast<size_t>(lod) >= m_frondStripLengths.size())
			return;

		PositionTree();
		
		// set alpha test value
		STATEMANAGER.SetRenderState(RS11_ALPHAREF, DWORD(m_pGeometryCache->m_fFrondAlphaTestValue));
		
		const auto& lengths = m_frondStripLengths[lod];
		const size_t stripCount = lengths.size() < m_frondStripOffsets.size() ? lengths.size() : m_frondStripOffsets.size();
		for (size_t s = 0; s < stripCount; ++s)
		{
			const uint16_t stripLength = lengths[s];
			if (stripLength > 2)
			{
				ms_faceCount += stripLength - 2;
				STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, m_pGeometryCache->m_sFronds.m_usVertexCount, m_frondStripOffsets[s], stripLength - 2);
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::SetupLeafForTreeType

void CSpeedTreeWrapper::SetupLeafForTreeType(void) const
{
#ifdef SPEEDTREE_LIGHTING_DYNAMIC
	// set lighting material
	STATEMANAGER.SetMaterial(m_cLeafMaterial.Get());
	SetShaderConstants(m_pSpeedTree->GetLeafMaterial());
#endif
	
	// pass leaf tables to shader
#ifdef WRAPPER_USE_GPU_LEAF_PLACEMENT
	UploadLeafTables(c_nVertexShader_LeafTables);
#endif
	
	if (!m_CompositeImageInstance.IsEmpty())
		STATEMANAGER.SetTexture(0, m_CompositeImageInstance.GetTextureReference().GetSRV());
	
	// bind shadow texture
#ifdef WRAPPER_RENDER_SELF_SHADOWS
	STATEMANAGER.SetTexture(1, NULL);
#endif
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::UploadLeafTables

#ifdef WRAPPER_USE_GPU_LEAF_PLACEMENT
void CSpeedTreeWrapper::UploadLeafTables(UINT uiLocation) const
{
	// query leaf cluster table from RT
	UINT uiEntryCount = 0;
	const float * pTable = m_pSpeedTree->GetLeafBillboardTable(uiEntryCount);
	
	// upload for vertex shader use
	if (_mgr)
		_mgr->GetCbMgr()->SetSpeedTreeLeafTables(pTable, uiEntryCount / 4);
}
#endif


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::RenderLeaves

void CSpeedTreeWrapper::RenderLeaves(void) const
{
	// update leaf geometry
	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_LeafGeometry);

	if (!m_pLeafVertexBuffer || m_usNumLeafLods == 0)
		return;

	const int maxLeafLod = static_cast<int>(m_usNumLeafLods);
	
	// update the LOD level vertex arrays we need
#if defined(WRAPPER_USE_GPU_LEAF_PLACEMENT) && defined(WRAPPER_USE_GPU_WIND)
	// do nothing, needs no updates
#else
#if !defined WRAPPER_USE_NO_WIND || defined WRAPPER_USE_CPU_LEAF_PLACEMENT
	// possibly need to update both leaf LOD's
	for (UINT i = 0; i < 2; ++i)
	{
		// reference to leaf structure
		const CSpeedTreeRT::SGeometry::SLeaf* pLeaf = (i == 0) ? &m_pGeometryCache->m_sLeaves0 : &m_pGeometryCache->m_sLeaves1;
		int unLod = pLeaf->m_nDiscreteLodLevel;

		if (!pLeaf->m_bIsActive || pLeaf->m_usLeafCount == 0)
			continue;

		if (unLod < 0 || unLod >= maxLeafLod)
			continue;

		if (!m_pLeafVertexBuffer[unLod])
			continue;
		
#if defined WRAPPER_USE_GPU_LEAF_PLACEMENT
		if (m_pLeavesUpdatedByCpu && !m_pLeavesUpdatedByCpu[unLod])
		{
			// update the centers
			SFVFLeafVertex* pVertex = NULL;
			m_pLeafVertexBuffer[unLod]->Lock(0, 0, reinterpret_cast<BYTE**>(&pVertex), D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
			for (UINT unLeaf = 0; unLeaf < pLeaf->m_usLeafCount; ++unLeaf)
			{
				SMVector3 vecCenter(&(pLeaf->m_pCenterCoords[unLeaf * 3]));
				(pVertex++)->m_vPosition = vecCenter;		// vertex 0
				(pVertex++)->m_vPosition = vecCenter;		// vertex 1
				(pVertex++)->m_vPosition = vecCenter;		// vertex 2
				(pVertex++)->m_vPosition = vecCenter;		// vertex 0
				(pVertex++)->m_vPosition = vecCenter;		// vertex 2
				(pVertex++)->m_vPosition = vecCenter;		// vertex 3
			}
			m_pLeafVertexBuffer[unLod]->Unlock();
			m_pLeavesUpdatedByCpu[unLod] = true;
		}
#else
		{ 
			// update the vertex positions
			SFVFLeafVertex * pVertex = NULL;

			const UINT VERTEX_NUM = 8192;
			if (pLeaf->m_usLeafCount*3>=VERTEX_NUM)
				return;

			D3DXVECTOR3 akPosition[VERTEX_NUM];
			D3DXVECTOR3* pkPosition = akPosition;
			const float* center=pLeaf->m_pCenterCoords;
			for (UINT unLeaf = 0; unLeaf < pLeaf->m_usLeafCount; ++unLeaf)
			{
				pkPosition[0].x=pLeaf->m_pLeafMapCoords[unLeaf][0]+center[0];
				pkPosition[0].y=pLeaf->m_pLeafMapCoords[unLeaf][1]+center[1];
				pkPosition[0].z=pLeaf->m_pLeafMapCoords[unLeaf][2]+center[2];
				pkPosition[1].x=pLeaf->m_pLeafMapCoords[unLeaf][4]+center[0];
				pkPosition[1].y=pLeaf->m_pLeafMapCoords[unLeaf][5]+center[1];
				pkPosition[1].z=pLeaf->m_pLeafMapCoords[unLeaf][6]+center[2];			
				pkPosition[2].x=pLeaf->m_pLeafMapCoords[unLeaf][8]+center[0];
				pkPosition[2].y=pLeaf->m_pLeafMapCoords[unLeaf][9]+center[1];
				pkPosition[2].z=pLeaf->m_pLeafMapCoords[unLeaf][10]+center[2];
				pkPosition[3]=pkPosition[0];
				pkPosition[4]=pkPosition[2];
				pkPosition[5].x=pLeaf->m_pLeafMapCoords[unLeaf][12]+center[0];
				pkPosition[5].y=pLeaf->m_pLeafMapCoords[unLeaf][13]+center[1];
				pkPosition[5].z=pLeaf->m_pLeafMapCoords[unLeaf][14]+center[2];
				pkPosition+=6;
				center+=3;
			}			
			
			if (SUCCEEDED(
				m_pLeafVertexBuffer[unLod]->Lock(0, 0, reinterpret_cast<void**>(&pVertex), D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK)
			))
			{			
				UINT uVtxCount=pLeaf->m_usLeafCount*6;
				for (UINT i = 0; i < uVtxCount; ++i)
					pVertex[i].m_vPosition = akPosition[i];

				m_pLeafVertexBuffer[unLod]->Unlock();
			}
		}
#endif
	}
#endif
#endif
	
	PositionTree();
	
	// render LODs, if needed
	for (UINT unLeafLevel = 0; unLeafLevel < 2; ++unLeafLevel)
	{
		const CSpeedTreeRT::SGeometry::SLeaf* pLeaf = (unLeafLevel == 0) ?
			&m_pGeometryCache->m_sLeaves0 : &m_pGeometryCache->m_sLeaves1;
		
		int unLod = pLeaf->m_nDiscreteLodLevel;
		
		if (unLod < 0 || unLod >= maxLeafLod || !pLeaf->m_bIsActive || pLeaf->m_usLeafCount == 0)
			continue;

		if (!m_pLeafVertexBuffer[unLod])
			continue;

		_mgr->SetVertexBuffer(m_pLeafVertexBuffer[unLod], sizeof(SFVFLeafVertex));
		STATEMANAGER.SetRenderState(RS11_ALPHAREF, DWORD(pLeaf->m_fAlphaTestValue));
		
		ms_faceCount += pLeaf->m_usLeafCount * 2;
		STATEMANAGER.DrawPrimitive(D3DPT_TRIANGLELIST, 0, pLeaf->m_usLeafCount * 2);
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::EndLeafForTreeType

void CSpeedTreeWrapper::EndLeafForTreeType(void)
{
	if (!m_pLeavesUpdatedByCpu)
		return;

	// reset copy flags for CPU wind
	for (UINT i = 0; i < m_usNumLeafLods; ++i)
		m_pLeavesUpdatedByCpu[i] = false;
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::RenderBillboards

void CSpeedTreeWrapper::RenderBillboards(void) const
{
	// render billboards in immediate mode (as close as DirectX comes to immediate mode)
#ifdef WRAPPER_BILLBOARD_MODE
	if (!m_CompositeImageInstance.IsEmpty())
		STATEMANAGER.SetTexture(0, m_CompositeImageInstance.GetTextureReference().GetSRV());
	
	PositionTree();	
	
	struct SBillboardVertex 
	{
		float fX, fY, fZ;
		float fU, fV;
	};
	
	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_BillboardGeometry);
	
	if (m_pGeometryCache->m_sBillboard0.m_bIsActive)
	{
		const float* pCoords = m_pGeometryCache->m_sBillboard0.m_pCoords;
		const float* pTexCoords = m_pGeometryCache->m_sBillboard0.m_pTexCoords;
		SBillboardVertex sVertex[4] = 
		{
			{ pCoords[0], pCoords[1], pCoords[2], pTexCoords[0], pTexCoords[1] },
			{ pCoords[3], pCoords[4], pCoords[5], pTexCoords[2], pTexCoords[3] },
			{ pCoords[6], pCoords[7], pCoords[8], pTexCoords[4], pTexCoords[5] },
			{ pCoords[9], pCoords[10], pCoords[11], pTexCoords[6], pTexCoords[7] },
		};
		
		STATEMANAGER.SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
		STATEMANAGER.SetRenderState(RS11_ALPHAREF, DWORD(m_pGeometryCache->m_sBillboard0.m_fAlphaTestValue));
		
		ms_faceCount += 2;
		STATEMANAGER.DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, sVertex, sizeof(SBillboardVertex));
	}
	
	// if tree supports 360 degree billboards, render the second
	if (m_pGeometryCache->m_sBillboard1.m_bIsActive)
	{
		const float* pCoords = m_pGeometryCache->m_sBillboard1.m_pCoords;
		const float* pTexCoords = m_pGeometryCache->m_sBillboard1.m_pTexCoords;
		SBillboardVertex sVertex[4] = 
		{
			{ pCoords[0], pCoords[1], pCoords[2], pTexCoords[0], pTexCoords[1] },
			{ pCoords[3], pCoords[4], pCoords[5], pTexCoords[2], pTexCoords[3] },
			{ pCoords[6], pCoords[7], pCoords[8], pTexCoords[4], pTexCoords[5] },
			{ pCoords[9], pCoords[10], pCoords[11], pTexCoords[6], pTexCoords[7] },
		};
		STATEMANAGER.SetRenderState(RS11_ALPHAREF, DWORD(m_pGeometryCache->m_sBillboard1.m_fAlphaTestValue));
		
		ms_faceCount += 2;
		STATEMANAGER.DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, sVertex, sizeof(SBillboardVertex));
	}
	
#ifdef WRAPPER_RENDER_HORIZONTAL_BILLBOARD
	// render horizontal billboard (if enabled)
	if (m_pGeometryCache->m_sHorizontalBillboard.m_bIsActive)
	{	
		const float* pCoords = m_pGeometryCache->m_sHorizontalBillboard.m_pCoords;
		const float* pTexCoords = m_pGeometryCache->m_sHorizontalBillboard.m_pTexCoords;
		SBillboardVertex sVertex[4] = 
		{
			{ pCoords[0], pCoords[1], pCoords[2], pTexCoords[0], pTexCoords[1] },
			{ pCoords[3], pCoords[4], pCoords[5], pTexCoords[2], pTexCoords[3] },
			{ pCoords[6], pCoords[7], pCoords[8], pTexCoords[4], pTexCoords[5] },
			{ pCoords[9], pCoords[10], pCoords[11], pTexCoords[6], pTexCoords[7] },
		};
		STATEMANAGER.SetRenderState(RS11_ALPHAREF, DWORD(m_pGeometryCache->m_sHorizontalBillboard.m_fAlphaTestValue));
		
		ms_faceCount += 2;
		STATEMANAGER.DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, sVertex, sizeof(SBillboardVertex));
	}
	
#endif
#endif
}

///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::CleanUpMemory

void CSpeedTreeWrapper::CleanUpMemory(void)
{
	if (!m_bIsInstance)
		m_pSpeedTree->DeleteTransientData();
}

///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::PositionTree

void CSpeedTreeWrapper::PositionTree(void) const
{
	D3DXVECTOR3 vecPosition = m_pSpeedTree->GetTreePosition();
	D3DXMATRIX matTranslation;
	D3DXMatrixIdentity(&matTranslation);
	D3DXMatrixTranslation(&matTranslation, vecPosition.x, vecPosition.y, vecPosition.z);

	// store translation for client-side transformation
	STATEMANAGER.SetTransform(World, &matTranslation);

	// store translation for use in vertex shader
	D3DXVECTOR4 vecConstant(vecPosition[0], vecPosition[1], vecPosition[2], 0.0f);
	if (_mgr)
		_mgr->GetCbMgr()->SetSpeedTreeTreePosition(vecConstant);
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::LoadTexture

bool CSpeedTreeWrapper::LoadTexture(const char * pFilename, CGraphicImageInstance & rImage)
{
	CResource * pResource = CResourceManager::Instance().GetResourcePointer(pFilename);
	rImage.SetImagePointer(static_cast<CGraphicImage *>(pResource));

	if (rImage.IsEmpty())
		return false;
	
	//TraceError("SpeedTreeWrapper::LoadTexture: %s", pFilename);
	return true;
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::SetShaderConstants

void CSpeedTreeWrapper::SetShaderConstants(const float* pMaterial) const
{
	if (_mgr)
		_mgr->GetCbMgr()->SetSpeedTreeMaterialConstants(pMaterial, m_pSpeedTree->GetLeafLightingAdjustment());
}

void CSpeedTreeWrapper::SetPosition(float x, float y, float z)
{
	m_afPos[0] = x;
	m_afPos[1] = y;
	m_afPos[2] = z;
	m_pSpeedTree->SetTreePosition(x, y, z);
	CGraphicObjectInstance::SetPosition(x, y, z);
}

bool CSpeedTreeWrapper::GetBoundingSphere(D3DXVECTOR3 & v3Center, float & fRadius)
{
	float fX, fY, fZ;
	
	fX = m_afBoundingBox[3] - m_afBoundingBox[0];
	fY = m_afBoundingBox[4] - m_afBoundingBox[1];
	fZ = m_afBoundingBox[5] - m_afBoundingBox[2];
	
	v3Center.x = 0.0f;
	v3Center.y = 0.0f;
	v3Center.z = fZ * 0.5f;
	
	fRadius = sqrtf(fX * fX + fY * fY + fZ * fZ) * 0.5f * 0.9f; // 0.9f for reduce size
	
	D3DXVECTOR3 vec = m_pSpeedTree->GetTreePosition();
	
	v3Center+=vec;
	
	return true;
}

void CSpeedTreeWrapper::CalculateBBox()
{
	float fX, fY, fZ;
	
	fX = m_afBoundingBox[3] - m_afBoundingBox[0];
	fY = m_afBoundingBox[4] - m_afBoundingBox[1];
	fZ = m_afBoundingBox[5] - m_afBoundingBox[2];
	
	m_v3BBoxMin.x = -fX / 2.0f;
	m_v3BBoxMin.y = -fY / 2.0f;
	m_v3BBoxMin.z = 0.0f;
	m_v3BBoxMax.x = fX / 2.0f;
	m_v3BBoxMax.y = fY / 2.0f;
	m_v3BBoxMax.z = fZ;
	
	m_v4TBBox[0] = D3DXVECTOR4(m_v3BBoxMin.x, m_v3BBoxMin.y, m_v3BBoxMin.z, 1.0f);
	m_v4TBBox[1] = D3DXVECTOR4(m_v3BBoxMin.x, m_v3BBoxMax.y, m_v3BBoxMin.z, 1.0f);
	m_v4TBBox[2] = D3DXVECTOR4(m_v3BBoxMax.x, m_v3BBoxMin.y, m_v3BBoxMin.z, 1.0f);
	m_v4TBBox[3] = D3DXVECTOR4(m_v3BBoxMax.x, m_v3BBoxMax.y, m_v3BBoxMin.z, 1.0f);
	m_v4TBBox[4] = D3DXVECTOR4(m_v3BBoxMin.x, m_v3BBoxMin.y, m_v3BBoxMax.z, 1.0f);
	m_v4TBBox[5] = D3DXVECTOR4(m_v3BBoxMin.x, m_v3BBoxMax.y, m_v3BBoxMax.z, 1.0f);
	m_v4TBBox[6] = D3DXVECTOR4(m_v3BBoxMax.x, m_v3BBoxMin.y, m_v3BBoxMax.z, 1.0f);
	m_v4TBBox[7] = D3DXVECTOR4(m_v3BBoxMax.x, m_v3BBoxMax.y, m_v3BBoxMax.z, 1.0f);
	
	const D3DXMATRIX & c_rmatTransform = GetTransform();
	
	for (DWORD i = 0; i < 8; ++i)
	{
		D3DXVec4Transform(&m_v4TBBox[i], &m_v4TBBox[i], &c_rmatTransform);
		if (0 == i)
		{
			m_v3TBBoxMin.x = m_v4TBBox[i].x;
			m_v3TBBoxMin.y = m_v4TBBox[i].y;
			m_v3TBBoxMin.z = m_v4TBBox[i].z;
			m_v3TBBoxMax.x = m_v4TBBox[i].x;
			m_v3TBBoxMax.y = m_v4TBBox[i].y;
			m_v3TBBoxMax.z = m_v4TBBox[i].z;
		}
		else
		{
			if (m_v3TBBoxMin.x > m_v4TBBox[i].x)
				m_v3TBBoxMin.x = m_v4TBBox[i].x;
			if (m_v3TBBoxMax.x < m_v4TBBox[i].x)
				m_v3TBBoxMax.x = m_v4TBBox[i].x;
			if (m_v3TBBoxMin.y > m_v4TBBox[i].y)
				m_v3TBBoxMin.y = m_v4TBBox[i].y;
			if (m_v3TBBoxMax.y < m_v4TBBox[i].y)
				m_v3TBBoxMax.y = m_v4TBBox[i].y;
			if (m_v3TBBoxMin.z > m_v4TBBox[i].z)
				m_v3TBBoxMin.z = m_v4TBBox[i].z;
			if (m_v3TBBoxMax.z < m_v4TBBox[i].z)
				m_v3TBBoxMax.z = m_v4TBBox[i].z;
		}
	}	
}

// collision detection routines
UINT CSpeedTreeWrapper::GetCollisionObjectCount()
{
	assert(m_pSpeedTree);
	return m_pSpeedTree->GetCollisionObjectCount();
}

void CSpeedTreeWrapper::GetCollisionObject(UINT nIndex, CSpeedTreeRT::ECollisionObjectType& eType, float* pPosition, float* pDimensions)
{
	assert(m_pSpeedTree);
	m_pSpeedTree->GetCollisionObject(nIndex, eType, pPosition, pDimensions);
}


const float * CSpeedTreeWrapper::GetPosition()
{
	return m_afPos;
}

void CSpeedTreeWrapper::GetTreeSize(float & r_fSize, float & r_fVariance)
{
	m_pSpeedTree->GetTreeSize(r_fSize, r_fVariance);
}

// pscdVector may be null
void CSpeedTreeWrapper::OnUpdateCollisionData(const CStaticCollisionDataVector * /*pscdVector*/)
{
	D3DXMATRIX mat;
	D3DXMatrixTranslation(&mat, m_afPos[0], m_afPos[1], m_afPos[2]);
	
	/////
	for (UINT i = 0; i < GetCollisionObjectCount(); ++i)
	{
		CSpeedTreeRT::ECollisionObjectType ObjectType;
		CStaticCollisionData CollisionData;
		
		GetCollisionObject(i, ObjectType, (float * )&CollisionData.v3Position, CollisionData.fDimensions);
		
		if (ObjectType == CSpeedTreeRT::CO_BOX)
			continue;
		
		switch(ObjectType)
		{
		case CSpeedTreeRT::CO_SPHERE:
			CollisionData.dwType = COLLISION_TYPE_SPHERE;
			CollisionData.fDimensions[0] = CollisionData.fDimensions[0] /** fSizeRatio*/;
			//AddCollision(&CollisionData);
			break;
			
		case CSpeedTreeRT::CO_CYLINDER:
			CollisionData.dwType = COLLISION_TYPE_CYLINDER;
			CollisionData.fDimensions[0] = CollisionData.fDimensions[0] /** fSizeRatio*/;
			CollisionData.fDimensions[1] = CollisionData.fDimensions[1] /** fSizeRatio*/;
			//AddCollision(&CollisionData);
			break;
			
			/*case CSpeedTreeRT::CO_BOX:
			break;*/
		}
		AddCollision(&CollisionData, &mat);
	}
}

