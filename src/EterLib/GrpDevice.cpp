#include "StdAfx.h"
#include "GrpDevice.h"
#include "GrpD3D11Renderer.h"
#include "StandaloneVertexDeclaration.h"
#include "EterBase/Stl.h"
#include "EterBase/Debug.h"
#include "qMin32Lib/All.h"

static ID3D11InfoQueue* s_pD3D11InfoQueue = NULL;

#include <intrin.h>
#include <utf8.h>

bool CPU_HAS_SSE2 = false;
bool GRAPHICS_CAPS_CAN_NOT_DRAW_LINE = false;
bool GRAPHICS_CAPS_CAN_NOT_DRAW_SHADOW = false;
bool GRAPHICS_CAPS_HALF_SIZE_IMAGE = false;
bool GRAPHICS_CAPS_CAN_NOT_TEXTURE_ADDRESS_BORDER = false;
bool GRAPHICS_CAPS_SOFTWARE_TILING = false;

bool g_isBrowserMode=false;
RECT g_rcBrowser;

// D3D11 feature level 11_0 guarantees 16384x16384 textures
static DWORD s_MaxTextureWidth = 16384;
static DWORD s_MaxTextureHeight = 16384;

DWORD GetMaxTextureWidth()
{
	return s_MaxTextureWidth;
}

DWORD GetMaxTextureHeight()
{
	return s_MaxTextureHeight;
}

CGraphicDevice::CGraphicDevice()
: m_uBackBufferCount(0)
{
	__Initialize();
}

CGraphicDevice::~CGraphicDevice()
{
	Destroy();
}

void CGraphicDevice::__Initialize()
{
	ms_lpd3dMatStack	= NULL;

	ms_dwWavingEndTime = 0;
	ms_dwFlashingEndTime = 0;

	m_pStateManager		= NULL;
	m_pD3D11Renderer	= NULL;

	__InitializePDTVertexBufferList();
}

void CGraphicDevice::RegisterWarningString(UINT uiMsg, const char * c_szString)
{
	m_kMap_strWarningMessage[uiMsg] = c_szString;
}

void CGraphicDevice::__WarningMessage(HWND hWnd, UINT uiMsg)
{
	auto it = m_kMap_strWarningMessage.find(uiMsg);
	if (it == m_kMap_strWarningMessage.end())
		return;

	const std::string& msgUtf8 = it->second;

	std::wstring wMsg = Utf8ToWide(msgUtf8);
	std::wstring wCaption = L"Warning";

	MessageBoxW(hWnd, wMsg.c_str(), wCaption.c_str(), MB_OK | MB_TOPMOST);
}

void CGraphicDevice::MoveWebBrowserRect(const RECT& c_rcWebPage)
{
	g_rcBrowser=c_rcWebPage;
}

void CGraphicDevice::EnableWebBrowserMode(const RECT& c_rcWebPage)
{
	g_isBrowserMode=true;
	g_rcBrowser=c_rcWebPage;
}

void CGraphicDevice::DisableWebBrowserMode()
{
	g_isBrowserMode=false;
}
		
