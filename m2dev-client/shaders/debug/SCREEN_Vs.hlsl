#include "common.hlsli"

struct VS_INPUT
{
	float4 pos : POSITION;		// x, y, z, rhw (screen space pixels)
	float4 color : COLOR0;		// diffuse
	float4 specular : COLOR1;	// specular (usually unused)
	float2 tex0 : TEXCOORD0;
	float2 tex1 : TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex0 : TEXCOORD0;
	float2 tex1 : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	// Convert screen-space pixels to NDC [-1,1]
	// D3D9 XYZRHW: x,y in pixels, z in [0,1], rhw = 1/w
	output.pos.x = (input.pos.x / screenWidth) * 2.0f - 1.0f;
	output.pos.y = 1.0f - (input.pos.y / screenHeight) * 2.0f;
	output.pos.z = input.pos.z;
	output.pos.w = 1.0f;
	output.color = input.color;
	output.tex0 = input.tex0;
	output.tex1 = input.tex1;
	return output;
}