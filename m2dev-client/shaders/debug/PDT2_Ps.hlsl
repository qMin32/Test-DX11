#include "common.hlsli"

Texture2D txDiffuse0 : register(t0);
Texture2D txDiffuse1 : register(t1);
SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

struct PS_INPUT { float4 pos : SV_POSITION; float4 color : COLOR0; float2 tex0 : TEXCOORD0; float2 tex1 : TEXCOORD1; float viewDepth : TEXCOORD2; };

// D3DTA values: 0=DIFFUSE, 1=CURRENT, 2=TEXTURE, 3=TFACTOR
float4 ResolveArg0(int arg, float4 tex, float4 diffuse)
{
	if (arg == 3) return textureFactor;
	if (arg == 2) return tex;
	return diffuse;
}

float4 main(PS_INPUT input) : SV_Target
{
	float4 tex0 = float4(1, 1, 1, 1);
	float4 tex1 = float4(1, 1, 1, 1);
	if (useTexture0) tex0 = txDiffuse0.Sample(sampler0, input.tex0);
	if (useTexture1) tex1 = txDiffuse1.Sample(sampler1, input.tex1);

	// Stage 0: resolve args
	float4 cArg1 = ResolveArg0(colorArg10, tex0, input.color);
	float4 cArg2 = ResolveArg0(colorArg20, tex0, input.color);

	float4 stage0;
	if (colorOp0 == 0) stage0.rgb = cArg1.rgb * cArg2.rgb;
	else if (colorOp0 == 1) stage0.rgb = cArg1.rgb;
	else if (colorOp0 == 3) stage0.rgb = cArg2.rgb;
	else stage0.rgb = cArg1.rgb * cArg2.rgb;

	float aArg1 = ResolveArg0(alphaArg10, tex0, input.color).a;
	float aArg2 = ResolveArg0(alphaArg20, tex0, input.color).a;

	if (alphaOp0 == 0) stage0.a = aArg1 * aArg2;
	else if (alphaOp0 == 1) stage0.a = aArg1;
	else if (alphaOp0 == 3) stage0.a = aArg2;
	else stage0.a = tex0.a;

	// Stage 1: resolve args and combine with second texture
	float4 cArg1s1 = (colorArg11 == 2) ? tex1 : ((colorArg11 == 1) ? stage0 : tex1);
	float4 cArg2s1 = (colorArg21 == 1) ? stage0 : ((colorArg21 == 2) ? tex1 : stage0);

	float4 result;
	if (colorOp1 == 0) result = float4(cArg1s1.rgb * cArg2s1.rgb, stage0.a);
	else if (colorOp1 == 1) result = float4(cArg1s1.rgb, stage0.a);
	else if (colorOp1 == 2) result = float4(cArg1s1.rgb + cArg2s1.rgb, stage0.a);
	else if (colorOp1 == 7) result = float4(stage0.rgb + stage0.a * tex1.rgb, stage0.a); // MODULATEALPHA_ADDCOLOR
	else result = stage0;

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