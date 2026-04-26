#include "common.hlsli"

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
#ifdef IS_SKINNED
    float4 weights : BLENDWEIGHT;
    uint4  indices : BLENDINDICES;
#endif
    float2 tex0 : TEXCOORD0;
#ifdef HAS_TEX2
    float2 tex1   : TEXCOORD1;
#endif
};

#ifdef HAS_TEX2
struct VS_OUTPUT
{
    float4 pos       : SV_POSITION;
    float4 color     : COLOR0;
    float2 tex0      : TEXCOORD0;
    float2 tex1      : TEXCOORD1;
    float  viewDepth : TEXCOORD2;
};
#else
struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex0 : TEXCOORD0;
    float viewDepth : TEXCOORD1;
    float2 tex1 : TEXCOORD2;
};
#endif

#ifdef IS_SKINNED
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
#endif

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

#ifdef IS_SKINNED
    float4 localPos;
    float3 localNormal;
    SkinVertex(input, localPos, localNormal);
    localNormal = normalize(localNormal);
#else
    float4 localPos = float4(input.pos, 1.0f);
    float3 localNormal = input.normal;
#endif

    float4 worldPos = mul(localPos, matWorld);
    float4 viewPos = mul(worldPos, matView);
    output.pos = mul(viewPos, matProj);
    output.viewDepth = length(viewPos.xyz);
    output.tex0 = input.tex0;

    float3 worldNormal = normalize(mul(localNormal, (float3x3) matWorld));

    if (lightingEnable)
    {
#ifdef HAS_TEX2
        float NdotL = max(dot(worldNormal, -lightDir.xyz), 0.0f);
        output.color = saturate(matDiffuse * lightDiffuse * NdotL + matAmbient * lightAmbient + matEmissive);
        output.color.a = matDiffuse.a;
#else
        float3 L = normalize(-lightDir.xyz);
        float NdotL = max(dot(worldNormal, L), 0.0f);

        float3 ambient = matAmbient.rgb * lightAmbient.rgb;
        if (length(ambient) < 0.001f)
            ambient = float3(0.5f, 0.5f, 0.5f);

        float3 diffuse = matDiffuse.rgb * lightDiffuse.rgb * NdotL;
        output.color.rgb = saturate(ambient + diffuse + matEmissive.rgb);
        output.color.a = (matDiffuse.a > 0.001f) ? matDiffuse.a : 1.0f;
#endif
    }
    else
    {
        output.color = float4(1, 1, 1, 1);
    }

#ifdef HAS_TEX2
    if (texCoordGen1 == 2)
    {
        float3 viewNormal = normalize(mul(worldNormal, (float3x3)matView));
        float3 viewDir    = normalize(viewPos.xyz);
        float3 reflVec    = reflect(viewDir, viewNormal);
        float4 tc         = mul(float4(reflVec, 1.0f), matTexTransform1);
        output.tex1 = tc.xy;
    }
    else if (texCoordGen1 == 1)
    {
        float4 tc   = mul(float4(viewPos.xyz, 1.0f), matTexTransform1);
        output.tex1 = tc.xy;
    }
    else if (texCoordGen1 == 3)
    {
        float3 viewNormal = normalize(mul(worldNormal, (float3x3)matView));
        float4 tc         = mul(float4(viewNormal, 1.0f), matTexTransform1);
        output.tex1 = tc.xy;
    }
    else
    {
        output.tex1 = input.tex1;
    }
#else
    if (texCoordGen1 == 2)
    {
        float3 viewNormal = normalize(mul(worldNormal, (float3x3) matView));
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
#endif

    return output;
}
