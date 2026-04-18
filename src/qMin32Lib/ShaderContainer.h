#pragma once
#include "Core.h"

class ShadersContainer
{
public:
	ShadersContainer(DxManager* manager);

	ShaderPtr GetShader(ED3D11VertexFormat format) const;

private:
	ShaderPtr m_shader[VF_COUNT];
};