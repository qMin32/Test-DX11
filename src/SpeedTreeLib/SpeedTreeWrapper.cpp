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
#include <cmath>
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

	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG2, TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP, TOP11_MODULATE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG2, TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP, TOP11_MODULATE);

	DWORD dwLighting = STATEMANAGER.GetRenderState(RS11_LIGHTING);
	DWORD dwFogEnable = STATEMANAGER.GetRenderState(RS11_FOGENABLE);
	DWORD dwAlphaBlendEnable = STATEMANAGER.GetRenderState(RS11_ALPHABLENDENABLE);
	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);
	STATEMANAGER.SetRenderState(RS11_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_ALPHATESTENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_ZFUNC, D3D11_COMPARISON_LESS_EQUAL);
	STATEMANAGER.SaveRenderState(RS11_CULLMODE, D3D11_CULL_NONE);
	STATEMANAGER.SetRenderState(RS11_FOGENABLE, FALSE);

	// choose fixed function pipeline or custom shader for fronds and branches
	if (ms_pBranchShader) 
		ms_pBranchShader->Set();

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

		ID3D11ShaderResourceView* lpd3dTexture = m_BranchImageInstance.GetTextureReference().GetSRV();

		// set texture map
		if (lpd3dTexture)
			STATEMANAGER.SetTexture(0, lpd3dTexture);

		if (m_pGeometryCache->m_sBranches.m_nNumVertices > 0)
		{
			// activate the branch vertex buffer
			_mgr->SetVertexBuffer(m_pBranchVertexBuffer, sizeof(SFVFBranchVertex));
			// set the index buffer
			_mgr->SetIndexBuffer(m_pBranchIndexBuffer);
		}
	}

	STATEMANAGER.SetRenderState(RS11_ALPHATESTENABLE, FALSE);
	STATEMANAGER.SetRenderState(RS11_ALPHABLENDENABLE, FALSE);

	RenderBranches();

	STATEMANAGER.SetRenderState(RS11_ALPHATESTENABLE, TRUE);
	STATEMANAGER.SetRenderState(RS11_ALPHAREF, c_nDefaultAlphaTestValue);

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
			SFVFBranchVertex* pVertexBuffer = NULL;
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

		if (m_pGeometryCache->m_sFronds.m_nNumVertices > 0)
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
	RenderBillboards();

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

	m_pSpeedTree->SetLodLevel(1.0f);
	//Advance();

	CSpeedTreeForestDirectX::Instance().UpdateCompundMatrix(CCameraManager::Instance().GetCurrentCamera()->GetEye(), ms_matView, ms_matProj);

	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG2, TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP, TOP11_MODULATE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG2, TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP, TOP11_MODULATE);

	STATEMANAGER.SetSamplerState(1, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_WRAP);
	STATEMANAGER.SetSamplerState(1, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_WRAP);

	STATEMANAGER.SaveRenderState(RS11_LIGHTING, FALSE);
	STATEMANAGER.SaveRenderState(RS11_ALPHATESTENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_ZFUNC, D3D11_COMPARISON_LESS_EQUAL);
	STATEMANAGER.SaveRenderState(RS11_CULLMODE, D3D11_CULL_NONE);
	STATEMANAGER.SaveRenderState(RS11_FOGENABLE, FALSE);

	// choose fixed function pipeline or custom shader for fronds and branches
	if (ms_pBranchShader) 
		ms_pBranchShader->Set();

	STATEMANAGER.SetRenderState(RS11_ALPHATESTENABLE, FALSE);
	STATEMANAGER.SetRenderState(RS11_ALPHABLENDENABLE, FALSE);

	SetupBranchForTreeType();
	RenderBranches();

	STATEMANAGER.SetRenderState(RS11_ALPHATESTENABLE, TRUE);
	STATEMANAGER.SetRenderState(RS11_ALPHAREF, c_nDefaultAlphaTestValue);

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
	RenderBillboards();

	STATEMANAGER.RestoreRenderState(RS11_LIGHTING);
	STATEMANAGER.RestoreRenderState(RS11_ALPHATESTENABLE);
	STATEMANAGER.RestoreRenderState(RS11_ZFUNC);
	STATEMANAGER.RestoreRenderState(RS11_CULLMODE);
	STATEMANAGER.RestoreRenderState(RS11_FOGENABLE);
}

