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
    output.tex = input.tex;
    output.viewDepth = length(viewPos.xyz);

    float3 worldNormal = normalize(mul(normalize(skinnedNormal), (float3x3)matWorld));

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
    else
    {
        output.tex1 = float2(0, 0);
    }

    return output;
}
