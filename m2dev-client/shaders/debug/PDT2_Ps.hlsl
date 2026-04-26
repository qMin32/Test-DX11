#include "common.hlsli"

Texture2D txDiffuse0 : register(t0);
SamplerState sampler0 : register(s0);

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex0 : TEXCOORD0;
	float viewDepth : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_Target
{
	float4 texColor = float4(1, 1, 1, 1);

	if (useTexture0)
		texColor = txDiffuse0.Sample(sampler0, input.tex0);

	float4 result = texColor * input.color;

	if (alphaTestEnable)
	{
		float ref = (float)alphaRef / 255.0f;
		if (result.a < ref)
			discard;
	}

	if (fogEnable && fogEnd > fogStart)
	{
		float fogFactor = saturate((fogEnd - input.viewDepth) / (fogEnd - fogStart));
		result.rgb = lerp(fogColor.rgb, result.rgb, fogFactor);
	}

	return result;
}