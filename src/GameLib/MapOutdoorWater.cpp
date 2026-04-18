#include "StdAfx.h"
#include "EterLib/StateManager.h"
#include "EterLib/ResourceManager.h"

#include "MapOutdoor.h"
#include "TerrainPatch.h"

void CMapOutdoor::LoadWaterTexture()
{
	UnloadWaterTexture();
	char buf[256];
	for (int i = 0; i < 30; ++i)
	{
		sprintf(buf, "d:/ymir Work/special/water/%02d.dds", i+1);
		m_WaterInstances[i].SetImagePointer((CGraphicImage *) CResourceManager::Instance().GetResourcePointer(buf));
	}
}

void CMapOutdoor::UnloadWaterTexture()
{
	for (int i = 0; i < 30; ++i)
		m_WaterInstances[i].Destroy();
}

void CMapOutdoor::RenderWater()
{
	if (m_PatchVector.empty())
		return;

	if (!IsVisiblePart(PART_WATER))
		return;

	//////////////////////////////////////////////////////////////////////////
	// RenderState
	D3DXMATRIX matTexTransformWater;
	
	STATEMANAGER.SaveRenderState(RS11_ZWRITEENABLE, FALSE);
	STATEMANAGER.SaveRenderState(RS11_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_CULLMODE, D3D11_CULL_NONE);
	
	STATEMANAGER.SetTexture(0, m_WaterInstances[((ELTimer_GetMSec() / 70) % 30)].GetTexturePointer()->GetSRV());

	D3DXMatrixScaling(&matTexTransformWater, m_fWaterTexCoordBase, -m_fWaterTexCoordBase, 0.0f);
	D3DXMatrixMultiply(&matTexTransformWater, &m_matViewInverse, &matTexTransformWater);
	
	STATEMANAGER.SaveTransform(Texture0, &matTexTransformWater);
	STATEMANAGER.SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);

	STATEMANAGER.SaveTextureStageState(0, TSS11_TEXCOORDINDEX, TSS11_TCI_CAMERASPACEPOSITION);
	STATEMANAGER.SaveTextureStageState(0, TSS11_TEXTURETRANSFORMFLAGS, TTFF11_COUNT2);

	STATEMANAGER.SaveSamplerState(0, SS11_MINFILTER, TF11_ANISOTROPIC);
	STATEMANAGER.SaveSamplerState(0, SS11_MAGFILTER, TF11_ANISOTROPIC);
	STATEMANAGER.SaveSamplerState(0, SS11_MIPFILTER, TF11_LINEAR);
	STATEMANAGER.SaveSamplerState(0, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_WRAP);
	STATEMANAGER.SaveSamplerState(0, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_WRAP);
	
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP, TOP11_SELECTARG1);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1, TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP, TOP11_SELECTARG1);
	

	STATEMANAGER.SetTexture(1,NULL);
	STATEMANAGER.SetTextureStageState(1, TSS11_COLOROP, TOP11_DISABLE);
	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP, TOP11_DISABLE);

	// RenderState
	//////////////////////////////////////////////////////////////////////////

	// 물 위 아래 애니시키기...
	static float s_fWaterHeightCurrent = 0;
	static float s_fWaterHeightBegin = 0;
	static float s_fWaterHeightEnd = 0;
	static DWORD s_dwLastHeightChangeTime = CTimer::Instance().GetCurrentMillisecond();
	static DWORD s_dwBlendtime = 300;

	// 1.5초 마다 변경
	if ((CTimer::Instance().GetCurrentMillisecond() - s_dwLastHeightChangeTime) > s_dwBlendtime)
	{
		s_dwBlendtime = random_range(1000, 3000);

		if (s_fWaterHeightEnd == 0)
			s_fWaterHeightEnd = -random_range(0, 15);
		else
			s_fWaterHeightEnd = 0;

		s_fWaterHeightBegin = s_fWaterHeightCurrent;
		s_dwLastHeightChangeTime = CTimer::Instance().GetCurrentMillisecond();
	}

	s_fWaterHeightCurrent = s_fWaterHeightBegin + (s_fWaterHeightEnd - s_fWaterHeightBegin) * (float) ((CTimer::Instance().GetCurrentMillisecond() - s_dwLastHeightChangeTime) / (float) s_dwBlendtime);
	m_matWorldForCommonUse._43 = s_fWaterHeightCurrent;

	m_matWorldForCommonUse._41 = 0.0f;
	m_matWorldForCommonUse._42 = 0.0f;
	STATEMANAGER.SetTransform(World, &m_matWorldForCommonUse);
	
	float fFogDistance = __GetFogDistance();

	std::vector<std::pair<float, long> >::iterator i;

	for(i = m_PatchVector.begin();i != m_PatchVector.end(); ++i)
	{
		if (i->first<fFogDistance)	
			DrawWater(i->second);
	}

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetRenderState(RS11_ALPHABLENDENABLE, FALSE);

	for(i = m_PatchVector.begin();i != m_PatchVector.end(); ++i)
	{
		if (i->first>=fFogDistance)	
			DrawWater(i->second);
	}

	// 렌더링 한 후에는 물 z 위치를 복구
	m_matWorldForCommonUse._43 = 0.0f;

	//////////////////////////////////////////////////////////////////////////
	// RenderState
	STATEMANAGER.RestoreTransform(Texture0);
	STATEMANAGER.RestoreSamplerState(0, SS11_MINFILTER);
	STATEMANAGER.RestoreSamplerState(0, SS11_MAGFILTER);
	STATEMANAGER.RestoreSamplerState(0, SS11_MIPFILTER);
	STATEMANAGER.RestoreSamplerState(0, SS11_ADDRESSU);
	STATEMANAGER.RestoreSamplerState(0, SS11_ADDRESSV);
	STATEMANAGER.RestoreTextureStageState(0, TSS11_TEXCOORDINDEX);
	STATEMANAGER.RestoreTextureStageState(0, TSS11_TEXTURETRANSFORMFLAGS);
	
	STATEMANAGER.RestoreRenderState(RS11_ZWRITEENABLE);
	STATEMANAGER.RestoreRenderState(RS11_ALPHABLENDENABLE);
	STATEMANAGER.RestoreRenderState(RS11_CULLMODE);	
}

void CMapOutdoor::DrawWater(long patchnum)
{
	assert(NULL!=m_pTerrainPatchProxyList);
	if (!m_pTerrainPatchProxyList)
		return;

	CTerrainPatchProxy& rkTerrainPatchProxy = m_pTerrainPatchProxyList[patchnum];

	if (!rkTerrainPatchProxy.isUsed())
		return;

	if (!rkTerrainPatchProxy.isWaterExists())
		return;

	CGraphicVertexBuffer* pkVB=rkTerrainPatchProxy.GetWaterVertexBufferPointer();
	if (!pkVB)
		return;
	
	if (!pkVB->GetD3DVertexBuffer())
		return;

	UINT uPriCount=rkTerrainPatchProxy.GetWaterFaceCount();
	if (!uPriCount)
		return;
	
	STATEMANAGER.SetStreamSource(0, pkVB->GetD3DVertexBuffer(), sizeof(SWaterVertex));
	STATEMANAGER.DrawPrimitive(D3DPT_TRIANGLELIST, 0, uPriCount);

	ms_faceCount += uPriCount;
}
