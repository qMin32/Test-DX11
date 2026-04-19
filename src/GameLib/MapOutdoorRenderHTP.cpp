#include "StdAfx.h"
#include "MapOutdoor.h"

#include "EterLib/StateManager.h"

void CMapOutdoor::__RenderTerrain_RenderHardwareTransformPatch()
{
	DWORD dwFogColor;
	float fFogFarDistance;
	float fFogNearDistance;
	if (mc_pEnvironmentData)
	{
		dwFogColor=mc_pEnvironmentData->FogColor;
		fFogNearDistance=mc_pEnvironmentData->GetFogNearDistance();
		fFogFarDistance=mc_pEnvironmentData->GetFogFarDistance();
	}
	else
	{
		dwFogColor=0xffffffff;
		fFogNearDistance=5000.0f;
		fFogFarDistance=10000.0f;
	}
	
	//////////////////////////////////////////////////////////////////////////
	// Render State & TextureStageState	

	STATEMANAGER.SaveTextureStageState(0, TSS11_TEXCOORDINDEX, TSS11_TCI_CAMERASPACEPOSITION);
	STATEMANAGER.SaveTextureStageState(0, TSS11_TEXTURETRANSFORMFLAGS, TTFF11_COUNT2);
	STATEMANAGER.SaveTextureStageState(1, TSS11_TEXCOORDINDEX, TSS11_TCI_CAMERASPACEPOSITION);
	STATEMANAGER.SaveTextureStageState(1, TSS11_TEXTURETRANSFORMFLAGS, TTFF11_COUNT2);

	STATEMANAGER.SaveRenderState(RS11_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_ALPHATESTENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_ALPHAREF, 0x00000000);

	STATEMANAGER.SaveRenderState(RS11_TEXTUREFACTOR, dwFogColor);

	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG2, TA11_CURRENT);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP,   TOP11_MODULATE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP,   TOP11_SELECTARG1);
	STATEMANAGER.SetSamplerState(0, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_WRAP);
	STATEMANAGER.SetSamplerState(0, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_WRAP);

	STATEMANAGER.SetTextureStageState(1, TSS11_COLORARG1, TA11_CURRENT);
	STATEMANAGER.SetTextureStageState(1, TSS11_COLOROP,   TOP11_SELECTARG1);
	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP,   TOP11_SELECTARG1);
	STATEMANAGER.SetSamplerState(1, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_CLAMP);
	STATEMANAGER.SetSamplerState(1, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_CLAMP);

	CSpeedTreeWrapper::ms_bSelfShadowOn = true;
	STATEMANAGER.SetBestFiltering(0);
	STATEMANAGER.SetBestFiltering(1);

	m_matWorldForCommonUse._41 = 0.0f;
	m_matWorldForCommonUse._42 = 0.0f;
	STATEMANAGER.SetTransform(World, &m_matWorldForCommonUse);

	STATEMANAGER.SaveTransform(Texture0, &m_matWorldForCommonUse);
	STATEMANAGER.SaveTransform(Texture1, &m_matWorldForCommonUse);

	// Render State & TextureStageState
	//////////////////////////////////////////////////////////////////////////

	m_iRenderedSplatNumSqSum = 0;
	m_iRenderedPatchNum = 0;
	m_iRenderedSplatNum = 0;
	m_RenderedTextureNumVector.clear();

	std::pair<float, long> fog_far(fFogFarDistance+1600.0f, 0);
	std::pair<float, long> fog_near(fFogNearDistance-3200.0f, 0);

	if (mc_pEnvironmentData && mc_pEnvironmentData->bDensityFog)
		fog_far.first = 1e10f;

	std::vector<std::pair<float ,long> >::iterator far_it = std::upper_bound(m_PatchVector.begin(),m_PatchVector.end(),fog_far);
	std::vector<std::pair<float ,long> >::iterator near_it = std::upper_bound(m_PatchVector.begin(),m_PatchVector.end(),fog_near);

	WORD wPrimitiveCount;
	D3DPRIMITIVETYPE ePrimitiveType;

	BYTE byCUrrentLODLevel = 0;

	float fLODLevel1Distance = __GetNoFogDistance();
	float fLODLevel2Distance = __GetFogDistance();

	SelectIndexBuffer(0, &wPrimitiveCount, &ePrimitiveType);

	std::vector<std::pair<float, long> >::iterator it = m_PatchVector.begin();

	for(; it != near_it; ++it)
	{
		if (byCUrrentLODLevel == 0 && fLODLevel1Distance <= it->first)
		{
			byCUrrentLODLevel = 1;
			SelectIndexBuffer(1, &wPrimitiveCount, &ePrimitiveType);
		}
		else if (byCUrrentLODLevel == 1 && fLODLevel2Distance <= it->first)
		{
			byCUrrentLODLevel = 2;
			SelectIndexBuffer(2, &wPrimitiveCount, &ePrimitiveType);
		}
		
		__HardwareTransformPatch_RenderPatchSplat(it->second, wPrimitiveCount, ePrimitiveType);

		if (m_iRenderedSplatNum >= m_iSplatLimit)
			break;
		
 		if (m_bDrawWireFrame)
			DrawWireFrame(it->second, wPrimitiveCount, ePrimitiveType);
	}

	if (m_iRenderedSplatNum < m_iSplatLimit)
	{
		for(it = near_it; it != far_it; ++it)
		{
			if (byCUrrentLODLevel == 0 && fLODLevel1Distance <= it->first)
			{
				byCUrrentLODLevel = 1;
				SelectIndexBuffer(1, &wPrimitiveCount, &ePrimitiveType);
			}
			else if (byCUrrentLODLevel == 1 && fLODLevel2Distance <= it->first)
			{
				byCUrrentLODLevel = 2;
				SelectIndexBuffer(2, &wPrimitiveCount, &ePrimitiveType);
			}

			__HardwareTransformPatch_RenderPatchSplat(it->second, wPrimitiveCount, ePrimitiveType);

			if (m_iRenderedSplatNum >= m_iSplatLimit)
				break;

			if (m_bDrawWireFrame)
				DrawWireFrame(it->second, wPrimitiveCount, ePrimitiveType);
		}
	}

	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTextureStageState(0, TSS11_TEXTURETRANSFORMFLAGS, FALSE);

	STATEMANAGER.SetTexture(1, NULL);
	STATEMANAGER.SetTextureStageState(1, TSS11_TEXTURETRANSFORMFLAGS, FALSE);

	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TFACTOR);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP,   TOP11_SELECTARG1);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP,   TOP11_DISABLE);

	STATEMANAGER.SetTextureStageState(1, TSS11_COLOROP,   TOP11_DISABLE);
	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP,   TOP11_DISABLE);	

	if (m_iRenderedSplatNum < m_iSplatLimit)
	{
		for(it = far_it; it != m_PatchVector.end(); ++it)
		{
			if (byCUrrentLODLevel == 0 && fLODLevel1Distance <= it->first)
			{
				byCUrrentLODLevel = 1;
				SelectIndexBuffer(1, &wPrimitiveCount, &ePrimitiveType);
			}
			else if (byCUrrentLODLevel == 1 && fLODLevel2Distance <= it->first)
			{
				byCUrrentLODLevel = 2;
				SelectIndexBuffer(2, &wPrimitiveCount, &ePrimitiveType);
			}

			__HardwareTransformPatch_RenderPatchNone(it->second, wPrimitiveCount, ePrimitiveType);

			if (m_iRenderedSplatNum >= m_iSplatLimit)
				break;

			if (m_bDrawWireFrame)
 				DrawWireFrame(it->second, wPrimitiveCount, ePrimitiveType);
		}
	}

	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG2, TA11_CURRENT);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP,   TOP11_MODULATE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP,   TOP11_SELECTARG1);

	STATEMANAGER.SetTextureStageState(1, TSS11_COLORARG1, TA11_CURRENT);
	STATEMANAGER.SetTextureStageState(1, TSS11_COLOROP,   TOP11_SELECTARG1);
	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP,   TOP11_SELECTARG1);

	STATEMANAGER.SetRenderState(RS11_LIGHTING, TRUE);

	std::sort(m_RenderedTextureNumVector.begin(),m_RenderedTextureNumVector.end());

	STATEMANAGER.RestoreRenderState(RS11_TEXTUREFACTOR);

	STATEMANAGER.RestoreTransform(Texture0);
	STATEMANAGER.RestoreTransform(Texture1);

	STATEMANAGER.RestoreTextureStageState(0, TSS11_TEXCOORDINDEX);
	STATEMANAGER.RestoreTextureStageState(0, TSS11_TEXTURETRANSFORMFLAGS);
	STATEMANAGER.RestoreTextureStageState(1, TSS11_TEXCOORDINDEX);
	STATEMANAGER.RestoreTextureStageState(1, TSS11_TEXTURETRANSFORMFLAGS);

	STATEMANAGER.RestoreRenderState(RS11_ALPHABLENDENABLE);
	STATEMANAGER.RestoreRenderState(RS11_ALPHATESTENABLE);
	STATEMANAGER.RestoreRenderState(RS11_ALPHAREF);
}

