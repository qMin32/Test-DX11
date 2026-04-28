#include "StdAfx.h"
#include "EterLib/StateManager.h"
#include "PythonGraphic.h"
#include <stb_image_write.h>
#include <thread>
#include <string>
#include <memory>

void CPythonGraphic::Destroy()
{	
}

LPDIRECT3D9EX CPythonGraphic::GetD3D()
{
	return NULL;
}

float CPythonGraphic::GetOrthoDepth()
{
	return m_fOrthoDepth;
}

void CPythonGraphic::SetInterfaceRenderState()
{
	STATEMANAGER.SetTransform(Projection, &ms_matIdentity);
 	STATEMANAGER.SetTransform(View, &ms_matIdentity);
	STATEMANAGER.SetTransform(World, &ms_matIdentity);

	STATEMANAGER.SetSamplerState(0, SS11_MINFILTER, TF11_NONE);
	STATEMANAGER.SetSamplerState(0, SS11_MAGFILTER, TF11_NONE);
	STATEMANAGER.SetSamplerState(0, SS11_MIPFILTER, TF11_NONE);

	STATEMANAGER.SetRenderState(RS11_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SetRenderState(RS11_SRCBLEND,	D3D11_BLEND_SRC_ALPHA);
	STATEMANAGER.SetRenderState(RS11_DESTBLEND, D3D11_BLEND_INV_SRC_ALPHA);

	// D3D11 fix: ensure scissor test is OFF for UI baseline (windows that need it enable it via ScopedScissorRect)
	STATEMANAGER.SetRenderState(RS11_SCISSORTESTENABLE, FALSE);

	CPythonGraphic::Instance().SetBlendOperation();
	CPythonGraphic::Instance().SetOrtho2D(ms_iWidth, ms_iHeight, GetOrthoDepth());

	STATEMANAGER.SetRenderState(RS11_LIGHTING, FALSE);
}

void CPythonGraphic::SetGameRenderState()
{
	STATEMANAGER.SetSamplerState(0, SS11_MINFILTER, TF11_ANISOTROPIC);
	STATEMANAGER.SetSamplerState(0, SS11_MAGFILTER, TF11_ANISOTROPIC);
	STATEMANAGER.SetSamplerState(0, SS11_MIPFILTER, TF11_LINEAR);

	STATEMANAGER.SetRenderState(RS11_ALPHABLENDENABLE, FALSE);
	STATEMANAGER.SetRenderState(RS11_LIGHTING, TRUE);
}

void CPythonGraphic::SetCursorPosition(int x, int y)
{
	CScreen::SetCursorPosition(x, y, ms_iWidth, ms_iHeight);
}

void CPythonGraphic::SetOmniLight()
{
    // Set up a material
    D3DMATERIAL9 Material;
	Material.Ambient = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
	Material.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	Material.Emissive = D3DXCOLOR(0.1f, 0.1f, 0.1f, 1.0f);
    STATEMANAGER.SetMaterial(&Material);

	D3DLIGHT9 Light;
	Light.Type = D3DLIGHT_SPOT;
    Light.Position = D3DXVECTOR3(50.0f, 150.0f, 350.0f);
    Light.Direction = D3DXVECTOR3(-0.15f, -0.3f, -0.9f);
    Light.Theta = D3DXToRadian(30.0f);
    Light.Phi = D3DXToRadian(45.0f);
    Light.Falloff = 1.0f;
    Light.Attenuation0 = 0.0f;
    Light.Attenuation1 = 0.005f;
    Light.Attenuation2 = 0.0f;
    Light.Diffuse.r = 1.0f;
    Light.Diffuse.g = 1.0f;
    Light.Diffuse.b = 1.0f;
	Light.Diffuse.a = 1.0f;
	Light.Ambient.r = 1.0f;
	Light.Ambient.g = 1.0f;
	Light.Ambient.b = 1.0f;
	Light.Ambient.a = 1.0f;
    Light.Range = 500.0f;
	STATEMANAGER.SetLight(0, &Light);

	Light.Type = D3DLIGHT_POINT;
	Light.Position = D3DXVECTOR3(0.0f, 200.0f, 200.0f);
	Light.Attenuation0 = 0.1f;
	Light.Attenuation1 = 0.01f;
	Light.Attenuation2 = 0.0f;
	// Note: D3D11 renderer only supports light index 0 currently
}

void CPythonGraphic::SetViewport(float fx, float fy, float fWidth, float fHeight)
{
	if (!ms_lpd3d11Context)
		return;

	UINT n = 1;
	ms_lpd3d11Context->RSGetViewports(&n, &m_backupViewport);

	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = fx;
	vp.TopLeftY = fy;
	vp.Width = fWidth;
	vp.Height = fHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	ms_lpd3d11Context->RSSetViewports(1, &vp);
}

void CPythonGraphic::RestoreViewport()
{
	if (ms_lpd3d11Context)
		ms_lpd3d11Context->RSSetViewports(1, &m_backupViewport);
}

void CPythonGraphic::SetGamma(float fGammaFactor)
{
	// TODO: D3D11 has no device-level gamma ramps. Implement via DXGI Output gamma control
	// or via a post-processing shader. For now this is a no-op.
	(void)fGammaFactor;
}

bool CPythonGraphic::SaveScreenShot()
{
	if (!ms_lpd3d11Device || !ms_lpd3d11Context || !ms_lpd3d11SwapChain)
		return false;

	// Get back buffer
	ID3D11Texture2D* pBackBuffer = nullptr;
	if (FAILED(ms_lpd3d11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
	{
		TraceError("CPythonGraphic::SaveScreenShot() - Failed to get back buffer");
		return false;
	}

	D3D11_TEXTURE2D_DESC desc;
	pBackBuffer->GetDesc(&desc);

	// Create staging texture for CPU readback
	D3D11_TEXTURE2D_DESC stagingDesc = desc;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;

	ID3D11Texture2D* pStaging = nullptr;
	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&stagingDesc, nullptr, &pStaging)))
	{
		TraceError("CPythonGraphic::SaveScreenShot() - Failed to create staging texture");
		pBackBuffer->Release();
		return false;
	}

	ms_lpd3d11Context->CopyResource(pStaging, pBackBuffer);
	pBackBuffer->Release();

	D3D11_MAPPED_SUBRESOURCE mapped;
	if (FAILED(ms_lpd3d11Context->Map(pStaging, 0, D3D11_MAP_READ, 0, &mapped)))
	{
		TraceError("CPythonGraphic::SaveScreenShot() - Failed to map staging texture");
		pStaging->Release();
		return false;
	}

	// Copy pixel data to a plain buffer (BGRA -> RGB)
	uint32_t width = desc.Width;
	uint32_t height = desc.Height;
	auto pixels = std::make_shared<std::vector<uint8_t>>(width * height * 3);
	uint8_t* dst = pixels->data();
	auto* src = static_cast<uint8_t*>(mapped.pData);

	for (uint32_t y = 0; y < height; ++y)
	{
		uint8_t* row = src + y * mapped.RowPitch;
		for (uint32_t x = 0; x < width; ++x)
		{
			dst[0] = row[2]; // R
			dst[1] = row[1]; // G
			dst[2] = row[0]; // B
			dst += 3;
			row += 4;
		}
	}

	ms_lpd3d11Context->Unmap(pStaging, 0);
	pStaging->Release();

	if (!CreateDirectoryA("screenshot", NULL) && ERROR_ALREADY_EXISTS != GetLastError())
		return false;

	SYSTEMTIME st;
	GetSystemTime(&st);

	char szFileName[MAX_PATH];
	sprintf_s(szFileName, "screenshot/%04d%02d%02d_%02d%02d%02d.png",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	// PNG encode + file write on background thread (no D3D dependency)
	std::thread([pixels, w = width, h = height, fileName = std::string(szFileName)]()
	{
		stbi_write_png(fileName.c_str(), w, h, 3, pixels->data(), w * 3);
	}).detach();

	return true;
}

void CPythonGraphic::PushState()
{
	TState curState;

	curState.matProj = ms_matProj;
	curState.matView = ms_matView;
	//STATEMANAGER.SaveTransform(World, &m_SaveWorldMatrix);

	m_stateStack.push(curState);
	//CCamera::Instance().PushParams();
}

void CPythonGraphic::PopState()
{
	if (m_stateStack.empty())
	{
		assert(!"PythonGraphic::PopState StateStack is EMPTY");
		return;
	}
	
	TState & rState = m_stateStack.top();

	//STATEMANAGER.RestoreTransform(World);
	ms_matProj = rState.matProj;
	ms_matView = rState.matView;
	
	UpdatePipeLineMatrix();

	m_stateStack.pop();
	//CCamera::Instance().PopParams();
}

void CPythonGraphic::RenderImage(CGraphicImageInstance* pImageInstance, float x, float y)
{
	assert(pImageInstance != NULL);

	//SetColorRenderState();
	const CGraphicTexture * c_pTexture = pImageInstance->GetTexturePointer();

	float width = (float) pImageInstance->GetWidth();
	float height = (float) pImageInstance->GetHeight();

	c_pTexture->SetTextureStage(0);

	RenderTextureBox(x,
					 y,
					 x + width,
					 y + height,
					 0.0f,
					 0.5f / width, 
					 0.5f / height, 
					 (width + 0.5f) / width, 
					 (height + 0.5f) / height);
}

void CPythonGraphic::RenderAlphaImage(CGraphicImageInstance* pImageInstance, float x, float y, float aLeft, float aRight)
{
	assert(pImageInstance != NULL);

	D3DXCOLOR DiffuseColor1 = D3DXCOLOR(1.0f, 1.0f, 1.0f, aLeft);
	D3DXCOLOR DiffuseColor2 = D3DXCOLOR(1.0f, 1.0f, 1.0f, aRight);

	const CGraphicTexture * c_pTexture = pImageInstance->GetTexturePointer();

	float width = (float) pImageInstance->GetWidth();
	float height = (float) pImageInstance->GetHeight();

	c_pTexture->SetTextureStage(0);

	float sx = x;
	float sy = y;
	float ex = x + width;
	float ey = y + height;
	float z = 0.0f;

	float su = 0.0f;
	float sv = 0.0f;
	float eu = 1.0f;
	float ev = 1.0f;

	TPDTVertex vertices[4];
	vertices[0].position = TPosition(sx, sy, z);
	vertices[0].diffuse = DiffuseColor1;
	vertices[0].texCoord = TTextureCoordinate(su, sv);

	vertices[1].position = TPosition(ex, sy, z);
	vertices[1].diffuse = DiffuseColor2;
	vertices[1].texCoord = TTextureCoordinate(eu, sv);

	vertices[2].position = TPosition(sx, ey, z);
	vertices[2].diffuse = DiffuseColor1;
	vertices[2].texCoord = TTextureCoordinate(su, ev);

	vertices[3].position = TPosition(ex, ey, z);
	vertices[3].diffuse = DiffuseColor2;
	vertices[3].texCoord = TTextureCoordinate(eu, ev);

	_mgr->SetShader(VF_MESH);
	// 2004.11.18.myevan.DrawIndexPrimitiveUP -> DynamicVertexBuffer
	CGraphicBase::SetDefaultIndexBuffer(DEFAULT_IB_FILL_RECT);
	if (CGraphicBase::SetPDTStream(vertices, 4))
		STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 0, 2);
}

