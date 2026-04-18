#pragma once

#include "GrpTexture.h"

class CGraphicShadowTexture : public CGraphicTexture
{
	public:
		CGraphicShadowTexture();
		virtual ~CGraphicShadowTexture();

		void Destroy();

		bool Create(int width, int height);

		void Begin();
		void End();
		void Set(int stage = 0) const;

		const D3DXMATRIX& GetLightVPMatrixReference() const;

	protected:
		void Initialize();

	protected:
		D3DXMATRIX			m_d3dLightVPMatrix;
		D3D11_VIEWPORT		m_d3dOldViewport;
		UINT				m_uOldNumViewports;

		ID3D11Texture2D*		m_pShadowTex;
		ID3D11RenderTargetView*	m_pShadowRTV;
		ID3D11Texture2D*		m_pDepthTex;
		ID3D11DepthStencilView*	m_pDepthDSV;

		ID3D11RenderTargetView*	m_pOldRTV;
		ID3D11DepthStencilView*	m_pOldDSV;
};
