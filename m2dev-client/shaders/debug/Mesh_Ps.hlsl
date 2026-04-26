#include "common.hlsli"

Texture2D txDiffuse0 : register(t0);
Texture2D txDiffuse1 : register(t1);
SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

#ifdef HAS_TEX2
struct PS_INPUT
{
    float4 pos       : SV_POSITION;
    float4 color     : COLOR0;
    float2 tex0      : TEXCOORD0;
    float2 tex1      : TEXCOORD1;
    float  viewDepth : TEXCOORD2;
};
#else
struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex0 : TEXCOORD0;
    float viewDepth : TEXCOORD1;
    float2 tex1 : TEXCOORD2;
};
#endif

float4 main(PS_INPUT input) : SV_Target
{
    float4 texColor0 = float4(1, 1, 1, 1);
    float4 texColor1 = float4(1, 1, 1, 1);

    if (useTexture0)
        texColor0 = txDiffuse0.Sample(sampler0, input.tex0);
    if (useTexture1)
        texColor1 = txDiffuse1.Sample(sampler1, input.tex1);

    // Stage 0
    float4 cArg1 = ResolveArg(colorArg10, texColor0, input.color);
    float4 cArg2 = ResolveArg(colorArg20, texColor0, input.color);

    float4 stage0;
    if (colorOp0 == 0)
        stage0.rgb = cArg1.rgb * cArg2.rgb;
    else if (colorOp0 == 1)
        stage0.rgb = cArg1.rgb;
    else if (colorOp0 == 2)
        stage0.rgb = cArg1.rgb + cArg2.rgb;
    else if (colorOp0 == 3)
        stage0.rgb = cArg2.rgb;
    else
        stage0.rgb = cArg1.rgb * cArg2.rgb;

    float aArg1 = ResolveArg(alphaArg10, texColor0, input.color).a;
    float aArg2 = ResolveArg(alphaArg20, texColor0, input.color).a;

    if (alphaOp0 == 0)
        stage0.a = aArg1 * aArg2;
    else if (alphaOp0 == 1)
        stage0.a = aArg1;
    else if (alphaOp0 == 3)
        stage0.a = aArg2;
    else
        stage0.a = texColor0.a;

    // Stage 1
    float4 result = stage0;
#ifdef HAS_TEX2
    if (useTexture1 && colorOp1 >= 0)
#else
    if (texCoordGen1 == 2 && useTexture1 && colorOp1 >= 0)
#endif
    {
        if (colorOp1 == 0)
            result.rgb = texColor1.rgb * stage0.rgb;
        else if (colorOp1 == 1)
            result.rgb = texColor1.rgb;
        else if (colorOp1 == 2)
            result.rgb = texColor1.rgb + stage0.rgb;
        else if (colorOp1 == 7)
            result.rgb = stage0.rgb + stage0.a * texColor1.rgb;

        if (alphaOp1 == 1)
            result.a = stage0.a;
        else if (alphaOp1 == 0)
            result.a = texColor1.a * stage0.a;
    }

    // Alpha test
    if (alphaTestEnable)
    {
        float ref = (float) alphaRef / 255.0f;
        if (result.a < ref)
            discard;
    }

    // Fog
    if (fogEnable && fogEnd > fogStart)
    {
        float fogFactor = saturate((fogEnd - input.viewDepth) / (fogEnd - fogStart));
        result.rgb = lerp(fogColor.rgb, result.rgb, fogFactor);
    }

    return result;
}
