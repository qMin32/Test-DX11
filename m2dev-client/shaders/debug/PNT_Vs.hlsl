cbuffer cbPerFrame : register(b0)
{
	row_major float4x4 matWorld; 
	row_major float4x4 matView; 
	row_major float4x4 matProj;
};
cbuffer cbMaterial : register(b1)
{
	float4 textureFactor;
	int useTexture0; int useTexture1;
	int colorOp0; int alphaOp0;
	int colorOp1; int alphaOp1;
	int colorArg10; int colorArg20;
	int alphaArg10; int alphaArg20;
	int colorArg11; int colorArg21;
	int alphaArg11; int alphaArg21;
	int alphaTestEnable; int alphaRef;
	int texCoordGen1; int padMat1; int padMat2; int padMat3;
};
cbuffer cbLighting : register(b2)
{
	float4 lightDir; 
	float4 lightDiffuse; 
	float4 lightAmbient;
	float4 matDiffuse;
	float4 matAmbient; 
	float4 matEmissive;
	int lightingEnable;
	int pad0;
	int pad1; 
	int pad2;
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
	float2 tex : TEXCOORD0;
};
struct VS_OUTPUT 
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;
	float viewDepth : TEXCOORD1;
	float2 tex1 : TEXCOORD2;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 worldPos = mul(float4(input.pos, 1.0f), matWorld);
	float4 viewPos = mul(worldPos, matView);
	output.pos = mul(viewPos, matProj);
	output.tex = input.tex;
	output.viewDepth = length(viewPos.xyz);

	float3 worldNormal = normalize(mul(input.normal, (float3x3)matWorld));

	if (lightingEnable)
	{
		float3 L = normalize(-lightDir.xyz);
		float NdotL = max(dot(worldNormal, L), 0.0f);

		float3 ambient = matAmbient.rgb * lightAmbient.rgb;
		if (length(ambient) < 0.001f)
			ambient = float3(0.5f, 0.5f, 0.5f);

		float3 diffuse = matDiffuse.rgb * lightDiffuse.rgb * NdotL;
		output.color.rgb = saturate(ambient + diffuse + matEmissive.rgb);
		output.color.a = (matDiffuse.a > 0.001f) ? matDiffuse.a : 1.0f;
	}
	else
	{
		output.color = float4(1, 1, 1, 1);
	}

	// Texture coordinate generation for stage 1 (specular sphere-map)
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
	else
	{
		output.tex1 = float2(0, 0);
	}

	return output;
}