#include "common.hlsli"

cbuffer GrannyBonePalette : register(b8)
{
    row_major float4x4 bonePalette[256];
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float4 weights : BLENDWEIGHT;
    uint4 indices : BLENDINDICES;
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

static void SkinVertex(VS_INPUT input, out float4 skinnedPos, out float3 skinnedNormal)
{
    float4 p = float4(input.pos, 1.0f);
    float3 n = input.normal;

    skinnedPos =
        mul(p, bonePalette[input.indices.x]) * input.weights.x +
        mul(p, bonePalette[input.indices.y]) * input.weights.y +
        mul(p, bonePalette[input.indices.z]) * input.weights.z +
        mul(p, bonePalette[input.indices.w]) * input.weights.w;

    skinnedNormal =
        mul(n, (float3x3)bonePalette[input.indices.x]) * input.weights.x +
        mul(n, (float3x3)bonePalette[input.indices.y]) * input.weights.y +
        mul(n, (float3x3)bonePalette[input.indices.z]) * input.weights.z +
        mul(n, (float3x3)bonePalette[input.indices.w]) * input.weights.w;
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 skinnedPos;
    float3 skinnedNormal;
    SkinVertex(input, skinnedPos, skinnedNormal);

    float4 worldPos = mul(skinnedPos, matWorld);
    float4 viewPos = mul(worldPos, matView);
    output.pos = mul(viewPos, matProj);

    float3 worldNormal = normalize(mul(normalize(skinnedNormal), (float3x3)matWorld));

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

    if (texCoordGen1 == 2)
    {
        float3 viewNormal = normalize(mul(worldNormal, (float3x3)matView));
        float3 viewDir = normalize(viewPos.xyz);
        float3 reflVec = reflect(viewDir, viewNormal);
        float4 tc = mul(float4(reflVec, 1.0f), matTexTransform1);
        output.tex1 = tc.xy;
    }
    else if (texCoordGen1 == 1)
    {
        float4 tc = mul(float4(viewPos.xyz, 1.0f), matTexTransform1);
        output.tex1 = tc.xy;
    }
    else if (texCoordGen1 == 3)
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
