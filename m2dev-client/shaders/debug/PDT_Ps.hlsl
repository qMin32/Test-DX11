#include "common.hlsli"

Texture2D txDiffuse0 : register(t0);
SamplerState sampler0 : register(s0);

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;
	float viewDepth : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_Target
{
	float4 texColor = float4(1, 1, 1, 1);
	if (useTexture0)
		texColor = txDiffuse0.Sample(sampler0, input.tex);

	// Resolve stage 0 color args
    float4 cArg1 = ResolveArg(colorArg10, texColor, input.color);
    float4 cArg2 = ResolveArg(colorArg20, texColor, input.color);

	float4 result;
	if (colorOp0 == 0) result.rgb = cArg1.rgb * cArg2.rgb;			// MODULATE
	else if (colorOp0 == 1) result.rgb = cArg1.rgb;				// SELECTARG1
	else if (colorOp0 == 2) result.rgb = cArg1.rgb + cArg2.rgb;	// ADD
	else if (colorOp0 == 3) result.rgb = cArg2.rgb;				// SELECTARG2
	else if (colorOp0 == 4) result.rgb = cArg1.rgb * cArg2.rgb * 2.0f;	// MODULATE2X
	else result.rgb = cArg1.rgb * cArg2.rgb;

	// Resolve stage 0 alpha args
	float aArg1 = ResolveArg(alphaArg10, texColor, input.color).a;
	float aArg2 = ResolveArg(alphaArg20, texColor, input.color).a;

	// Alpha — alphaOp == -1 (DISABLE) means use texture alpha as-is
	if (alphaOp0 == 0) result.a = aArg1 * aArg2;				// MODULATE
	else if (alphaOp0 == 1) result.a = aArg1;					// SELECTARG1
	else if (alphaOp0 == 3) result.a = aArg2;					// SELECTARG2
	else result.a = texColor.a;									// DISABLE → texture alpha

	// Alpha test
	if (alphaTestEnable)
	{
		float ref = (float)alphaRef / 255.0f;
		if (result.a < ref) discard;
	}

	// Fog
	if (fogEnable && fogEnd > fogStart)
	{
		float fogFactor = saturate((fogEnd - input.viewDepth) / (fogEnd - fogStart));
		result.rgb = lerp(fogColor.rgb, result.rgb, fogFactor);
	}

	return result;
	//return float4(1.0f,1.0f,1.0f,1.0f);
}