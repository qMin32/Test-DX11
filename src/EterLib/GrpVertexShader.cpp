#include "StdAfx.h"
#include "GrpVertexShader.h"
#include "GrpD3DXBuffer.h"
#include "StateManager.h"

#include <utf8.h>

CVertexShader::CVertexShader()
{
	Initialize();
}

CVertexShader::~CVertexShader()
{
	Destroy();
}

void CVertexShader::Initialize()
{
	m_handle=0;
}

void CVertexShader::Destroy()
{
	if (m_handle)
	{
		m_handle->Release();
		m_handle = nullptr;
	}
}

bool CVertexShader::CreateFromDiskFile(const char* c_szFileName, const DWORD* c_pdwVertexDecl)
{
    // Legacy D3D9 .vsh assembly path — unused. D3D11 shaders are managed by CD3D11Renderer.
    (void)c_szFileName; (void)c_pdwVertexDecl;
    return false;
}

void CVertexShader::Set()
{
	STATEMANAGER.SetVertexShader(m_handle);
}