///////////////////////////////////////////////////////////////////////  
//	
CSpeedTreeWrapper::~CSpeedTreeWrapper()
{
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
			if (m_pLeafVertexBuffer[i])
				m_pLeafVertexBuffer[i].reset();
		}

		SAFE_DELETE_ARRAY(m_pLeavesUpdatedByCpu);
		SAFE_DELETE_ARRAY(m_pLeafVertexBuffer);

		SAFE_DELETE(m_pTextureInfo);
		SAFE_DELETE(m_pGeometryCache);
	}

	SAFE_DELETE(m_pSpeedTree);
	Clear();
}

///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::LoadTree
bool CSpeedTreeWrapper::LoadTree(const char* pszSptFile, const BYTE* c_pbBlock, unsigned int uiBlockSize, UINT nSeed, float fSize, float fSizeVariance)
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

	m_pSpeedTree->SetNumLeafRockingGroups(3);

	float fSafeSize = fSize;
	float fSafeVariance = fSizeVariance;

	if (!_finite(fSafeSize) || fSafeSize <= 0.0f)
		fSafeSize = 1000.0f;

	if (!_finite(fSafeVariance) || fSafeVariance < 0.0f)
		fSafeVariance = 0.0f;

	if (fSafeVariance >= fSafeSize)
		fSafeVariance = 0.0f;

	m_pSpeedTree->SetTreeSize(fSafeSize, fSafeVariance);

	UINT nSafeSeed = nSeed ? nSeed : 1;

	bool bComputed = m_pSpeedTree->Compute(NULL, nSafeSeed, false);

	if (!bComputed)
	{
		TraceError("SpeedTree skipped: %s", pszSptFile);
		return false;
	}
	if (bComputed)
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
		m_pTextureInfo = new CSpeedTreeRT::SMapBank;
		m_pSpeedTree->GetMapBank(*m_pTextureInfo);

		std::filesystem::path path = pszSptFile;
		path = path.parent_path();

		m_strBranchTextureName.clear();

		const char* branchMap = NULL;

		if (m_pTextureInfo && m_pTextureInfo->m_pBranchMaps)
			branchMap = m_pTextureInfo->m_pBranchMaps[CSpeedTreeRT::TL_DIFFUSE];

		if (!branchMap || IsBadStringPtrA(branchMap, MAX_PATH))
		{
			TraceError("SpeedTree branchMap NULL/INVALID");
			m_BranchImageInstance.SetImagePointer(NULL);
		}
		else
		{
			std::string sptPath = pszSptFile;
			CFileNameHelper::ChangeDosPath(sptPath);

			size_t slash = sptPath.find_last_of("/\\");
			std::string dir = (slash != std::string::npos) ? sptPath.substr(0, slash + 1) : "";

			std::string branchName = branchMap;
			CFileNameHelper::ChangeDosPath(branchName);

			size_t branchSlash = branchName.find_last_of("/\\");
			if (branchSlash != std::string::npos)
				branchName = branchName.substr(branchSlash + 1);

			std::string branchFile = dir + branchName;

			size_t dot = branchFile.find_last_of('.');
			if (dot != std::string::npos)
				branchFile = branchFile.substr(0, dot);

			branchFile += ".dds";

			m_strBranchTextureName = branchFile;

			TraceError("SpeedTree branch try: %s", m_strBranchTextureName.c_str());

			if (!LoadTexture(m_strBranchTextureName.c_str(), m_BranchImageInstance))
			{
				TraceError("SpeedTree branch load FAILED: %s", m_strBranchTextureName.c_str());
				m_BranchImageInstance.SetImagePointer(NULL);
			}
			else
			{
				TraceError("SpeedTree branch load OK: %s", m_strBranchTextureName.c_str());
			}
		}

#ifdef WRAPPER_RENDER_SELF_SHADOWS
		if (m_pTextureInfo->m_pSelfShadowMap != NULL)
		{
			auto selfShadowTexture = path / m_pTextureInfo->m_pSelfShadowMap;
			selfShadowTexture.replace_extension(".dds");

			if (!LoadTexture(selfShadowTexture.generic_string().c_str(), m_ShadowImageInstance))
				TraceError("SpeedTree self shadow failed: %s", selfShadowTexture.generic_string().c_str());
			else
				TraceError("SpeedTree self shadow OK: %s", selfShadowTexture.generic_string().c_str());
		}