void CMapOutdoor::__HardwareTransformPatch_RenderPatchSplat(long patchnum, WORD wPrimitiveCount, D3DPRIMITIVETYPE ePrimitiveType)
{
	assert(NULL!=m_pTerrainPatchProxyList && "__HardwareTransformPatch_RenderPatchSplat");
	CTerrainPatchProxy * pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];
	
	if (!pTerrainPatchProxy->isUsed())
		return;

	long sPatchNum = pTerrainPatchProxy->GetPatchNum();
	if (sPatchNum < 0)
		return;

	BYTE ucTerrainNum = pTerrainPatchProxy->GetTerrainNum();
	if (0xFF == ucTerrainNum)
		return;

	CTerrain * pTerrain;
	if (!GetTerrainPointer(ucTerrainNum, &pTerrain))
		return;

	DWORD dwFogColor;
	if (mc_pEnvironmentData)
		dwFogColor=mc_pEnvironmentData->FogColor;
	else
		dwFogColor=0xffffffff;

	WORD wCoordX, wCoordY;
	pTerrain->GetCoordinate(&wCoordX, &wCoordY);

	TTerrainSplatPatch & rTerrainSplatPatch = pTerrain->GetTerrainSplatPatch();
	
	D3DXMATRIX matTexTransform, matSplatAlphaTexTransform, matSplatColorTexTransform;
	m_matWorldForCommonUse._41 = -(float) (wCoordX * CTerrainImpl::TERRAIN_XSIZE);
	m_matWorldForCommonUse._42 = (float) (wCoordY * CTerrainImpl::TERRAIN_YSIZE);
	D3DXMatrixMultiply(&matTexTransform, &m_matViewInverse, &m_matWorldForCommonUse);
	D3DXMatrixMultiply(&matSplatAlphaTexTransform, &matTexTransform, &m_matSplatAlpha);
	STATEMANAGER.SetTransform(Texture1, &matSplatAlphaTexTransform);

	D3DXMATRIX matTiling;
	D3DXMatrixScaling(&matTiling, 1.0f/640.0f, -1.0f/640.0f, 0.0f);
	matTiling._41=0.0f;
	matTiling._42=0.0f;
	
	D3DXMatrixMultiply(&matSplatColorTexTransform, &m_matViewInverse, &matTiling);
	STATEMANAGER.SetTransform(Texture0, &matSplatColorTexTransform);
					
	auto pkVB = pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr();
	if (!pkVB)
		return;

	_mgr->SetShader(VF_PN);
	_mgr->SetVertexBuffer(pkVB);
	
	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);

	int iPrevRenderedSplatNum=m_iRenderedSplatNum;
	bool isFirst=true;
	for (DWORD j = 1; j < pTerrain->GetNumTextures(); ++j)
	{
		TTerainSplat & rSplat = rTerrainSplatPatch.Splats[j];
		
		if (!rSplat.Active)
			continue;
		
		if (rTerrainSplatPatch.PatchTileCount[sPatchNum][j] == 0)
			continue;
		
		const TTerrainTexture & rTexture = m_TextureSet.GetTexture(j);
		
		D3DXMatrixMultiply(&matSplatColorTexTransform, &m_matViewInverse, &rTexture.m_matTransform);
		STATEMANAGER.SetTransform(Texture0, &matSplatColorTexTransform);
		if (isFirst)
		{
			STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP,   TOP11_DISABLE);
			STATEMANAGER.SetTexture(0, rTexture.pd3dTexture);
			STATEMANAGER.SetTexture(1, rSplat.pd3dTexture);
			STATEMANAGER.DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);
			STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP,   TOP11_SELECTARG1);
			isFirst=false;
		}
		else
		{
			STATEMANAGER.SetTexture(0, rTexture.pd3dTexture);
			STATEMANAGER.SetTexture(1, rSplat.pd3dTexture);
			STATEMANAGER.DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);			
		}

		std::vector<int>::iterator aIterator = std::find(m_RenderedTextureNumVector.begin(), m_RenderedTextureNumVector.end(), (int)j);
		if (aIterator == m_RenderedTextureNumVector.end())
			m_RenderedTextureNumVector.push_back(j);
		++m_iRenderedSplatNum;
		if (m_iRenderedSplatNum >= m_iSplatLimit)
			break;
		
	}

