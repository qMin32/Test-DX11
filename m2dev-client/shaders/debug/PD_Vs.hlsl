cbuffer cbPerFrame : register(b0)
{
    row_major float4x4 matWorld;
    row_major float4x4 matView;
    row_major float4x4 matProj;
};

cbuffer cbTexTransform : register(b3)
{
    row_major float4x4 matTexTransform;
};

struct VS_INPUT
{
    float3 pos   : POSITION;
    float4 color : COLOR0;
};

struct VS_OUTPUT
{
    float4 pos       : SV_POSITION;
    float4 color     : COLOR0;
    float2 tex       : TEXCOORD0;
    float  viewDepth : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    float4 worldPos  = mul(float4(input.pos, 1.0f), matWorld);
    float4 viewPos   = mul(worldPos, matView);
    output.pos       = mul(viewPos, matProj);
    output.color     = input.color;
    output.viewDepth = viewPos.z;

    float4 texCoord  = mul(viewPos, matTexTransform);
    output.tex       = texCoord.xy;

    return output;
}