#endif

		if (m_pTextureInfo->m_pCompositeMaps)
		{
			const char* compositeMap = m_pTextureInfo->m_pCompositeMaps[CSpeedTreeRT::TL_DIFFUSE];

			if (compositeMap && strlen(compositeMap) > 0)
			{
				TraceError("SpeedTree Composite DIFFUSE = %s", compositeMap);

				auto compositeTexture = path / compositeMap;
				compositeTexture.replace_extension(".dds");

				if (!LoadTexture(compositeTexture.generic_string().c_str(), m_CompositeImageInstance))
				{
					TraceError("SpeedTree composite diffuse failed: %s", compositeTexture.generic_string().c_str());
					m_CompositeImageInstance = CGraphicImageInstance();
				}
			}
			else
			{
				TraceError("SpeedTree Composite DIFFUSE missing");
				m_CompositeImageInstance = CGraphicImageInstance();
			}
		}
		else
		{
			TraceError("SpeedTree CompositeMaps missing");
			m_CompositeImageInstance = CGraphicImageInstance();
		}

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
	m_unBranchVertexCount = pBranches->m_nNumVertices;

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
		v.m_dwDiffuseColor = pBranches->m_pColors ? pBranches->m_pColors[i] : 0xffffffff;
#endif

		const float* pTexCoords = pBranches->m_pTexCoords[CSpeedTreeRT::TL_DIFFUSE];
		v.m_fTexCoords[0] = pTexCoords ? pTexCoords[i * 2] : 0.0f;
		v.m_fTexCoords[1] = pTexCoords ? pTexCoords[i * 2 + 1] : 0.0f;

#ifdef WRAPPER_RENDER_SELF_SHADOWS
		const float* pShadowCoords = pBranches->m_pTexCoords[CSpeedTreeRT::TL_SHADOW];
		v.m_fShadowCoords[0] = pShadowCoords ? pShadowCoords[i * 2] : 0.0f;
		v.m_fShadowCoords[1] = pShadowCoords ? pShadowCoords[i * 2 + 1] : 0.0f;
#endif

#ifdef WRAPPER_USE_GPU_WIND
		v.m_fWindIndex = pBranches->m_pWindMatrixIndices[0] ? 4.0f * pBranches->m_pWindMatrixIndices[0][i] : 0.0f;
		v.m_fWindWeight = pBranches->m_pWindWeights[0] ? pBranches->m_pWindWeights[0][i] : 0.0f;
