#include "common.hlsli"


Texture2D    txWater : register(t0);
SamplerState sampler0 : register(s0);

struct PS_INPUT
{
    float4 pos       : SV_POSITION;
    float4 color     : COLOR0;
    float2 tex       : TEXCOORD0;
    float  viewDepth : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_Target
{
    float4 result;
    result.rgb = txWater.Sample(sampler0, input.tex).rgb; // SELECTARG1 = TEXTURE
    result.a   = input.color.a;                           // SELECTARG1 = DIFFUSE alpha

    if (fogEnable && fogEnd > fogStart)
    {
        float fogFactor = saturate((fogEnd - input.viewDepth) / (fogEnd - fogStart));
        result.rgb = lerp(fogColor.rgb, result.rgb, fogFactor);
    }

    return result;
}