bool CGraphicDevice::ResizeBackBuffer(UINT uWidth, UINT uHeight)
{
	if (!ms_lpd3d11SwapChain)
		return false;

	if ((UINT)ms_iWidth == uWidth && (UINT)ms_iHeight == uHeight)
		return true;

	// Release existing views before resizing
	if (ms_lpd3d11Context)
		ms_lpd3d11Context->OMSetRenderTargets(0, NULL, NULL);

	safe_release(ms_lpd3d11DSV);
	safe_release(ms_lpd3d11DepthStencil);
	safe_release(ms_lpd3d11RTV);

	HRESULT hr = ms_lpd3d11SwapChain->ResizeBuffers(0, uWidth, uHeight, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr))
	{
		Tracenf("D3D11: ResizeBuffers failed (0x%08X)", hr);
		return false;
	}

	// Recreate render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = ms_lpd3d11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	if (FAILED(hr))
		return false;

	hr = ms_lpd3d11Device->CreateRenderTargetView(pBackBuffer, NULL, &ms_lpd3d11RTV);
	pBackBuffer->Release();
	if (FAILED(hr))
		return false;

	// Recreate depth stencil
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = uWidth;
	descDepth.Height = uHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	hr = ms_lpd3d11Device->CreateTexture2D(&descDepth, NULL, &ms_lpd3d11DepthStencil);
	if (FAILED(hr))
		return false;

	hr = ms_lpd3d11Device->CreateDepthStencilView(ms_lpd3d11DepthStencil, NULL, &ms_lpd3d11DSV);
	if (FAILED(hr))
		return false;

	ms_lpd3d11Context->OMSetRenderTargets(1, &ms_lpd3d11RTV, ms_lpd3d11DSV);

	// Update viewport
	D3D11_VIEWPORT vp = {};
	vp.Width = (float)uWidth;
	vp.Height = (float)uHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	ms_lpd3d11Context->RSSetViewports(1, &vp);

	ms_iWidth = uWidth;
	ms_iHeight = uHeight;

	// Update viewport struct
	ms_Viewport.X = 0;
	ms_Viewport.Y = 0;
	ms_Viewport.Width = uWidth;
	ms_Viewport.Height = uHeight;
	ms_Viewport.MinZ = 0.0f;
	ms_Viewport.MaxZ = 1.0f;

	if (m_pD3D11Renderer)
		m_pD3D11Renderer->SetScreenSize((float)uWidth, (float)uHeight);

	STATEMANAGER.SetDefaultState();

	return true;
}

