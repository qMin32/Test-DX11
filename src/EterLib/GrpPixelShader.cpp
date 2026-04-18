#include "StdAfx.h"
#include "GrpPixelShader.h"
#include "GrpD3DXBuffer.h"
#include "StateManager.h"

#include <utf8.h>

CPixelShader::CPixelShader()
{
	Initialize();
}

CPixelShader::~CPixelShader()
{
	Destroy();
}

void CPixelShader::Initialize()
{
	m_handle=0;
}

void CPixelShader::Destroy()
{
	if (m_handle)
	{
		m_handle->Release();
		m_handle=nullptr;
	}
}

bool CPixelShader::CreateFromDiskFile(const char* c_szFileName)
{
    // Legacy D3D9 .psh assembly path — unused. D3D11 shaders are managed by CD3D11Renderer.
    (void)c_szFileName;
    return false;
}

void CPixelShader::Set()
{
	STATEMANAGER.SetPixelShader(m_handle);
}
