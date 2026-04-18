cbuffer cbMaterial : register(b1)
{
	float4 textureFactor;
	int useTexture0; int useTexture1;
	int colorOp0;    int alphaOp0;
	int colorOp1;    int alphaOp1;
	int colorArg10;  int colorArg20;
	int alphaArg10;  int alphaArg20;
	int colorArg11;  int colorArg21;
	int alphaArg11;  int alphaArg21;
	int alphaTestEnable; int alphaRef;
	int texCoordGen1; int padMat1; int padMat2; int padMat3;
};
cbuffer cbFog : register(b4) { float4 fogColor; float fogStart; float fogEnd; int fogEnable; int fogPad; };

Texture2D txDiffuse0 : register(t0);
Texture2D txDiffuse1 : register(t1);
SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

struct PS_INPUT { float4 pos : SV_POSITION; float2 tex0 : TEXCOORD0; float viewDepth : TEXCOORD1; float2 tex1 : TEXCOORD2; };

// Helper: resolve a texture stage arg to a color value
// arg values: 0=DIFFUSE, 1=CURRENT, 2=TEXTURE, 3=TFACTOR
float4 ResolveArg(int arg, float4 tex, float4 current)
{
	if (arg == 3) return textureFactor;
	if (arg == 2) return tex;
	return current;
}

float4 main(PS_INPUT input) : SV_Target
{
	float4 tex0 = float4(1, 1, 1, 1);
	if (useTexture0)
		tex0 = txDiffuse0.Sample(sampler0, input.tex0);

	// Stage 0: apply color/alpha ops with arg resolution (supports TFACTOR for effects)
	float4 result = tex0;
	{
		float4 cArg1 = ResolveArg(colorArg10, tex0, tex0);
		float4 cArg2 = ResolveArg(colorArg20, tex0, tex0);
		if (colorOp0 == 0) result.rgb = cArg1.rgb * cArg2.rgb;
		else if (colorOp0 == 1) result.rgb = cArg1.rgb;
		else if (colorOp0 == 2) result.rgb = cArg1.rgb + cArg2.rgb;
		else if (colorOp0 == 3) result.rgb = cArg2.rgb;
		else if (colorOp0 == 4) result.rgb = cArg1.rgb * cArg2.rgb * 2.0f;
		else if (colorOp0 == 5) result.rgb = cArg1.rgb * cArg2.rgb * 4.0f;
		else if (colorOp0 == 6) result.rgb = lerp(cArg2.rgb, cArg1.rgb, tex0.a);
		else result.rgb = tex0.rgb;

		float aArg1 = ResolveArg(alphaArg10, tex0, tex0).a;
		float aArg2 = ResolveArg(alphaArg20, tex0, tex0).a;
		if (alphaOp0 == 0) result.a = aArg1 * aArg2;
		else if (alphaOp0 == 1) result.a = aArg1;
		else if (alphaOp0 == 3) result.a = aArg2;
		else if (alphaOp0 == 4) result.a = saturate(aArg1 * aArg2 * 2.0f);
		else if (alphaOp0 == 5) result.a = saturate(aArg1 * aArg2 * 4.0f);
		else result.a = tex0.a;
	}

	// Stage 1: apply second texture (e.g. minimap circular mask)
	if (useTexture1)
	{
		float4 tex1 = txDiffuse1.Sample(sampler1, input.tex1);
		if (colorOp1 == 0) result.rgb = tex1.rgb * result.rgb;
		else if (colorOp1 == 1) result.rgb = tex1.rgb;
		else if (colorOp1 == 2) result.rgb = tex1.rgb + result.rgb;
		else if (colorOp1 == 7) result.rgb = result.rgb + result.a * tex1.rgb; // MODULATEALPHA_ADDCOLOR

		if (alphaOp1 == 1) result.a = tex1.a;
		else if (alphaOp1 == 0) result.a = tex1.a * result.a;
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