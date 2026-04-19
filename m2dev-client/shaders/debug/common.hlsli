cbuffer cbPerFrame : register(b0)
{
	row_major float4x4 matWorld;
	row_major float4x4 matView;
	row_major float4x4 matProj;
};

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

cbuffer cbLighting : register(b2)
{
	float4 lightDir; 
	float4 lightDiffuse; 
	float4 lightAmbient;
	float4 matDiffuse; 
	float4 matAmbient; 
	float4 matEmissive;
	int lightingEnable; 
	int pad0; int pad1; int pad2;
};

cbuffer cbTexTransform : register(b3)
{
	row_major float4x4 matTexTransform0;
	row_major float4x4 matTexTransform1;
};

cbuffer cbFog : register(b4)
{
    float4 fogColor;
    float  fogStart;
    float  fogEnd;
    int    fogEnable;
    int    fogPad;
};

cbuffer CBScreenSize : register(b5)
{
    float screenWidth;
    float screenHeight;
    float padScreen0;
    float padScreen1;
};

float4 ResolveArg(int arg, float4 tex, float4 diffuse)
{
	if (arg == 3) return textureFactor;
	if (arg == 2) return tex;
	return diffuse;  // DIFFUSE(0) or CURRENT(1)
}
