#include "StdAfx.h"

#include "EterBase/Timer.h"
#include "EterLib/StateManager.h"
#include "EterLib/Camera.h"

#include "SpeedTreeForestDirectX.h"
#include "SpeedTreeConfig.h"
#include "VertexShaders.h"
#include "qMin32Lib/ConstantBufferManager.h"
#include "qMin32Lib/DxManager.h"
//

CSpeedTreeForestDirectX::CSpeedTreeForestDirectX()
	: m_pDx(nullptr)
{
}

CSpeedTreeForestDirectX::~CSpeedTreeForestDirectX()
{
}

bool CSpeedTreeForestDirectX::InitVertexShaders()
{
	if (!m_pBranchShader)
		m_pBranchShader = LoadBranchShader();

	if (!m_pLeafShader)
		m_pLeafShader = LoadLeafShader();

	if (m_pBranchShader && m_pBranchShader->IsValid() && m_pLeafShader && m_pLeafShader->IsValid())
	{
		CSpeedTreeWrapper::SetVertexShaders(m_pBranchShader, m_pLeafShader);
		return true;
	}

	return false;
}

bool CSpeedTreeForestDirectX::SetRenderingDevice(ID3D11Device* lpDevice)
{
	m_pDx = lpDevice;

	if (!InitVertexShaders())
		return false;

	const float c_afLightPosition[4] = { -0.707f, -0.300f, 0.707f, 0.0f };
	const float	c_afLightAmbient[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	const float	c_afLightDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const float	c_afLightSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	float afLight1[] =
	{
		c_afLightPosition[0], c_afLightPosition[1], c_afLightPosition[2],
		c_afLightDiffuse[0], c_afLightDiffuse[1], c_afLightDiffuse[2],
		c_afLightAmbient[0], c_afLightAmbient[1], c_afLightAmbient[2],
		c_afLightSpecular[0], c_afLightSpecular[1], c_afLightSpecular[2],
		c_afLightPosition[3],
		1.0f, 0.0f, 0.0f
	};

	CSpeedTreeRT::SetNumWindMatrices(c_nNumWindMatrices);
	CSpeedTreeRT::SetLightAttributes(0, afLight1);
	CSpeedTreeRT::SetLightState(0, true);
	return true;
}

void CSpeedTreeForestDirectX::UploadWindMatrix(UINT uiLocation, const float* pMatrix) const
{
	(void)uiLocation;
	(void)pMatrix;
}

void CSpeedTreeForestDirectX::UpdateCompundMatrix(const D3DXVECTOR3& c_rEyeVec, const D3DXMATRIX& c_rmatView, const D3DXMATRIX& c_rmatProj)
{
	D3DXMATRIX matBlendShader;
	D3DXMatrixMultiply(&matBlendShader, &c_rmatView, &c_rmatProj);

	float afDirection[3];
	afDirection[0] = matBlendShader.m[0][2];
	afDirection[1] = matBlendShader.m[1][2];
	afDirection[2] = matBlendShader.m[2][2];
	CSpeedTreeRT::SetCamera(c_rEyeVec, afDirection);

	if (_mgr)
		_mgr->GetCbMgr()->SetSpeedTreeCompoundMatrix(matBlendShader);
}

void CSpeedTreeForestDirectX::Render(unsigned long ulRenderBitVector)
{
	UpdateSystem(CTimer::Instance().GetCurrentSecond());

	if (m_pMainTreeMap.empty())
		return;

	if (!EnsureVertexShaders())
		return;

	if (!(ulRenderBitVector & Forest_RenderToShadow) && !(ulRenderBitVector & Forest_RenderToMiniMap))
		UpdateCompundMatrix(CCameraManager::Instance().GetCurrentCamera()->GetEye(), ms_matView, ms_matProj);

#ifdef WRAPPER_USE_DYNAMIC_LIGHTING
	STATEMANAGER.SetRenderState(RS11_LIGHTING, TRUE);
#else
	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);
#endif

	UINT uiCount = 0;
	for (auto& it : m_pMainTreeMap)
	{
		auto ppInstances = it.second->GetInstances(uiCount);
		for (auto& inst : ppInstances)
			inst->Advance();
	}

	if (_mgr)
	{
		_mgr->GetCbMgr()->SetSpeedTreeLight(m_afLighting);
		_mgr->GetCbMgr()->SetSpeedTreeFog(m_afFog);
	}

	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLORARG2, TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_COLOROP, TOP11_MODULATE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAARG2, TA11_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, TSS11_ALPHAOP, TOP11_MODULATE);
	STATEMANAGER.SetSamplerState(0, SS11_MINFILTER, TF11_LINEAR);
	STATEMANAGER.SetSamplerState(0, SS11_MAGFILTER, TF11_LINEAR);
	STATEMANAGER.SetSamplerState(0, SS11_MIPFILTER, TF11_LINEAR);

	STATEMANAGER.SetTextureStageState(1, TSS11_COLORARG1, TA11_TEXTURE);
	STATEMANAGER.SetTextureStageState(1, TSS11_COLORARG2, TA11_CURRENT);
	STATEMANAGER.SetTextureStageState(1, TSS11_COLOROP, TOP11_MODULATE);
	STATEMANAGER.SetTextureStageState(1, TSS11_ALPHAOP, TOP11_DISABLE);
	STATEMANAGER.SetSamplerState(1, SS11_ADDRESSU, D3D11_TEXTURE_ADDRESS_WRAP);
	STATEMANAGER.SetSamplerState(1, SS11_ADDRESSV, D3D11_TEXTURE_ADDRESS_WRAP);

	STATEMANAGER.SaveRenderState(RS11_ALPHATESTENABLE, TRUE);
	STATEMANAGER.SaveRenderState(RS11_CULLMODE, D3D11_CULL_NONE);

	if (ulRenderBitVector & Forest_RenderBranches)
	{
		m_pBranchShader->Set();
		for (auto& it : m_pMainTreeMap)
		{
			auto pMainTree = it.second;
			auto ppInstances = pMainTree->GetInstances(uiCount);

			pMainTree->SetupBranchForTreeType();
			for (auto& inst : ppInstances)
				if (inst->isShow())
					inst->RenderBranches();
		}
	}

	STATEMANAGER.SetRenderState(RS11_CULLMODE, D3D11_CULL_NONE);

	if (ulRenderBitVector & Forest_RenderFronds)
	{
		m_pBranchShader->Set();
		for (auto& it : m_pMainTreeMap)
		{
			auto pMainTree = it.second;
			auto ppInstances = pMainTree->GetInstances(uiCount);

			pMainTree->SetupFrondForTreeType();
			for (auto& inst : ppInstances)
				if (inst->isShow())
					inst->RenderFronds();
		}
	}

	if (ulRenderBitVector & Forest_RenderLeaves)
	{
		m_pLeafShader->Set();
		if (ulRenderBitVector & Forest_RenderToShadow || ulRenderBitVector & Forest_RenderToMiniMap)
			STATEMANAGER.SaveRenderState(RS11_ALPHAREF, 0);

		for (auto& it : m_pMainTreeMap)
		{
			auto pMainTree = it.second;
			auto ppInstances = pMainTree->GetInstances(uiCount);

			pMainTree->SetupLeafForTreeType();
			for (auto& inst : ppInstances)
				if (inst->isShow())
					inst->RenderLeaves();
			pMainTree->EndLeafForTreeType();
		}

		if (ulRenderBitVector & Forest_RenderToShadow || ulRenderBitVector & Forest_RenderToMiniMap)
			STATEMANAGER.RestoreRenderState(RS11_ALPHAREF);
	}

	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);

	if (ulRenderBitVector & Forest_RenderBillboards)
	{
		for (auto& it : m_pMainTreeMap)
		{
			auto ppInstances = it.second->GetInstances(uiCount);
			for (auto& inst : ppInstances)
				if (inst->isShow())
					inst->RenderBillboards();
		}
	}

	STATEMANAGER.RestoreRenderState(RS11_CULLMODE);
	STATEMANAGER.RestoreRenderState(RS11_ALPHATESTENABLE);
}
