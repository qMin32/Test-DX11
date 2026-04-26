#pragma once
#include "Core.h"
#include <unordered_map>
#include <string>
#include <cstdint>
#include <vector>

class ShadersContainer
{
public:
	ShadersContainer(DxManager* manager);

	ShaderPtr GetShader(ED3D11VertexFormat format, uint32_t flags);

private:
	void Precompile(ED3D11VertexFormat format, const std::vector<uint32_t>& flagCombinations);
	uint64_t MakeKey(ED3D11VertexFormat format, uint32_t flags) const;

	DxManager* m_manager = nullptr;

	struct ShaderDesc
	{
		std::string vs;
		std::string ps;
	};

	ShaderDesc m_desc[VF_COUNT];
	std::unordered_map<uint64_t, ShaderPtr> m_cache;
};
