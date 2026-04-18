#pragma once
#include "Core.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>

class CShaders
{
public:
	CShaders(ID3D11Device* device, const std::string& vsName, const std::string& psName);
	~CShaders();

	ID3D11VertexShader* GetVS() const;
	ID3D11PixelShader* GetPS() const;
	ID3D11InputLayout* GetInputLayout() const;

	void Set(ID3D11DeviceContext* context) const;

private:
	void Reload();

#ifdef _DEBUG
	void StartWatcher();
#endif

private:
	ID3D11Device* m_device = nullptr;

	ComPtr<ID3D11VertexShader> m_vs;
	ComPtr<ID3D11PixelShader> m_ps;
	ComPtr<ID3D11InputLayout> m_inputLayout;

	std::string m_vsName;
	std::string m_psName;

#ifdef _DEBUG
	std::thread m_watcherThread;
	std::atomic<bool> m_watcherRunning{ false };
	mutable std::atomic<bool> m_needReload{ false };
#endif
};