/*
	if (GetAsyncKeyState(VK_CAPITAL) & 0x8000)
	{
		TTerainSplat & rSplat = rTerrainSplatPatch.Splats[200];
		
		if (rSplat.Active)
		{
			const TTerrainTexture & rTexture = m_TextureSet.GetTexture(1);
			
			D3DXMatrixMultiply(&matSplatColorTexTransform, &m_matViewInverse, &rTexture.m_matTransform);
			STATEMANAGER.SetTransform(Texture0, &matSplatColorTexTransform);
			
			STATEMANAGER.SetTexture(0, NULL);
			STATEMANAGER.SetTexture(1, rSplat.pd3dTexture);
			STATEMANAGER.DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);
		}
	}
*/

	// 그림자
	if (m_bDrawShadow)
	{
		STATEMANAGER.SetRenderState(RS11_LIGHTING, TRUE);
		
		STATEMANAGER.SetRenderState(RS11_FOGCOLOR, 0xFFFFFFFF);
		STATEMANAGER.SetRenderState(RS11_SRCBLEND, D3D11_BLEND_ZERO);
		STATEMANAGER.SetRenderState(RS11_DESTBLEND, D3D11_BLEND_SRC_COLOR);
				
		D3DXMATRIX matShadowTexTransform;
		D3DXMatrixMultiply(&matShadowTexTransform, &matTexTransform, &m_matStaticShadow);

		STATEMANAGER.SetTransform(Texture0, &matShadowTexTransform);
 		STATEMANAGER.SetTexture(0, pTerrain->GetShadowTexture());		
		
		STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TEXTURE);
		STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG2, TA11_CURRENT);
		STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP,   TOP11_MODULATE);
		STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1, TA11_TEXTURE);
		STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG2, TA11_CURRENT);
		STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP,   TOP11_DISABLE);
		STATEMANAGER.SetSamplerState(0, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_CLAMP);
		STATEMANAGER.SetSamplerState(0, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_CLAMP);
		if (m_bDrawChrShadow)
		{
			STATEMANAGER.SetTransform(Texture1, &m_matDynamicShadow);

 			STATEMANAGER.SetTexture(1, m_lpCharacterShadowMapTexture);
			STATEMANAGER.SetTextureStageState(1, TSS11_COLORARG1, TA11_TEXTURE);
			STATEMANAGER.SetTextureStageState(1, TSS11_COLORARG2, TA11_CURRENT);
			STATEMANAGER.SetTextureStageState(1, TSS11_COLOROP,   TOP11_MODULATE);
			STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP,   TOP11_DISABLE);
			STATEMANAGER.SetSamplerState(1, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_CLAMP);
			STATEMANAGER.SetSamplerState(1, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_CLAMP);
		}		
		else
		{
			STATEMANAGER.SetTexture(1, NULL);			
		}
		
		ms_faceCount += wPrimitiveCount;
		STATEMANAGER.DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);
  		++m_iRenderedSplatNum;

		if (m_bDrawChrShadow)
		{
			STATEMANAGER.SetTextureStageState(1, TSS11_COLORARG1, TA11_CURRENT);
			STATEMANAGER.SetTextureStageState(1, TSS11_COLOROP,   TOP11_SELECTARG1);
			STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAARG1, TA11_TEXTURE);
			STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP,   TOP11_SELECTARG1);
		}			

 		STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TEXTURE);
		STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG2, TA11_CURRENT);
		STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP,   TOP11_MODULATE);
		STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1, TA11_TEXTURE);
		STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP,   TOP11_SELECTARG1);
		STATEMANAGER.SetSamplerState(0, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_WRAP);
		STATEMANAGER.SetSamplerState(0, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_WRAP);
		
		
		STATEMANAGER.SetRenderState(RS11_SRCBLEND, D3D11_BLEND_SRC_ALPHA);
		STATEMANAGER.SetRenderState(RS11_DESTBLEND, D3D11_BLEND_INV_SRC_ALPHA);
		STATEMANAGER.SetRenderState(RS11_FOGCOLOR, dwFogColor);

		STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);
	}
	++m_iRenderedPatchNum;

	int iCurRenderedSplatNum=m_iRenderedSplatNum-iPrevRenderedSplatNum;

	m_iRenderedSplatNumSqSum+=iCurRenderedSplatNum*iCurRenderedSplatNum;

}

void CMapOutdoor::__HardwareTransformPatch_RenderPatchNone(long patchnum, WORD wPrimitiveCount, D3DPRIMITIVETYPE ePrimitiveType)
{
	assert(NULL!=m_pTerrainPatchProxyList && "__HardwareTransformPatch_RenderPatchNone");
	CTerrainPatchProxy * pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];
	
	if (!pTerrainPatchProxy->isUsed())
		return;

	auto pkVB = pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr();
	if (!pkVB)
		return;

	_mgr->SetShader(VF_PN);
	_mgr->SetVertexBuffer(pkVB);
	STATEMANAGER.DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);
}
