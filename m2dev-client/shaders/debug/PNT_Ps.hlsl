#include "common.hlsli"


Texture2D txDiffuse0 : register(t0);
Texture2D txDiffuse1 : register(t1);
SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

struct PS_INPUT 
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;
	float viewDepth : TEXCOORD1;
	float2 tex1 : TEXCOORD2;
};

float4 main(PS_INPUT input) : SV_Target
{
	float4 texColor = float4(1, 1, 1, 1);
	if (useTexture0)
		texColor = txDiffuse0.Sample(sampler0, input.tex);

	float4 cArg1 = ResolveArg(colorArg10, texColor, input.color);
	float4 cArg2 = ResolveArg(colorArg20, texColor, input.color);

	float4 stage0;
	if (colorOp0 == 0) stage0.rgb = cArg1.rgb * cArg2.rgb;
	else if (colorOp0 == 1) stage0.rgb = cArg1.rgb;
	else if (colorOp0 == 2) stage0.rgb = cArg1.rgb + cArg2.rgb;
	else if (colorOp0 == 3) stage0.rgb = cArg2.rgb;
	else stage0.rgb = cArg1.rgb * cArg2.rgb;

	float aArg1 = ResolveArg(alphaArg10, texColor, input.color).a;
	float aArg2 = ResolveArg(alphaArg20, texColor, input.color).a;

	if (alphaOp0 == 0) stage0.a = aArg1 * aArg2;
	else if (alphaOp0 == 1) stage0.a = aArg1;
	else if (alphaOp0 == 3) stage0.a = aArg2;
	else stage0.a = texColor.a;
	
	float4 result = stage0;
	if (texCoordGen1 == 2 && useTexture1 && colorOp1 != -1)
	{
		float4 tex1 = txDiffuse1.Sample(sampler1, input.tex1);
		if (colorOp1 == 7) result.rgb = stage0.rgb + stage0.a * tex1.rgb; // MODULATEALPHA_ADDCOLOR
		else if (colorOp1 == 0) result.rgb = tex1.rgb * stage0.rgb;       // MODULATE
		else if (colorOp1 == 2) result.rgb = tex1.rgb + stage0.rgb;       // ADD

		if (alphaOp1 == 1) result.a = stage0.a;
		else if (alphaOp1 == 0) result.a = tex1.a * stage0.a;
	}

	if (alphaTestEnable)
	{
		float ref = (float)alphaRef / 255.0f;
		if (result.a < ref) discard;
	}

	if (fogEnable && fogEnd > fogStart)
	{
		float fogFactor = saturate((fogEnd - input.viewDepth) / (fogEnd - fogStart));
		result.rgb = lerp(fogColor.rgb, result.rgb, fogFactor);
	}

	return result;
}