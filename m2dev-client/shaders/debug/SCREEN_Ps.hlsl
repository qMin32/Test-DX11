cbuffer cbMaterial : register(b1)
{
	float4 textureFactor;
	int useTexture0;
	int useTexture1;
	int colorOp0;
	int alphaOp0;
	int colorOp1;
	int alphaOp1;
	int colorArg10;
	int colorArg20;
	int alphaArg10;
	int alphaArg20;
	int colorArg11;
	int colorArg21;
	int alphaArg11;
	int alphaArg21;
	int alphaTestEnable;
	int alphaRef;
	int texCoordGen1;
	int padMat1;
	int padMat2;
	int padMat3;
};

Texture2D txDiffuse0 : register(t0);
Texture2D txDiffuse1 : register(t1);
SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex0 : TEXCOORD0;
	float2 tex1 : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_Target
{
	float4 tex0 = useTexture0 ? txDiffuse0.Sample(sampler0, input.tex0) : float4(1,1,1,1);
	float4 tex1 = useTexture1 ? txDiffuse1.Sample(sampler1, input.tex1) : float4(1,1,1,1);

	float4 stage0 = tex0 * input.color;

	if (colorOp0 == 1)
	{
		if (colorArg10 == 3) stage0.rgb = textureFactor.rgb;
		else if (colorArg10 == 0 || colorArg10 == 1) stage0.rgb = input.color.rgb;
		else stage0.rgb = tex0.rgb;
	}
	else if (colorOp0 == 3)
	{
		if (colorArg20 == 3) stage0.rgb = textureFactor.rgb;
		else if (colorArg20 == 0 || colorArg20 == 1) stage0.rgb = input.color.rgb;
		else stage0.rgb = tex0.rgb;
	}
	else if (colorOp0 == 2)
	{
		float3 a = (colorArg10 == 3) ? textureFactor.rgb : ((colorArg10 == 0 || colorArg10 == 1) ? input.color.rgb : tex0.rgb);
		float3 b = (colorArg20 == 3) ? textureFactor.rgb : ((colorArg20 == 0 || colorArg20 == 1) ? input.color.rgb : tex0.rgb);
		stage0.rgb = a + b;
	}

	if (alphaOp0 == 1)
		stage0.a = (alphaArg10 == 3) ? textureFactor.a : ((alphaArg10 == 0 || alphaArg10 == 1) ? input.color.a : tex0.a);
	else if (alphaOp0 == 3)
		stage0.a = (alphaArg20 == 3) ? textureFactor.a : ((alphaArg20 == 0 || alphaArg20 == 1) ? input.color.a : tex0.a);
	else if (alphaOp0 == 0)
	{
		float a = (alphaArg10 == 3) ? textureFactor.a : ((alphaArg10 == 0 || alphaArg10 == 1) ? input.color.a : tex0.a);
		float b = (alphaArg20 == 3) ? textureFactor.a : ((alphaArg20 == 0 || alphaArg20 == 1) ? input.color.a : tex0.a);
		stage0.a = a * b;
	}

	float4 result = stage0;

	if (colorOp1 == 0) result.rgb = tex1.rgb * stage0.rgb;
	else if (colorOp1 == 1) result.rgb = (colorArg11 == 1) ? stage0.rgb : tex1.rgb;
	else if (colorOp1 == 2) result.rgb = tex1.rgb + stage0.rgb;
	else if (colorOp1 == 3) result.rgb = (colorArg21 == 1) ? stage0.rgb : tex1.rgb;
	else if (colorOp1 == 7) result.rgb = stage0.rgb + stage0.a * tex1.rgb; // MODULATEALPHA_ADDCOLOR

	if (alphaOp1 == 1) result.a = (alphaArg11 == 1) ? stage0.a : tex1.a;
	else if (alphaOp1 == 0) result.a = ((alphaArg11 == 1) ? stage0.a : tex1.a) * ((alphaArg21 == 1) ? stage0.a : tex1.a);
	else if (alphaOp1 == 3) result.a = (alphaArg21 == 1) ? stage0.a : tex1.a;
	else if (alphaOp1 == -1) result.a = stage0.a;

	if (alphaTestEnable)
	{
		float ref = (float)alphaRef / 255.0f;
		if (result.a < ref) discard;
	}

	return result;
}