#endif
	}

	_mgr->CreateVertexBuffer(m_pBranchVertexBuffer, vertices.data(), (UINT)vertices.size(), sizeof(SFVFBranchVertex),
#ifdef WRAPPER_USE_CPU_WIND
		true
#else
		false
#endif
	);

	const uint32_t unNumLodLevels = (uint32_t)pBranches->m_nNumLods;
	m_branchStripOffsets.clear();
	m_branchStripLengths.clear();

	if (unNumLodLevels == 0 || !pBranches->m_pNumStrips || !pBranches->m_pStripLengths || !pBranches->m_pStrips)
		return;

	m_branchStripLengths.resize(unNumLodLevels);

	const uint32_t stripCount = (uint32_t)pBranches->m_pNumStrips[0];
	uint32_t totalIndexCount = 0;

	m_branchStripOffsets.resize(stripCount);

	for (uint32_t s = 0; s < stripCount; ++s)
	{
		m_branchStripOffsets[s] = totalIndexCount;
		totalIndexCount += (uint32_t)pBranches->m_pStripLengths[0][s];
	}

	for (uint32_t lod = 0; lod < unNumLodLevels; ++lod)
	{
		auto& lengths = m_branchStripLengths[lod];
		lengths.assign(stripCount, 0);

		const uint32_t lodStripCount = (uint32_t)pBranches->m_pNumStrips[lod];

		for (uint32_t st = 0; st < stripCount && st < lodStripCount; ++st)
			lengths[st] = (uint16_t)pBranches->m_pStripLengths[lod][st];
	}

	if (totalIndexCount > 0)
	{
		std::vector<uint16_t> indices(totalIndexCount);
		uint32_t cursor = 0;

		for (uint32_t st = 0; st < stripCount; ++st)
		{
			const uint32_t length = (uint32_t)pBranches->m_pStripLengths[0][st];

			for (uint32_t i = 0; i < length; ++i)
				indices[cursor + i] = (uint16_t)pBranches->m_pStrips[0][st][i];

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
	m_unFrondVertexCount = pFronds->m_nNumVertices;

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
		v.m_dwDiffuseColor = pFronds->m_pColors ? pFronds->m_pColors[i] : 0xffffffff;
#endif

		const float* pTexCoords = pFronds->m_pTexCoords[CSpeedTreeRT::TL_DIFFUSE];
		v.m_fTexCoords[0] = pTexCoords ? pTexCoords[i * 2] : 0.0f;
		v.m_fTexCoords[1] = pTexCoords ? pTexCoords[i * 2 + 1] : 0.0f;

#ifdef WRAPPER_RENDER_SELF_SHADOWS
		const float* pShadowCoords = pFronds->m_pTexCoords[CSpeedTreeRT::TL_SHADOW];
		v.m_fShadowCoords[0] = pShadowCoords ? pShadowCoords[i * 2] : 0.0f;
		v.m_fShadowCoords[1] = pShadowCoords ? pShadowCoords[i * 2 + 1] : 0.0f;
#endif

#ifdef WRAPPER_USE_GPU_WIND
		v.m_fWindIndex = pFronds->m_pWindMatrixIndices[0] ? 4.0f * pFronds->m_pWindMatrixIndices[0][i] : 0.0f;
		v.m_fWindWeight = pFronds->m_pWindWeights[0] ? pFronds->m_pWindWeights[0][i] : 0.0f;
#endif
	}

	_mgr->CreateVertexBuffer(m_pFrondVertexBuffer, vertices.data(), (UINT)vertices.size(), sizeof(SFVFBranchVertex),
#ifdef WRAPPER_USE_CPU_WIND
		true
#else
		false
#endif
	);

	const uint32_t unNumLodLevels = (uint32_t)pFronds->m_nNumLods;
	m_frondStripOffsets.clear();
	m_frondStripLengths.clear();

	if (unNumLodLevels == 0 || !pFronds->m_pNumStrips || !pFronds->m_pStripLengths || !pFronds->m_pStrips)
		return;

	m_frondStripLengths.resize(unNumLodLevels);

	const uint32_t stripCount = (uint32_t)pFronds->m_pNumStrips[0];
	uint32_t totalIndexCount = 0;

	m_frondStripOffsets.resize(stripCount);

	for (uint32_t s = 0; s < stripCount; ++s)
	{
		m_frondStripOffsets[s] = totalIndexCount;
		totalIndexCount += (uint32_t)pFronds->m_pStripLengths[0][s];
	}

	for (uint32_t lod = 0; lod < unNumLodLevels; ++lod)
	{
		auto& lengths = m_frondStripLengths[lod];
		lengths.assign(stripCount, 0);

		const uint32_t lodStripCount = (uint32_t)pFronds->m_pNumStrips[lod];

		for (uint32_t st = 0; st < stripCount && st < lodStripCount; ++st)
			lengths[st] = (uint16_t)pFronds->m_pStripLengths[lod][st];
	}

	if (totalIndexCount > 0)
	{
		std::vector<uint16_t> indices(totalIndexCount);
		uint32_t cursor = 0;

		for (uint32_t st = 0; st < stripCount; ++st)
		{
			const uint32_t length = (uint32_t)pFronds->m_pStripLengths[0][st];

			for (uint32_t i = 0; i < length; ++i)
				indices[cursor + i] = (uint16_t)pFronds->m_pStrips[0][st][i];

			cursor += length;
		}

		_mgr->CreateIndexBuffer(m_pFrondIndexBuffer, indices.data(), (UINT)indices.size(), false, DXGI_FORMAT_R16_UINT);
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::SetupLeafBuffers

void CSpeedTreeWrapper::SetupLeafBuffers(void)
{
	if (!m_pGeometryCache || m_pGeometryCache->m_nNumLeafLods <= 0 || !m_pGeometryCache->m_pLeaves)
	{
		m_usNumLeafLods = 0;
		m_pLeafVertexBuffer = NULL;
		m_pLeavesUpdatedByCpu = NULL;
		return;
	}

	const short anVertexIndices[6] = { 0, 1, 2, 0, 2, 3 };

	m_usNumLeafLods = (unsigned short)m_pGeometryCache->m_nNumLeafLods;
	m_pLeafVertexBuffer = new VBufferPtr[m_usNumLeafLods];
	m_pLeavesUpdatedByCpu = new bool[m_usNumLeafLods];

	for (UINT unLod = 0; unLod < m_usNumLeafLods; ++unLod)
	{
		m_pLeavesUpdatedByCpu[unLod] = false;
		m_pLeafVertexBuffer[unLod].reset();

		const CSpeedTreeRT::SGeometry::SLeaf* pLeaf = &m_pGeometryCache->m_pLeaves[unLod];

		if (!pLeaf)
			continue;

		const int nLeafCount = pLeaf->m_nNumLeaves;

		if (nLeafCount < 1)
			continue;

		if (!_mgr || !pLeaf->m_pCards || !pLeaf->m_pLeafCardIndices || !pLeaf->m_pCenterCoords)
			continue;

		std::vector<SFVFLeafVertex> vertices(nLeafCount * 12);
		SFVFLeafVertex* pVertex = vertices.data();

		for (int nLeaf = 0; nLeaf < nLeafCount; ++nLeaf)
		{
			const float* center = &pLeaf->m_pCenterCoords[nLeaf * 3];

			const int cardIndex = pLeaf->m_pLeafCardIndices[nLeaf];

			if (cardIndex < 0)
				continue;

			const CSpeedTreeRT::SGeometry::SLeaf::SCard& card = pLeaf->m_pCards[cardIndex];

			for (int side = 0; side < 2; ++side)
			{
				for (UINT unVert = 0; unVert < 6; ++unVert)
				{
					const int corner = anVertexIndices[unVert];

					pVertex->m_vPosition.x = center[0];
					pVertex->m_vPosition.y = center[1];
					pVertex->m_vPosition.z = center[2];

#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
					if (pLeaf->m_pNormals)
						memcpy(&pVertex->m_vNormal, &(pLeaf->m_pNormals[(nLeaf * 4 + corner) * 3]), 3 * sizeof(float));
					else
						pVertex->m_vNormal = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
#else
					pVertex->m_dwDiffuseColor = pLeaf->m_pColors ? pLeaf->m_pColors[nLeaf * 4 + corner] : 0xffffffff;
#endif

					pVertex->m_fTexCoords[0] = card.m_pTexCoords ? card.m_pTexCoords[corner * 2] : 0.0f;
					pVertex->m_fTexCoords[1] = card.m_pTexCoords ? card.m_pTexCoords[corner * 2 + 1] : 0.0f;

					pVertex->m_fWindIndex = float(side);
					pVertex->m_fWindWeight = 0.0f;
					pVertex->m_fLeafPlacementIndex = float(cardIndex * 4 + corner);
					pVertex->m_fLeafScalarValue = 1.0f;

					++pVertex;
				}
			}
		}

		if (pVertex == vertices.data())
			continue;

		vertices.resize((size_t)(pVertex - vertices.data()));

		_mgr->CreateVertexBuffer(m_pLeafVertexBuffer[unLod], vertices.data(), (UINT)vertices.size(), sizeof(SFVFLeafVertex));
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
	if (!m_pSpeedTree)
		return SpeedTreeWrapperPtr();

	auto spInstance = std::make_shared<CSpeedTreeWrapper>();

	spInstance->m_bIsInstance = true;

	SAFE_DELETE(spInstance->m_pSpeedTree);

	spInstance->m_pSpeedTree = m_pSpeedTree->MakeInstance();

	if (!spInstance->m_pSpeedTree)
	{
		TraceError("SpeedTree MakeInstance failed");
		spInstance.reset();
		return SpeedTreeWrapperPtr();
	}

	spInstance->m_cBranchMaterial = m_cBranchMaterial;
	spInstance->m_cLeafMaterial = m_cLeafMaterial;
	spInstance->m_cFrondMaterial = m_cFrondMaterial;

	spInstance->m_CompositeImageInstance.SetImagePointer(m_CompositeImageInstance.GetGraphicImagePointer());
	spInstance->m_BranchImageInstance.SetImagePointer(m_BranchImageInstance.GetGraphicImagePointer());

	if (!m_ShadowImageInstance.IsEmpty())
		spInstance->m_ShadowImageInstance.SetImagePointer(m_ShadowImageInstance.GetGraphicImagePointer());

	spInstance->m_pTextureInfo = m_pTextureInfo;
	spInstance->m_pGeometryCache = m_pGeometryCache;

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

	memcpy(spInstance->m_afPos, m_afPos, 3 * sizeof(float));
	memcpy(spInstance->m_afBoundingBox, m_afBoundingBox, 6 * sizeof(float));

	spInstance->m_pInstanceOf = shared_from_this();
	m_vInstances.push_back(spInstance);

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

	ID3D11ShaderResourceView* lpd3dTexture = nullptr;

	if (!m_BranchImageInstance.IsEmpty())
		lpd3dTexture = m_BranchImageInstance.GetTextureReference().GetSRV();

	if (!lpd3dTexture)
	{
		TraceError("SpeedTree branch SRV NULL: %s",
			m_strBranchTextureName.empty() ? "unknown" : m_strBranchTextureName.c_str());
	}

	STATEMANAGER.SetTexture(0, lpd3dTexture);

	// bind shadow texture
#ifdef WRAPPER_RENDER_SELF_SHADOWS
	if (ms_bSelfShadowOn && (lpd3dTexture = m_ShadowImageInstance.GetTextureReference().GetSRV()))
		STATEMANAGER.SetTexture(1, NULL);
	else
		STATEMANAGER.SetTexture(1, NULL);
#endif

	if (m_pGeometryCache->m_sBranches.m_nNumVertices > 0)
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

	const int lod = 0;

	if (m_pGeometryCache->m_sBranches.m_nNumVertices <= 0 || !m_pBranchIndexBuffer ||
		m_branchStripLengths.empty() || m_branchStripOffsets.empty() ||
		static_cast<size_t>(lod) >= m_branchStripLengths.size())
		return;

	PositionTree();
	STATEMANAGER.SetRenderState(RS11_ALPHAREF, c_nDefaultAlphaTestValue);

	const auto& lengths = m_branchStripLengths[lod];
	const size_t stripCount = lengths.size() < m_branchStripOffsets.size() ? lengths.size() : m_branchStripOffsets.size();

	for (size_t s = 0; s < stripCount; ++s)
	{
		const uint16_t stripLength = lengths[s];

		if (stripLength > 2)
		{
			ms_faceCount += stripLength - 2;
			STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, m_pGeometryCache->m_sBranches.m_nNumVertices, m_branchStripOffsets[s], stripLength - 2);
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::SetupFrondForTreeType

void CSpeedTreeWrapper::SetupFrondForTreeType(void) const
{
#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
	STATEMANAGER.SetMaterial(m_cFrondMaterial.Get());
	SetShaderConstants(m_pSpeedTree->GetFrondMaterial());
#endif

#ifdef WRAPPER_USE_CPU_WIND
	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_FrondGeometry);

	if (m_pGeometryCache->m_sFronds.m_pNumStrips && m_pGeometryCache->m_sFronds.m_pNumStrips[0] > 0)
	{
		SFVFBranchVertex* pVertexBuffer = NULL;
		m_pFrondVertexBuffer->Lock(0, 0, reinterpret_cast<BYTE**>(&pVertexBuffer), D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);

		for (UINT i = 0; i < m_unFrondVertexCount; ++i)
			memcpy(&(pVertexBuffer[i].m_vPosition), &(m_pGeometryCache->m_sFronds.m_pCoords[i * 3]), 3 * sizeof(float));

		m_pFrondVertexBuffer->Unlock();
	}
#endif

	if (!m_CompositeImageInstance.IsEmpty())
		STATEMANAGER.SetTexture(0, m_CompositeImageInstance.GetTextureReference().GetSRV());

#ifdef WRAPPER_RENDER_SELF_SHADOWS
	ID3D11ShaderResourceView* lpd3dTexture;

	if ((lpd3dTexture = m_ShadowImageInstance.GetTextureReference().GetSRV()))
		STATEMANAGER.SetTexture(1, NULL);
#endif

	if (m_pGeometryCache->m_sFronds.m_nNumVertices > 0)
	{
		_mgr->SetVertexBuffer(m_pFrondVertexBuffer, sizeof(SFVFBranchVertex));
		_mgr->SetIndexBuffer(m_pFrondIndexBuffer);
	}
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::RenderFronds

void CSpeedTreeWrapper::RenderFronds(void) const
{
	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_FrondGeometry);

	const int lod = 0;

	if (m_pGeometryCache->m_sFronds.m_nNumVertices <= 0 || !m_pFrondIndexBuffer ||
		m_frondStripLengths.empty() || m_frondStripOffsets.empty() ||
		static_cast<size_t>(lod) >= m_frondStripLengths.size())
		return;

	PositionTree();
	STATEMANAGER.SetRenderState(RS11_ALPHAREF, c_nDefaultAlphaTestValue);

	const auto& lengths = m_frondStripLengths[lod];
	const size_t stripCount = lengths.size() < m_frondStripOffsets.size() ? lengths.size() : m_frondStripOffsets.size();

	for (size_t s = 0; s < stripCount; ++s)
	{
		const uint16_t stripLength = lengths[s];

		if (stripLength > 2)
		{
			ms_faceCount += stripLength - 2;
			STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, m_pGeometryCache->m_sFronds.m_nNumVertices, m_frondStripOffsets[s], stripLength - 2);
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
	UINT uiEntryCount = 0;
	const float* pTable = m_pSpeedTree->GetLeafBillboardTable(uiEntryCount);

	if (_mgr)
		_mgr->GetCbMgr()->SetSpeedTreeLeafTables(pTable, uiEntryCount / 4);
}
#endif


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::RenderLeaves

void CSpeedTreeWrapper::RenderLeaves(void) const
{
	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_LeafGeometry);

#ifdef WRAPPER_USE_GPU_LEAF_PLACEMENT
	UploadLeafTables(c_nVertexShader_LeafTables);
#endif

	if (!m_pLeafVertexBuffer || m_usNumLeafLods == 0 || !m_pGeometryCache->m_pLeaves)
		return;

	const int unLod = 0;

	if (unLod >= static_cast<int>(m_usNumLeafLods))
		return;

	const CSpeedTreeRT::SGeometry::SLeaf* pLeaf = &m_pGeometryCache->m_pLeaves[unLod];

	if (pLeaf->m_nNumLeaves <= 0 || !m_pLeafVertexBuffer[unLod])
		return;

	PositionTree();

	_mgr->SetVertexBuffer(m_pLeafVertexBuffer[unLod], sizeof(SFVFLeafVertex));
	STATEMANAGER.SetRenderState(RS11_ALPHAREF, c_nDefaultAlphaTestValue);

	ms_faceCount += pLeaf->m_nNumLeaves * 4;
	STATEMANAGER.DrawPrimitive(D3DPT_TRIANGLELIST, 0, pLeaf->m_nNumLeaves * 4);
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
#ifdef WRAPPER_BILLBOARD_MODE
	if (!m_CompositeImageInstance.IsEmpty())
		STATEMANAGER.SetTexture(0, m_CompositeImageInstance.GetTextureReference().GetSRV());

	PositionTree();

	m_pSpeedTree->GetGeometry(*m_pGeometryCache, SpeedTree_BillboardGeometry);
	m_pSpeedTree->UpdateBillboardGeometry(*m_pGeometryCache);

	struct SBillboardVertex
	{
		float fX, fY, fZ;
		float fU, fV;
	};

	const CSpeedTreeRT::SGeometry::S360Billboard& bb = m_pGeometryCache->m_s360Billboard;

	for (int pass = 0; pass < 2; ++pass)
	{
		if (!bb.m_pCoords || !bb.m_pTexCoords[pass])
			continue;

		const float* pCoords = bb.m_pCoords;
		const float* pTexCoords = bb.m_pTexCoords[pass];

		SBillboardVertex sVertex[4] =
		{
			{ pCoords[0], pCoords[1], pCoords[2], pTexCoords[0], pTexCoords[1] },
			{ pCoords[3], pCoords[4], pCoords[5], pTexCoords[2], pTexCoords[3] },
			{ pCoords[6], pCoords[7], pCoords[8], pTexCoords[4], pTexCoords[5] },
			{ pCoords[9], pCoords[10], pCoords[11], pTexCoords[6], pTexCoords[7] },
		};

		STATEMANAGER.SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
		STATEMANAGER.SetRenderState(RS11_ALPHAREF, DWORD(bb.m_afAlphaTestValues[pass]));

		ms_faceCount += 2;
		STATEMANAGER.DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, sVertex, sizeof(SBillboardVertex));
	}

#ifdef WRAPPER_RENDER_HORIZONTAL_BILLBOARD
	const CSpeedTreeRT::SGeometry::SHorzBillboard& hb = m_pGeometryCache->m_sHorzBillboard;

	if (hb.m_pCoords && hb.m_pTexCoords)
	{
		const float* pCoords = hb.m_pCoords;
		const float* pTexCoords = hb.m_pTexCoords;

		SBillboardVertex sVertex[4] =
		{
			{ pCoords[0], pCoords[1], pCoords[2], pTexCoords[0], pTexCoords[1] },
			{ pCoords[3], pCoords[4], pCoords[5], pTexCoords[2], pTexCoords[3] },
			{ pCoords[6], pCoords[7], pCoords[8], pTexCoords[4], pTexCoords[5] },
			{ pCoords[9], pCoords[10], pCoords[11], pTexCoords[6], pTexCoords[7] },
		};

		STATEMANAGER.SetRenderState(RS11_ALPHAREF, DWORD(hb.m_fAlphaTestValue));

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
	const float* pPosition = m_pSpeedTree->GetTreePosition();
	D3DXVECTOR3 vecPosition(pPosition[0], pPosition[1], pPosition[2]);

	D3DXMATRIX matTranslation;
	D3DXMatrixIdentity(&matTranslation);
	D3DXMatrixTranslation(&matTranslation, vecPosition.x, vecPosition.y, vecPosition.z);

	STATEMANAGER.SetTransform(World, &matTranslation);

	D3DXVECTOR4 vecConstant(vecPosition.x, vecPosition.y, vecPosition.z, 0.0f);

	if (_mgr)
		_mgr->GetCbMgr()->SetSpeedTreeTreePosition(vecConstant);
}


///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeWrapper::LoadTexture

bool CSpeedTreeWrapper::LoadTexture(const char* pFilename, CGraphicImageInstance& rImage)
{
	TraceError("SpeedTree LoadTexture try: %s", pFilename);

	CResource* pResource = CResourceManager::Instance().GetResourcePointer(pFilename);

	if (!pResource)
	{
		TraceError("SpeedTree LoadTexture resource NULL: %s", pFilename);
		rImage.SetImagePointer(NULL);
		return false;
	}

	CGraphicImage* pImage = static_cast<CGraphicImage*>(pResource);
	rImage.SetImagePointer(pImage);

	if (rImage.IsEmpty())
	{
		TraceError("SpeedTree LoadTexture image empty: %s", pFilename);
		rImage.SetImagePointer(NULL);
		return false;
	}

	TraceError("SpeedTree LoadTexture OK: %s", pFilename);
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

bool CSpeedTreeWrapper::GetBoundingSphere(D3DXVECTOR3& v3Center, float& fRadius)
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

	v3Center += vec;

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

	const D3DXMATRIX& c_rmatTransform = GetTransform();

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
	return (UINT)m_pSpeedTree->GetNumCollisionObjects();
}

void CSpeedTreeWrapper::GetCollisionObject(UINT nIndex, CSpeedTreeRT::ECollisionObjectType& eType, float* pPosition, float* pDimensions)
{
	assert(m_pSpeedTree);
	float euler[3] = { 0.0f, 0.0f, 0.0f };
	m_pSpeedTree->GetCollisionObject(nIndex, eType, pPosition, pDimensions, euler);
}


const float* CSpeedTreeWrapper::GetPosition()
{
	return m_afPos;
}

void CSpeedTreeWrapper::GetTreeSize(float& r_fSize, float& r_fVariance)
{
	m_pSpeedTree->GetTreeSize(r_fSize, r_fVariance);
}

// pscdVector may be null
void CSpeedTreeWrapper::OnUpdateCollisionData(const CStaticCollisionDataVector* /*pscdVector*/)
{
	D3DXMATRIX mat;
	D3DXMatrixTranslation(&mat, m_afPos[0], m_afPos[1], m_afPos[2]);

	/////
	for (UINT i = 0; i < GetCollisionObjectCount(); ++i)
	{
		CSpeedTreeRT::ECollisionObjectType ObjectType;
		CStaticCollisionData CollisionData;

		GetCollisionObject(i, ObjectType, (float*)&CollisionData.v3Position, CollisionData.fDimensions);

		if (ObjectType == CSpeedTreeRT::CO_BOX)
			continue;

		switch (ObjectType)
		{
		case CSpeedTreeRT::CO_SPHERE:
			CollisionData.dwType = COLLISION_TYPE_SPHERE;
			CollisionData.fDimensions[0] = CollisionData.fDimensions[0] /** fSizeRatio*/;
			//AddCollision(&CollisionData);
			break;

		case CSpeedTreeRT::CO_CAPSULE:
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