LPDIRECT3DVERTEXDECLARATION9 CGraphicDevice::CreatePNTStreamVertexShader()
{
	LPDIRECT3DVERTEXDECLARATION9 dwShader = NULL;

	D3DVERTEXELEMENT9 pShaderDecl[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	dwShader = CreateStandaloneVertexDeclaration(pShaderDecl);

	return dwShader;
}

LPDIRECT3DVERTEXDECLARATION9 CGraphicDevice::CreatePNT2StreamVertexShader()
{
	LPDIRECT3DVERTEXDECLARATION9 dwShader = NULL;

	D3DVERTEXELEMENT9 pShaderDecl[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 32, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
		D3DDECL_END()
	};

	dwShader = CreateStandaloneVertexDeclaration(pShaderDecl);

	return dwShader;
}

LPDIRECT3DVERTEXDECLARATION9 CGraphicDevice::CreatePTStreamVertexShader()
{
	LPDIRECT3DVERTEXDECLARATION9 dwShader = NULL;

	D3DVERTEXELEMENT9 pShaderDecl[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 1, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	dwShader = CreateStandaloneVertexDeclaration(pShaderDecl);

	return dwShader;
}

LPDIRECT3DVERTEXDECLARATION9 CGraphicDevice::CreateDoublePNTStreamVertexShader()
{
	LPDIRECT3DVERTEXDECLARATION9 dwShader = NULL;

	D3DVERTEXELEMENT9 pShaderDecl[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },

		{ 1, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1 },
		{ 1, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 1 },
		{ 1, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
		D3DDECL_END()
	};

	dwShader = CreateStandaloneVertexDeclaration(pShaderDecl);

	return dwShader;
}

CGraphicDevice::EDeviceState CGraphicDevice::GetDeviceState()
{
	// D3D11 doesn't have device lost — always OK
	if (!ms_lpd3d11Device)
		return DEVICESTATE_NULL;

	return DEVICESTATE_OK;
}

bool CGraphicDevice::Reset()
{
	// D3D11 uses swap chain resize instead of Reset
	return true;
}

int CGraphicDevice::Create(HWND hWnd, int iHres, int iVres, bool Windowed, int /*iBit*/, int iReflashRate)
{
	int iRet = CREATE_OK;

	Destroy();

	ms_iWidth	= iHres;
	ms_iHeight	= iVres;

	ms_hWnd		= hWnd;
	ms_hDC		= GetDC(hWnd);

	if (!Windowed)
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, iHres, iVres, SWP_SHOWWINDOW);

	if (!CreateD3D11(hWnd, iHres, iVres, Windowed))
	{
		Tracenf("D3D11: Device creation failed, game cannot continue");
		return CREATE_DEVICE;
	}

	m_pStateManager = new CStateManager();

	if (m_pStateManager && m_pD3D11Renderer)
		m_pStateManager->SetD3D11Renderer(m_pD3D11Renderer);

	D3DXCreateMatrixStack(0, &ms_lpd3dMatStack);
	ms_lpd3dMatStack->LoadIdentity();

	ms_ptVS	= CreatePTStreamVertexShader();
	ms_pntVS = CreatePNTStreamVertexShader();
	ms_pnt2VS = CreatePNT2StreamVertexShader();

	D3DXMatrixIdentity(&ms_matIdentity);
	D3DXMatrixIdentity(&ms_matView);
	D3DXMatrixIdentity(&ms_matProj);
	D3DXMatrixIdentity(&ms_matInverseView);
	D3DXMatrixIdentity(&ms_matInverseViewYAxis);
	D3DXMatrixIdentity(&ms_matScreen0);
	D3DXMatrixIdentity(&ms_matScreen1);
	D3DXMatrixIdentity(&ms_matScreen2);

	ms_matScreen0._11 = 1;
	ms_matScreen0._22 = -1;	

	ms_matScreen1._41 = 1;
	ms_matScreen1._42 = 1;

	ms_matScreen2._11 = (float) iHres / 2;
	ms_matScreen2._22 = (float) iVres / 2;

	// Set viewport from D3D11 device
	ms_Viewport.X = 0;
	ms_Viewport.Y = 0;
	ms_Viewport.Width = iHres;
	ms_Viewport.Height = iVres;
	ms_Viewport.MinZ = 0.0f;
	ms_Viewport.MaxZ = 1.0f;

	// D3DX mesh objects not available without D3D9 device — set to NULL (debug only)
	ms_lpSphereMesh = NULL;
	ms_lpCylinderMesh = NULL;

	// Create default index buffers and PDT vertex buffers on D3D11
	if (!__CreateDefaultIndexBufferList())
	{
		Tracenf("D3D11: Failed to create default index buffers");
		return CREATE_DEVICE;
	}

	if (!__CreatePDTVertexBufferList())
	{
		Tracenf("D3D11: Failed to create PDT vertex buffers");
		return CREATE_DEVICE;
	}

	// Check texture memory
	DWORD dwTexMemSize = GetAvailableTextureMemory();

	if (dwTexMemSize < 64 * 1024 * 1024)
		ms_isLowTextureMemory = true;
	else
		ms_isLowTextureMemory = false;

	if (dwTexMemSize > 100 * 1024 * 1024)
		ms_isHighTextureMemory = true;
	else
		ms_isHighTextureMemory = false;

	// D3D11 feature level 11_0 supports border addressing
	GRAPHICS_CAPS_CAN_NOT_TEXTURE_ADDRESS_BORDER = false;

	// D3D11 supports DXT
	ms_bSupportDXT = true;

	// CPU check
	int cpuInfo[4] = { 0 };
	__cpuid(cpuInfo, 1);

	CPU_HAS_SSE2 = cpuInfo[3] & (1 << 26);

	return (iRet);
}

void CGraphicDevice::__InitializePDTVertexBufferList()
{
	for (UINT i=0; i<PDT_VERTEXBUFFER_NUM; ++i)
		ms_alpd3dPDTVB[i]=NULL;	
}
		
void CGraphicDevice::__DestroyPDTVertexBufferList()
{
	for (UINT i=0; i<PDT_VERTEXBUFFER_NUM; ++i)
	{
		if (ms_alpd3dPDTVB[i])
		{
			ms_alpd3dPDTVB[i]->Release();
			ms_alpd3dPDTVB[i]=NULL;
		}
	}
}

bool CGraphicDevice::__CreatePDTVertexBufferList()
{
	if (!ms_lpd3d11Device)
		return false;

	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = sizeof(TPDTVertex) * PDT_VERTEX_NUM;
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	for (UINT i = 0; i < PDT_VERTEXBUFFER_NUM; ++i)
	{
		if (FAILED(ms_lpd3d11Device->CreateBuffer(&bd, NULL, &ms_alpd3dPDTVB[i])))
			return false;
	}
	return true;
}

bool CGraphicDevice::__CreateDefaultIndexBuffer(UINT eDefIB, UINT uIdxCount, const WORD* c_awIndices)
{
	assert(ms_alpd3dDefIB[eDefIB] == nullptr);

	if (uIdxCount == 0 || !c_awIndices)
		return false;

	_mgr->CreateIndexBuffer(ms_alpd3dDefIB[eDefIB], c_awIndices, uIdxCount);

	return true;
}

bool CGraphicDevice::__CreateDefaultIndexBufferList()
{
	static const WORD c_awLineIndices[2] = { 0, 1, };
	static const WORD c_awLineTriIndices[6] = { 0, 1, 0, 2, 1, 2, };
	static const WORD c_awLineRectIndices[8] = { 0, 1, 0, 2, 1, 3, 2, 3,};
	static const WORD c_awLineCubeIndices[24] = { 
		0, 1, 0, 2, 1, 3, 2, 3,
		0, 4, 1, 5, 2, 6, 3, 7,
		4, 5, 4, 6, 5, 7, 6, 7,
	};
	static const WORD c_awFillTriIndices[3]= { 0, 1, 2, };
	static const WORD c_awFillRectIndices[6] = { 0, 2, 1, 2, 3, 1, };
	static const WORD c_awFillCubeIndices[36] = { 
		0, 1, 2, 1, 3, 2,
		2, 0, 6, 0, 4, 6,
		0, 1, 4, 1, 5, 4,
		1, 3, 5, 3, 7, 5,
		3, 2, 7, 2, 6, 7,
		4, 5, 6, 5, 7, 6,
	};
	
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_LINE, 2, c_awLineIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_LINE_TRI, 6, c_awLineTriIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_LINE_RECT, 8, c_awLineRectIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_LINE_CUBE, 24, c_awLineCubeIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_FILL_TRI, 3, c_awFillTriIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_FILL_RECT, 6, c_awFillRectIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_FILL_CUBE, 36, c_awFillCubeIndices))
		return false;
	
	return true;
}

