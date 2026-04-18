#include "pch.h"
#include "ShaderContainer.h"
#include "DxManager.h"
#include "Shaders.h"

ShadersContainer::ShadersContainer(DxManager* manager)
{
	if (!manager)
		return;

	manager->CreateShader(m_shader[VF_PDT], "PDT_Vs", "PDT_Ps");
	manager->CreateShader(m_shader[VF_PNT], "PNT_Vs", "PNT_Ps");
	manager->CreateShader(m_shader[VF_PD], "PD_Vs", "PD_Ps");
	manager->CreateShader(m_shader[VF_PNT2], "PNT2_Vs", "PNT2_Ps");
	manager->CreateShader(m_shader[VF_PN], "PN_Vs", "PN_Ps");
	manager->CreateShader(m_shader[VF_SCREEN], "SCREEN_Vs", "SCREEN_Ps");
	manager->CreateShader(m_shader[VF_PT], "PT_Vs", "PT_Ps");
	manager->CreateShader(m_shader[VF_PDT2], "PDT2_Vs", "PDT2_Ps");
}

ShaderPtr ShadersContainer::GetShader(ED3D11VertexFormat format) const
{
    return m_shader[format];
}
