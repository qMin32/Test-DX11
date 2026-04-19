#include "common.hlsli"

struct VS_INPUT  
{ 
	float3 pos : POSITION; 
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT 
{
	float4 pos : SV_POSITION; 
	float2 tex0 : TEXCOORD0;
	float viewDepth : TEXCOORD1;
	float2 tex1 : TEXCOORD2;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 worldPos = mul(float4(input.pos, 1.0f), matWorld);
	float4 viewPos = mul(worldPos, matView);
	output.pos = mul(viewPos, matProj);
	output.tex0 = input.tex;
	output.viewDepth = length(viewPos.xyz);
	// Generate second texcoord from view-space position (D3DTSS_TCI_CAMERASPACEPOSITION emulation)
	float4 tc1 = mul(float4(viewPos.xyz, 1.0f), matTexTransform1);
	output.tex1 = tc1.xy;
	return output;
}