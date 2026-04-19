#include "common.hlsli"

struct VS_INPUT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex0 : TEXCOORD0;
	float2 tex1 : TEXCOORD1;
};
struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex0 : TEXCOORD0;
	float2 tex1 : TEXCOORD1;
	float viewDepth : TEXCOORD2;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 worldPos = mul(float4(input.pos, 1.0f), matWorld);
	float4 viewPos = mul(worldPos, matView);
	output.pos = mul(viewPos, matProj);

	float3 worldNormal = normalize(mul(input.normal, (float3x3)matWorld));

	if (lightingEnable)
	{
		float NdotL = max(dot(worldNormal, -lightDir.xyz), 0.0f);
		output.color = saturate(matDiffuse * lightDiffuse * NdotL + matAmbient * lightAmbient + matEmissive);
		output.color.a = matDiffuse.a;
	}
	else
	{
		output.color = float4(1, 1, 1, 1);
	}

	output.tex0 = input.tex0;

	// Texture coordinate generation for stage 1
	if (texCoordGen1 == 2) // Camera-space reflection vector
	{
		float3 viewNormal = normalize(mul(worldNormal, (float3x3)matView));
		float3 viewDir = normalize(viewPos.xyz);
		float3 reflVec = reflect(viewDir, viewNormal);
		float4 tc = mul(float4(reflVec, 1.0f), matTexTransform1);
		output.tex1 = tc.xy;
	}
	else if (texCoordGen1 == 1) // Camera-space position
	{
		float4 tc = mul(float4(viewPos.xyz, 1.0f), matTexTransform1);
		output.tex1 = tc.xy;
	}
	else if (texCoordGen1 == 3) // Camera-space normal
	{
		float3 viewNormal = normalize(mul(worldNormal, (float3x3)matView));
		float4 tc = mul(float4(viewNormal, 1.0f), matTexTransform1);
		output.tex1 = tc.xy;
	}
	else
	{
		output.tex1 = input.tex1;
	}

	output.viewDepth = length(viewPos.xyz);
	return output;
}