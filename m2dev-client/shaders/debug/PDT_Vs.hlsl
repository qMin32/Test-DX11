#include "common.hlsli"

struct VS_INPUT  
{ 
	float3 pos : POSITION;
	float4 color : COLOR0; 
	float2 tex : TEXCOORD0; 
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;
	float viewDepth : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 worldPos = mul(float4(input.pos, 1.0f), matWorld);
	float4 viewPos = mul(worldPos, matView);
	output.pos = mul(viewPos, matProj);
	output.color = input.color;
	output.tex = input.tex;
	output.viewDepth = length(viewPos.xyz);
	return output;
}