void CPythonGraphic::RenderCoolTimeBox(float fxCenter, float fyCenter, float fRadius, float fTime)
{
	if (fTime >= 1.0f)
		return;

	fTime = std::max(0.0f, fTime);

	static D3DXCOLOR color = D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.5f);
	static D3DXVECTOR2 s_v2BoxPos[8] =
	{
		D3DXVECTOR2( -1.0f, -1.0f ),
		D3DXVECTOR2( -1.0f,  0.0f ),
		D3DXVECTOR2( -1.0f, +1.0f ),
		D3DXVECTOR2(  0.0f, +1.0f ),
		D3DXVECTOR2( +1.0f, +1.0f ),
		D3DXVECTOR2( +1.0f,  0.0f ),
		D3DXVECTOR2( +1.0f, -1.0f ),
		D3DXVECTOR2(  0.0f, -1.0f ),
	};

	int iTriCount = int(8 - 8.0f * fTime);
	float fLastPercentage = (8 - 8.0f * fTime) - iTriCount;

	// Build fan vertex positions (center + edge points)
	std::vector<D3DXVECTOR2> fanPos;
	fanPos.push_back(D3DXVECTOR2(fxCenter, fyCenter));                      // v0 = center
	fanPos.push_back(D3DXVECTOR2(fxCenter, fyCenter - fRadius));            // v1 = top-center

	for (int j = 0; j < iTriCount; ++j)
		fanPos.push_back(D3DXVECTOR2(fxCenter + s_v2BoxPos[j].x * fRadius, fyCenter + s_v2BoxPos[j].y * fRadius));

	if (fLastPercentage > 0.0f)
	{
		D3DXVECTOR2 * pv2LastPos = &s_v2BoxPos[(iTriCount-1+8)%8];
		D3DXVECTOR2 * pv2Pos = &s_v2BoxPos[(iTriCount+8)%8];
		fanPos.push_back(D3DXVECTOR2(
			fxCenter + ((pv2Pos->x-pv2LastPos->x) * fLastPercentage + pv2LastPos->x) * fRadius,
			fyCenter + ((pv2Pos->y-pv2LastPos->y) * fLastPercentage + pv2LastPos->y) * fRadius));
		++iTriCount;
	}

	if (iTriCount < 1)
		return;

	// Expand fan into triangle list: for each tri, emit (center, v[i+1], v[i+2])
	std::vector<TPDTVertex> vertices;
	vertices.reserve(iTriCount * 3);
	TPDTVertex vtx;
	vtx.position.z = 0.0f;
	vtx.diffuse = color;
	vtx.texCoord.x = 0.0f;
	vtx.texCoord.y = 0.0f;

	for (int i = 0; i < iTriCount; ++i)
	{
		vtx.position.x = fanPos[0].x; vtx.position.y = fanPos[0].y;
		vertices.push_back(vtx);
		vtx.position.x = fanPos[i+1].x; vtx.position.y = fanPos[i+1].y;
		vertices.push_back(vtx);
		vtx.position.x = fanPos[i+2].x; vtx.position.y = fanPos[i+2].y;
		vertices.push_back(vtx);
	}

	if (SetPDTStream(&vertices[0], vertices.size()))
	{
		STATEMANAGER.SaveTextureStageState(0, TSS11_COLORARG1,	TA11_DIFFUSE);
		STATEMANAGER.SaveTextureStageState(0, TSS11_COLOROP,	TOP11_SELECTARG1);
		STATEMANAGER.SaveTextureStageState(0, TSS11_ALPHAARG1,	TA11_DIFFUSE);
		STATEMANAGER.SaveTextureStageState(0, TSS11_ALPHAOP,	TOP11_SELECTARG1);
		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
		_mgr->SetShader(VF_PDT);
		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, iTriCount, 0);
		STATEMANAGER.RestoreTextureStageState(0, TSS11_COLORARG1);
		STATEMANAGER.RestoreTextureStageState(0, TSS11_COLOROP);
		STATEMANAGER.RestoreTextureStageState(0, TSS11_ALPHAARG1);
		STATEMANAGER.RestoreTextureStageState(0, TSS11_ALPHAOP);
	}
}