void CGraphicDevice::InitBackBufferCount(UINT uBackBufferCount)
{
	m_uBackBufferCount=uBackBufferCount;
}

void CGraphicDevice::Destroy()
{
	__DestroyPDTVertexBufferList();

	if (ms_hDC)
	{
		ReleaseDC(ms_hWnd, ms_hDC);
		ms_hDC = NULL;
	}

	safe_release(ms_ptVS);
	safe_release(ms_pntVS);
	safe_release(ms_pnt2VS);

	safe_release(ms_lpSphereMesh);
	safe_release(ms_lpCylinderMesh);

	if (m_pD3D11Renderer)
	{
		m_pD3D11Renderer->Destroy();
		delete m_pD3D11Renderer;
		m_pD3D11Renderer = NULL;
	}

	DestroyD3D11();

	safe_release(ms_lpd3dMatStack);

	if (m_pStateManager)
	{
		delete m_pStateManager;
		m_pStateManager = NULL;
	}

	__Initialize();
}

///////////////////////////////////////////////////////////////////////////////
// D3D11 Device Creation
///////////////////////////////////////////////////////////////////////////////
bool CGraphicDevice::CreateD3D11(HWND hWnd, int iHres, int iVres, bool bWindowed)
{
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 2;
	sd.BufferDesc.Width = iHres;
	sd.BufferDesc.Height = iVres;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = bWindowed ? TRUE : FALSE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL featureLevelOut;

	// Try with debug layer first (requires Graphics Tools optional Windows feature)
	UINT createFlags = D3D11_CREATE_DEVICE_DEBUG;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		createFlags,
		featureLevels, 1,
		D3D11_SDK_VERSION,
		&sd,
		&ms_lpd3d11SwapChain,
		&ms_lpd3d11Device,
		&featureLevelOut,
		&ms_lpd3d11Context
	);

	if (FAILED(hr))
	{
		Tracenf("D3D11: Debug layer not available (0x%08X), retrying without it", hr);
		createFlags = 0;
		hr = D3D11CreateDeviceAndSwapChain(
			NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
			createFlags, featureLevels, 1, D3D11_SDK_VERSION,
			&sd, &ms_lpd3d11SwapChain, &ms_lpd3d11Device,
			&featureLevelOut, &ms_lpd3d11Context
		);
	}

	if (FAILED(hr))
	{
		// Retry without FLIP_DISCARD (older Windows)
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;
		hr = D3D11CreateDeviceAndSwapChain(
			NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
			createFlags, featureLevels, 1, D3D11_SDK_VERSION,
			&sd, &ms_lpd3d11SwapChain, &ms_lpd3d11Device,
			&featureLevelOut, &ms_lpd3d11Context
		);
		if (FAILED(hr))
		{
			Tracenf("D3D11: Failed to create device (0x%08X)", hr);
			return false;
		}
	}

	if (createFlags & D3D11_CREATE_DEVICE_DEBUG)
	{
		Tracenf("D3D11: Debug layer enabled");

		if (SUCCEEDED(ms_lpd3d11Device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&s_pD3D11InfoQueue)))
		{
			s_pD3D11InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			s_pD3D11InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, FALSE);
			s_pD3D11InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, FALSE);

			D3D11_MESSAGE_SEVERITY denySeverities[] = {
				D3D11_MESSAGE_SEVERITY_INFO,
				D3D11_MESSAGE_SEVERITY_MESSAGE,
			};
			D3D11_MESSAGE_ID denyIDs[] = {
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
			};
			D3D11_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumSeverities = _countof(denySeverities);
			filter.DenyList.pSeverityList = denySeverities;
			filter.DenyList.NumIDs = _countof(denyIDs);
			filter.DenyList.pIDList = denyIDs;
			s_pD3D11InfoQueue->AddStorageFilterEntries(&filter);

			Tracenf("D3D11: InfoQueue configured (errors + warnings)");
		}
	}
	else
		Tracenf("D3D11: Running without debug layer (install 'Graphics Tools' Windows optional feature for shader debugging)");

	Tracenf("D3D11: Device created (Feature Level %X)", featureLevelOut);


	// Create render target view from back buffer
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = ms_lpd3d11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	if (FAILED(hr))
	{
		Tracenf("D3D11: Failed to get back buffer");
		return false;
	}

	hr = ms_lpd3d11Device->CreateRenderTargetView(pBackBuffer, NULL, &ms_lpd3d11RTV);
	pBackBuffer->Release();
	if (FAILED(hr))
	{
		Tracenf("D3D11: Failed to create render target view");
		return false;
	}

	// Create depth stencil texture and view
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = iHres;
	descDepth.Height = iVres;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	hr = ms_lpd3d11Device->CreateTexture2D(&descDepth, NULL, &ms_lpd3d11DepthStencil);
	if (FAILED(hr))
	{
		Tracenf("D3D11: Failed to create depth stencil texture");
		return false;
	}

	hr = ms_lpd3d11Device->CreateDepthStencilView(ms_lpd3d11DepthStencil, NULL, &ms_lpd3d11DSV);
	if (FAILED(hr))
	{
		Tracenf("D3D11: Failed to create depth stencil view");
		return false;
	}

	// Bind render targets
	ms_lpd3d11Context->OMSetRenderTargets(1, &ms_lpd3d11RTV, ms_lpd3d11DSV);

	// Set viewport
	D3D11_VIEWPORT vp = {};
	vp.Width = (float)iHres;
	vp.Height = (float)iVres;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	ms_lpd3d11Context->RSSetViewports(1, &vp);

	// init before CD3D11Renderer, because CD3D11Renderer will use DxManager
	m_mgr = std::make_unique<DxManager>(ms_lpd3d11Device, ms_lpd3d11Context);

	// Initialize the D3D11 renderer (shaders, constant buffers, state objects)
	if (!m_pD3D11Renderer)
		m_pD3D11Renderer = new CD3D11Renderer();

	if (!m_pD3D11Renderer->Initialize(ms_lpd3d11Device, ms_lpd3d11Context))
	{
		Tracenf("D3D11: Renderer initialization failed");
		delete m_pD3D11Renderer;
		m_pD3D11Renderer = NULL;
		return false;
	}

	m_pD3D11Renderer->SetScreenSize((float)iHres, (float)iVres);

	Tracenf("D3D11: Device ready (%dx%d)", iHres, iVres);
	return true;
}

