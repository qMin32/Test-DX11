#pragma once

///////////////////////////////////////////////////////////////////////////////
// D3D11 HLSL shaders — fixed-function pipeline emulation
//
// These shaders replace the D3D9 fixed-function pipeline. The game uses
// D3D9 render states (alpha blend, alpha test, fog, lighting) and texture
// stage states (COLOROP, ALPHAOP, etc.) which don't exist in D3D11.
// Instead, we emulate them via constant buffers and shader logic.
//
// Vertex formats handled:
//   VF_PDT    — Position + Diffuse + TexCoord (UI, 2D quads)
//   VF_PNT    — Position + Normal + TexCoord (3D models, characters)
//   VF_PD     — Position + Diffuse (debug lines, wireframes)
//   VF_PNT2   — Position + Normal + TexCoord + TexCoord2 (terrain multi-tex)
//   VF_PN     — Position + Normal (terrain HTP, camera-space tex gen)
//   VF_SCREEN — Pre-transformed XYZRHW + Diffuse + Specular + Tex2 (terrain STP)
//
// Constant buffers:
//   b0: CBPerFrame   — World/View/Proj matrices
//   b1: CBMaterial    — Texture factor, color/alpha ops, alpha test
//   b2: CBLighting    — Light direction, color, ambient, material diffuse
//   b3: CBTexTransform — Texture coordinate transform matrix
//   b4: CBFog         — Fog parameters
//   b5: CBScreenSize  — Screen dimensions for XYZRHW conversion
///////////////////////////////////////////////////////////////////////////////

namespace D3D11Shaders
{

///////////////////////////////////////////////////////////////////////////////
// Common constant buffer declarations (shared across shaders)
///////////////////////////////////////////////////////////////////////////////
static const char* szCommonCB = R"(
	cbuffer cbPerFrame : register(b0)
	{
		float4x4 matWorld;
		float4x4 matView;
		float4x4 matProj;
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
		float4 lightDir;		// Directional light direction (world space)
		float4 lightDiffuse;	// Light diffuse color
		float4 lightAmbient;	// Ambient light color
		float4 matDiffuse;		// Material diffuse color
		float4 matAmbient;		// Material ambient color
		float4 matEmissive;		// Material emissive color
		int lightingEnable;		// D3DRS_LIGHTING
		int pad0, pad1, pad2;
	};

	cbuffer cbTexTransform : register(b3)
	{
		row_major float4x4 matTexTransform0;
		row_major float4x4 matTexTransform1;
	};

	cbuffer cbFog : register(b4)
	{
		float4 fogColor;
		float fogStart;
		float fogEnd;
		int fogEnable;
		int fogPad;
	};

	cbuffer cbScreenSize : register(b5)
	{
		float screenWidth;
		float screenHeight;
		float screenPad0;
		float screenPad1;
	};
)";

///////////////////////////////////////////////////////////////////////////////
// Common pixel shader functions
///////////////////////////////////////////////////////////////////////////////
static const char* szCommonPS = R"(
	Texture2D txDiffuse0 : register(t0);
	Texture2D txDiffuse1 : register(t1);
	SamplerState sampler0 : register(s0);
	SamplerState sampler1 : register(s1);

	// Apply D3D9-style texture stage color operation
	float4 ApplyColorOp(int op, float4 arg1, float4 arg2)
	{
		if (op == 0) return arg1 * arg2;			// MODULATE
		if (op == 1) return arg1;					// SELECTARG1 (texture)
		if (op == 2) return arg1 + arg2;			// ADD
		if (op == 3) return arg2;					// SELECTARG2 (diffuse/current)
		if (op == 4) return arg1 * arg2 * 2.0f;	// MODULATE2X
		if (op == 5) return arg1 * arg2 * 4.0f;	// MODULATE4X
		if (op == 6)								// BLENDDIFFUSEALPHA
		{
			return lerp(arg2, arg1, arg2.a);
		}
		return arg1 * arg2;
	}

	// Alpha test — returns true if pixel should be discarded
	bool AlphaTestDiscard(float alpha)
	{
		if (alphaTestEnable)
		{
			float ref = (float)alphaRef / 255.0f;
			if (alpha < ref)
				return true;
		}
		return false;
	}

	// Fog — linear fog based on view-space depth
	float4 ApplyFog(float4 color, float viewDepth)
	{
		if (fogEnable && fogEnd > fogStart)
		{
			float fogFactor = saturate((fogEnd - viewDepth) / (fogEnd - fogStart));
			color.rgb = lerp(fogColor.rgb, color.rgb, fogFactor);
		}
		return color;
	}
)";

} // namespace D3D11Shaders