long CPythonGraphic::GenerateColor(float r, float g, float b, float a)
{
	return GetColor(r, g, b, a);
}

void CPythonGraphic::RenderDownButton(float sx, float sy, float ex, float ey)
{
	RenderBox2d(sx, sy, ex, ey);

	SetDiffuseColor(m_darkColor);
	RenderLine2d(sx, sy, ex, sy);
	RenderLine2d(sx, sy, sx, ey);

	SetDiffuseColor(m_lightColor);
	RenderLine2d(sx, ey, ex, ey);
	RenderLine2d(ex, sy, ex, ey);
}

void CPythonGraphic::RenderUpButton(float sx, float sy, float ex, float ey)
{
	RenderBox2d(sx, sy, ex, ey);

	SetDiffuseColor(m_lightColor);
	RenderLine2d(sx, sy, ex, sy);
	RenderLine2d(sx, sy, sx, ey);

	SetDiffuseColor(m_darkColor);
	RenderLine2d(sx, ey, ex, ey);
	RenderLine2d(ex, sy, ex, ey);
}

DWORD CPythonGraphic::GetAvailableMemory()
{
	// D3D11: query DXGI adapter for video memory
	if (!ms_lpd3d11Device)
		return 0;

	IDXGIDevice* pDxgiDevice = NULL;
	if (FAILED(ms_lpd3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDxgiDevice)))
		return 0;

	IDXGIAdapter* pDxgiAdapter = NULL;
	if (FAILED(pDxgiDevice->GetAdapter(&pDxgiAdapter)))
	{
		pDxgiDevice->Release();
		return 0;
	}

	DXGI_ADAPTER_DESC desc;
	HRESULT hr = pDxgiAdapter->GetDesc(&desc);
	pDxgiAdapter->Release();
	pDxgiDevice->Release();

	if (FAILED(hr))
		return 0;

	return (DWORD)desc.DedicatedVideoMemory;
}

CPythonGraphic::CPythonGraphic()
{
	m_lightColor = GetColor(1.0f, 1.0f, 1.0f);
	m_darkColor = GetColor(0.0f, 0.0f, 0.0f);
	
	memset(&m_backupViewport, 0, sizeof(D3D11_VIEWPORT));

	m_fOrthoDepth = 1000.0f;
}

CPythonGraphic::~CPythonGraphic()
{
	Tracef("Python Graphic Clear\n");
}