void CGraphicDevice::DestroyD3D11()
{
	if (ms_lpd3d11Context)
		ms_lpd3d11Context->ClearState();

	safe_release(s_pD3D11InfoQueue);
	safe_release(ms_lpd3d11DSV);
	safe_release(ms_lpd3d11DepthStencil);
	safe_release(ms_lpd3d11RTV);
	safe_release(ms_lpd3d11SwapChain);
	safe_release(ms_lpd3d11Context);
	safe_release(ms_lpd3d11Device);
}

///////////////////////////////////////////////////////////////////////////////
// Drain D3D11 InfoQueue and print messages via Tracenf — call once per frame
///////////////////////////////////////////////////////////////////////////////
void CGraphicDevice::FlushD3D11DebugMessages()
{
	if (!s_pD3D11InfoQueue)
		return;

	UINT64 numMessages = s_pD3D11InfoQueue->GetNumStoredMessages();
	if (numMessages == 0)
		return;

	UINT64 cap = (numMessages < 50) ? numMessages : 50;

	for (UINT64 i = 0; i < cap; ++i)
	{
		SIZE_T msgSize = 0;
		if (FAILED(s_pD3D11InfoQueue->GetMessage(i, NULL, &msgSize)) || msgSize == 0)
			continue;

		std::vector<uint8_t> buf(msgSize);
		D3D11_MESSAGE* pMsg = (D3D11_MESSAGE*)buf.data();
		if (FAILED(s_pD3D11InfoQueue->GetMessage(i, pMsg, &msgSize)))
			continue;

		const char* sev = "INFO";
		switch (pMsg->Severity)
		{
		case D3D11_MESSAGE_SEVERITY_CORRUPTION:	sev = "CORRUPTION"; break;
		case D3D11_MESSAGE_SEVERITY_ERROR:		sev = "ERROR"; break;
		case D3D11_MESSAGE_SEVERITY_WARNING:	sev = "WARNING"; break;
		case D3D11_MESSAGE_SEVERITY_INFO:		sev = "INFO"; break;
		case D3D11_MESSAGE_SEVERITY_MESSAGE:	sev = "MSG"; break;
		}

		Tracenf("D3D11 %s: %.*s", sev, (int)pMsg->DescriptionByteLength, pMsg->pDescription);
	}

	s_pD3D11InfoQueue->ClearStoredMessages();
}
