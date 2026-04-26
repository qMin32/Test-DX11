#pragma once
#include "Core.h"
#include <string>
#include <thread>
#include <atomic>
#include <cstdint>

class CShaders
{
public:
	CShaders(ID3D11Device* device, const std::string& vsName, const std::string& psName, uint32_t shaderFlags);
	~CShaders();

	ID3D11VertexShader* GetVS() const;
	ID3D11PixelShader* GetPS() const;
	ID3D11InputLayout* GetInputLayout() const;
	bool IsValid() const { return m_vs != nullptr && m_ps != nullptr && m_inputLayout != nullptr; }
	void Set(ID3D11DeviceContext* context) const;
	void Reload();

private:
	ID3D11Device* m_device = nullptr;

	std::string m_vsName;
	std::string m_psName;

	uint32_t m_shaderFlags = 0;

	ComPtr<ID3D11VertexShader> m_vs;
	ComPtr<ID3D11PixelShader> m_ps;
	ComPtr<ID3D11InputLayout> m_inputLayout;

#ifdef _DEBUG
	void StartWatcher();

	std::thread m_watcherThread;
	std::atomic<bool> m_watcherRunning = false;
	mutable std::atomic<bool> m_needReload = false;
#endif
};
