cbuffer cbPerFrame : register(b0)
{
	row_major float4x4 matWorld;
	row_major float4x4 matView;
	row_major float4x4 matProj;
};
cbuffer cbLighting : register(b2)
{
	float4 lightDir; float4 lightDiffuse; float4 lightAmbient;
	float4 matDiffuse; float4 matAmbient; float4 matEmissive;
	int lightingEnable; int pad0; int pad1; int pad2;
};

cbuffer cbTexTransform : register(b3)
{
	row_major float4x4 matTexTransform0;
	row_major float4x4 matTexTransform1;
};

struct VS_INPUT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
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
	output.viewDepth = length(viewPos.xyz);

	if (lightingEnable)
	{
		float3 worldNormal = normalize(mul(input.normal, (float3x3)matWorld));
		float NdotL = max(dot(worldNormal, -lightDir.xyz), 0.0f);
		output.color = saturate(matDiffuse * lightDiffuse * NdotL + matAmbient * lightAmbient + matEmissive);
		output.color.a = matDiffuse.a;
	}
	else
	{
		output.color = float4(1, 1, 1, 1);
	}

	float4 tc0 = mul(float4(viewPos.xyz, 1.0f), matTexTransform0);
	float4 tc1 = mul(float4(viewPos.xyz, 1.0f), matTexTransform1);
	output.tex0 = tc0.xy;
	output.tex1 = tc1.xy;
	return output;
}