#include "pch.h"
#include "Shaders.h"
#include <d3dcompiler.h>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>

#pragma comment(lib, "d3dcompiler.lib")
#include "ShadersHelper.h"

CShaders::CShaders(ID3D11Device* device, const std::string& vsName, const std::string& psName)
	: m_device(device), m_vsName(vsName), m_psName(psName)
{
	if (!m_vsName.empty())
		LoadVertexShader(m_device, m_vsName, "vs_5_0", m_vs, m_inputLayout);

	if (!m_psName.empty())
		LoadPixelShader(m_device, m_psName, "ps_5_0", m_ps);
#ifdef CECK_ERRORS
	if (!m_vs)
		WriteShaderError("VS FAILED: " + m_vsName);
	if (!m_ps)
		WriteShaderError("PS FAILED: " + m_psName);
	if (!m_inputLayout)
		WriteShaderError("INPUT LAYOUT FAILED: " + m_vsName);
#endif
#ifdef _DEBUG
	StartWatcher();
#endif
}

CShaders::~CShaders()
{
#ifdef _DEBUG
	m_watcherRunning = false;
	if (m_watcherThread.joinable())
		m_watcherThread.join();
#endif
}

ID3D11VertexShader* CShaders::GetVS() const
{
#ifdef _DEBUG
	if (m_needReload.exchange(false))
		const_cast<CShaders*>(this)->Reload();
#endif
	return m_vs.Get();
}

ID3D11PixelShader* CShaders::GetPS() const
{
#ifdef _DEBUG
	if (m_needReload.exchange(false))
		const_cast<CShaders*>(this)->Reload();
#endif
	return m_ps.Get();
}

ID3D11InputLayout* CShaders::GetInputLayout() const
{
#ifdef _DEBUG
	if (m_needReload.exchange(false))
		const_cast<CShaders*>(this)->Reload();
#endif
	return m_inputLayout.Get();
}

void CShaders::Set(ID3D11DeviceContext* context) const
{
	if (!context)
		return;

#ifdef _DEBUG
	if (m_needReload.exchange(false))
		const_cast<CShaders*>(this)->Reload();
#endif

	context->IASetInputLayout(m_inputLayout.Get());
	context->VSSetShader(m_vs.Get(), nullptr, 0);
	context->PSSetShader(m_ps.Get(), nullptr, 0);
}


void CShaders::Reload()
{
	if (!m_vsName.empty())
		LoadVertexShader(m_device, m_vsName, "vs_5_0", m_vs, m_inputLayout);

	if (!m_psName.empty())
		LoadPixelShader(m_device, m_psName, "ps_5_0", m_ps);
#ifdef CECK_ERRORS
	WriteShaderError("Reloaded: " + m_vsName + " / " + m_psName);
#endif
}

#ifdef _DEBUG
void CShaders::StartWatcher()
{
	m_watcherRunning = true;
	m_watcherThread = std::thread([this]()
		{
			auto getModTime = [](const std::string& path) -> FILETIME
				{
					FILETIME ft = {};
					HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
					if (h != INVALID_HANDLE_VALUE)
					{
						GetFileTime(h, nullptr, nullptr, &ft);
						CloseHandle(h);
					}
					return ft;
				};

			const std::string vsPath = BuildShaderPath(m_vsName);
			const std::string psPath = BuildShaderPath(m_psName);

			FILETIME ftVS = getModTime(vsPath);
			FILETIME ftPS = getModTime(psPath);

			while (m_watcherRunning)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

				FILETIME newVS = getModTime(vsPath);
				FILETIME newPS = getModTime(psPath);

				if (CompareFileTime(&newVS, &ftVS) != 0 || CompareFileTime(&newPS, &ftPS) != 0)
				{
					ftVS = newVS;
					ftPS = newPS;
					m_needReload = true;
				}
			}
		});
}
#endif