#pragma once
#ifndef __D3D9_COMPAT_H__
#define __D3D9_COMPAT_H__

//////////////////////////////////////////////////////////////////////////////
// D3D9Compat.h — D3D9 type/enum definitions for D3D9->D3D11 migration
//
// Provides all D3D9 enum values, struct layouts, and interface stubs needed
// by the codebase WITHOUT requiring d3d9.h or d3dx9.h. Values match the
// original D3D9 SDK exactly. This is a transitional header — each symbol
// defined here should eventually be replaced with native D3D11 equivalents.
//////////////////////////////////////////////////////////////////////////////

// Block legacy SDK headers from being included. We provide all needed D3D9
// types/enums here. Without these guards, the SDK copies get pulled in (via
// extern/include or ddraw.h -> d3d.h -> d3dtypes.h) and cause redefinitions.
#ifndef _D3D9_H_
#define _D3D9_H_
#endif
#ifndef _d3d9TYPES_H_
#define _d3d9TYPES_H_
#endif
// Block D3D7 headers (d3d.h, d3dtypes.h, d3dcaps.h) pulled in by ddraw.h.
// Our D3D9 enums are a superset of D3D7 with different naming (D3DRS_* vs
// D3DRENDERSTATE_*), so the two cannot coexist.
#ifndef _D3DTYPES_H_
#define _D3DTYPES_H_
#endif
#ifndef _D3D_H_
#define _D3D_H_
#endif
#ifndef _D3DCAPS_H_
#define _D3DCAPS_H_
#endif

#include <windows.h>
#include <d3d11.h>

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
	((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#endif

//////////////////////////////////////////////////////////////////////////////
// D3DCOLOR — ARGB packed 32-bit color
//////////////////////////////////////////////////////////////////////////////
typedef DWORD D3DCOLOR;

inline D3DCOLOR D3DCOLOR_ARGB(BYTE a, BYTE r, BYTE g, BYTE b)
{
	return ((D3DCOLOR)((((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)));
}

inline D3DCOLOR D3DCOLOR_XRGB(BYTE r, BYTE g, BYTE b)
{
	return D3DCOLOR_ARGB(0xff, r, g, b);
}

//////////////////////////////////////////////////////////////////////////////
// D3DCOLORVALUE — float4 color used in D3DMATERIAL9 / D3DLIGHT9
//////////////////////////////////////////////////////////////////////////////
#ifndef D3DCOLORVALUE_DEFINED
typedef struct _D3DCOLORVALUE {
	float r, g, b, a;
} D3DCOLORVALUE;
#define D3DCOLORVALUE_DEFINED
#endif

//////////////////////////////////////////////////////////////////////////////
// D3DVECTOR — base 3-component vector
//////////////////////////////////////////////////////////////////////////////
#ifndef D3DVECTOR_DEFINED
typedef struct _D3DVECTOR {
	float x, y, z;
} D3DVECTOR;
#define D3DVECTOR_DEFINED
#endif

//////////////////////////////////////////////////////////////////////////////
// D3DMATERIAL9
//////////////////////////////////////////////////////////////////////////////
typedef struct _D3DMATERIAL9 {
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Ambient;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Emissive;
	float         Power;
} D3DMATERIAL9;

//////////////////////////////////////////////////////////////////////////////
// D3DLIGHTTYPE
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DLIGHTTYPE {
	D3DLIGHT_POINT       = 1,
	D3DLIGHT_SPOT        = 2,
	D3DLIGHT_DIRECTIONAL = 3,
} D3DLIGHTTYPE;

//////////////////////////////////////////////////////////////////////////////
// D3DLIGHT9
//////////////////////////////////////////////////////////////////////////////
typedef struct _D3DLIGHT9 {
	D3DLIGHTTYPE  Type;
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Ambient;
	D3DVECTOR     Position;
	D3DVECTOR     Direction;
	float         Range;
	float         Falloff;
	float         Attenuation0;
	float         Attenuation1;
	float         Attenuation2;
	float         Theta;
	float         Phi;
} D3DLIGHT9;

//////////////////////////////////////////////////////////////////////////////
// D3DVIEWPORT9
//////////////////////////////////////////////////////////////////////////////
#ifndef _D3DVIEWPORT9_DEFINED
#define _D3DVIEWPORT9_DEFINED
typedef struct _D3DVIEWPORT9 {
	DWORD X, Y;
	DWORD Width, Height;
	float MinZ, MaxZ;
} D3DVIEWPORT9;
#endif

//////////////////////////////////////////////////////////////////////////////
// D3DFORMAT
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DFORMAT {
	D3DFMT_UNKNOWN      = 0,
	D3DFMT_R8G8B8       = 20,
	D3DFMT_A8R8G8B8     = 21,
	D3DFMT_X8R8G8B8     = 22,
	D3DFMT_R5G6B5       = 23,
	D3DFMT_X1R5G5B5     = 24,
	D3DFMT_A1R5G5B5     = 25,
	D3DFMT_A4R4G4B4     = 26,
	D3DFMT_A8           = 28,
	D3DFMT_A8R3G3B2     = 29,
	D3DFMT_X4R4G4B4     = 30,
	D3DFMT_A2B10G10R10  = 31,
	D3DFMT_A8B8G8R8     = 32,
	D3DFMT_G16R16       = 34,
	D3DFMT_A2R10G10B10  = 35,
	D3DFMT_A16B16G16R16 = 36,
	D3DFMT_A8P8         = 40,
	D3DFMT_P8           = 41,
	D3DFMT_L8           = 50,
	D3DFMT_A8L8         = 51,
	D3DFMT_A4L4         = 52,
	D3DFMT_V8U8         = 60,
	D3DFMT_L6V5U5       = 61,
	D3DFMT_X8L8V8U8     = 62,
	D3DFMT_Q8W8V8U8     = 63,
	D3DFMT_V16U16       = 64,
	D3DFMT_A2W10V10U10  = 67,
	D3DFMT_D16_LOCKABLE = 70,
	D3DFMT_D32          = 71,
	D3DFMT_D15S1        = 73,
	D3DFMT_D24S8        = 75,
	D3DFMT_D24X8        = 77,
	D3DFMT_D24X4S4      = 79,
	D3DFMT_D16          = 80,
	D3DFMT_L16          = 81,
	D3DFMT_D32F_LOCKABLE = 82,
	D3DFMT_D24FS8       = 83,
	D3DFMT_INDEX16      = 101,
	D3DFMT_INDEX32      = 102,
	D3DFMT_Q16W16V16U16 = 110,
	D3DFMT_R16F         = 111,
	D3DFMT_G16R16F      = 112,
	D3DFMT_A16B16G16R16F = 113,
	D3DFMT_R32F         = 114,
	D3DFMT_G32R32F      = 115,
	D3DFMT_A32B32G32R32F = 116,
	D3DFMT_CxV8U8       = 117,

	D3DFMT_DXT1 = MAKEFOURCC('D','X','T','1'),
	D3DFMT_DXT2 = MAKEFOURCC('D','X','T','2'),
	D3DFMT_DXT3 = MAKEFOURCC('D','X','T','3'),
	D3DFMT_DXT4 = MAKEFOURCC('D','X','T','4'),
	D3DFMT_DXT5 = MAKEFOURCC('D','X','T','5'),

	D3DFMT_FORCE_DWORD = 0x7fffffff,
} D3DFORMAT;

//////////////////////////////////////////////////////////////////////////////
// D3DRENDERSTATETYPE
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DRENDERSTATETYPE {
	D3DRS_ZENABLE                   = 7,
	D3DRS_FILLMODE                  = 8,
	D3DRS_SHADEMODE                 = 9,
	D3DRS_ZWRITEENABLE              = 14,
	D3DRS_ALPHATESTENABLE           = 15,
	D3DRS_LASTPIXEL                 = 16,
	D3DRS_SRCBLEND                  = 19,
	D3DRS_DESTBLEND                 = 20,
	D3DRS_CULLMODE                  = 22,
	D3DRS_ZFUNC                     = 23,
	D3DRS_ALPHAREF                  = 24,
	D3DRS_ALPHAFUNC                 = 25,
	D3DRS_DITHERENABLE              = 26,
	D3DRS_ALPHABLENDENABLE          = 27,
	D3DRS_FOGENABLE                 = 28,
	D3DRS_SPECULARENABLE            = 29,
	D3DRS_FOGCOLOR                  = 34,
	D3DRS_FOGTABLEMODE              = 35,
	D3DRS_FOGSTART                  = 36,
	D3DRS_FOGEND                    = 37,
	D3DRS_FOGDENSITY                = 38,
	D3DRS_RANGEFOGENABLE            = 48,
	D3DRS_STENCILENABLE             = 52,
	D3DRS_STENCILFAIL               = 53,
	D3DRS_STENCILZFAIL              = 54,
	D3DRS_STENCILPASS               = 55,
	D3DRS_STENCILFUNC               = 56,
	D3DRS_STENCILREF                = 57,
	D3DRS_STENCILMASK               = 58,
	D3DRS_STENCILWRITEMASK          = 59,
	D3DRS_TEXTUREFACTOR             = 60,
	D3DRS_WRAP0                     = 128,
	D3DRS_WRAP1                     = 129,
	D3DRS_WRAP2                     = 130,
	D3DRS_WRAP3                     = 131,
	D3DRS_WRAP4                     = 132,
	D3DRS_WRAP5                     = 133,
	D3DRS_WRAP6                     = 134,
	D3DRS_WRAP7                     = 135,
	D3DRS_CLIPPING                  = 136,
	D3DRS_LIGHTING                  = 137,
	D3DRS_AMBIENT                   = 139,
	D3DRS_FOGVERTEXMODE             = 140,
	D3DRS_COLORVERTEX               = 141,
	D3DRS_LOCALVIEWER               = 142,
	D3DRS_NORMALIZENORMALS          = 143,
	D3DRS_DIFFUSEMATERIALSOURCE     = 145,
	D3DRS_SPECULARMATERIALSOURCE    = 146,
	D3DRS_AMBIENTMATERIALSOURCE     = 147,
	D3DRS_EMISSIVEMATERIALSOURCE    = 148,
	D3DRS_VERTEXBLEND               = 151,
	D3DRS_CLIPPLANEENABLE           = 152,
	D3DRS_POINTSIZE                 = 154,
	D3DRS_POINTSIZE_MIN             = 155,
	D3DRS_POINTSPRITEENABLE         = 156,
	D3DRS_POINTSCALEENABLE          = 157,
	D3DRS_POINTSCALE_A              = 158,
	D3DRS_POINTSCALE_B              = 159,
	D3DRS_POINTSCALE_C              = 160,
	D3DRS_MULTISAMPLEANTIALIAS      = 161,
	D3DRS_MULTISAMPLEMASK           = 162,
	D3DRS_PATCHEDGESTYLE            = 163,
	D3DRS_DEBUGMONITORTOKEN         = 165,
	D3DRS_POINTSIZE_MAX             = 166,
	D3DRS_INDEXEDVERTEXBLENDENABLE  = 167,
	D3DRS_COLORWRITEENABLE          = 168,
	D3DRS_TWEENFACTOR               = 170,
	D3DRS_BLENDOP                   = 171,
	D3DRS_POSITIONDEGREE            = 172,
	D3DRS_NORMALDEGREE              = 173,
	D3DRS_SCISSORTESTENABLE         = 174,
	D3DRS_SLOPESCALEDEPTHBIAS       = 175,
	D3DRS_ANTIALIASEDLINEENABLE     = 176,
	D3DRS_MINTESSELLATIONLEVEL      = 178,
	D3DRS_MAXTESSELLATIONLEVEL      = 179,
	D3DRS_ADAPTIVETESS_X            = 180,
	D3DRS_ADAPTIVETESS_Y            = 181,
	D3DRS_ADAPTIVETESS_Z            = 182,
	D3DRS_ADAPTIVETESS_W            = 183,
	D3DRS_ENABLEADAPTIVETESSELLATION = 184,
	D3DRS_TWOSIDEDSTENCILMODE       = 185,
	D3DRS_CCW_STENCILFAIL           = 186,
	D3DRS_CCW_STENCILZFAIL          = 187,
	D3DRS_CCW_STENCILPASS           = 188,
	D3DRS_CCW_STENCILFUNC           = 189,
	D3DRS_COLORWRITEENABLE1         = 190,
	D3DRS_COLORWRITEENABLE2         = 191,
	D3DRS_COLORWRITEENABLE3         = 192,
	D3DRS_BLENDFACTOR               = 193,
	D3DRS_SRGBWRITEENABLE           = 194,
	D3DRS_DEPTHBIAS                 = 195,
	D3DRS_WRAP8                     = 198,
	D3DRS_WRAP9                     = 199,
	D3DRS_WRAP10                    = 200,
	D3DRS_WRAP11                    = 201,
	D3DRS_WRAP12                    = 202,
	D3DRS_WRAP13                    = 203,
	D3DRS_WRAP14                    = 204,
	D3DRS_WRAP15                    = 205,
	D3DRS_SEPARATEALPHABLENDENABLE  = 206,
	D3DRS_SRCBLENDALPHA             = 207,
	D3DRS_DESTBLENDALPHA            = 208,
	D3DRS_BLENDOPALPHA              = 209,

	D3DRS_FORCE_DWORD = 0x7fffffff,
} D3DRENDERSTATETYPE;

//////////////////////////////////////////////////////////////////////////////
// D3DFILLMODE
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DFILLMODE {
	D3DFILL_POINT       = 1,
	D3DFILL_WIREFRAME   = 2,
	D3DFILL_SOLID       = 3,
	D3DFILL_FORCE_DWORD = 0x7fffffff,
} D3DFILLMODE;

//////////////////////////////////////////////////////////////////////////////
// D3DSHADEMODE
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DSHADEMODE {
	D3DSHADE_FLAT        = 1,
	D3DSHADE_GOURAUD     = 2,
	D3DSHADE_PHONG       = 3,
	D3DSHADE_FORCE_DWORD = 0x7fffffff,
} D3DSHADEMODE;

//////////////////////////////////////////////////////////////////////////////
// D3DBLEND
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DBLEND {
	D3DBLEND_ZERO            = 1,
	D3DBLEND_ONE             = 2,
	D3DBLEND_SRCCOLOR        = 3,
	D3DBLEND_INVSRCCOLOR     = 4,
	D3DBLEND_SRCALPHA        = 5,
	D3DBLEND_INVSRCALPHA     = 6,
	D3DBLEND_DESTALPHA       = 7,
	D3DBLEND_INVDESTALPHA    = 8,
	D3DBLEND_DESTCOLOR       = 9,
	D3DBLEND_INVDESTCOLOR    = 10,
	D3DBLEND_SRCALPHASAT     = 11,
	D3DBLEND_BOTHSRCALPHA    = 12,
	D3DBLEND_BOTHINVSRCALPHA = 13,
	D3DBLEND_BLENDFACTOR     = 14,
	D3DBLEND_INVBLENDFACTOR  = 15,
	D3DBLEND_FORCE_DWORD     = 0x7fffffff,
} D3DBLEND;

//////////////////////////////////////////////////////////////////////////////
// D3DBLENDOP
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DBLENDOP {
	D3DBLENDOP_ADD         = 1,
	D3DBLENDOP_SUBTRACT    = 2,
	D3DBLENDOP_REVSUBTRACT = 3,
	D3DBLENDOP_MIN         = 4,
	D3DBLENDOP_MAX         = 5,
	D3DBLENDOP_FORCE_DWORD = 0x7fffffff,
} D3DBLENDOP;

//////////////////////////////////////////////////////////////////////////////
// D3DCMPFUNC
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DCMPFUNC {
	D3DCMP_NEVER        = 1,
	D3DCMP_LESS         = 2,
	D3DCMP_EQUAL        = 3,
	D3DCMP_LESSEQUAL    = 4,
	D3DCMP_GREATER      = 5,
	D3DCMP_NOTEQUAL     = 6,
	D3DCMP_GREATEREQUAL = 7,
	D3DCMP_ALWAYS       = 8,
	D3DCMP_FORCE_DWORD  = 0x7fffffff,
} D3DCMPFUNC;

//////////////////////////////////////////////////////////////////////////////
// D3DCULL
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DCULL {
	D3DCULL_NONE        = 1,
	D3DCULL_CW          = 2,
	D3DCULL_CCW         = 3,
	D3DCULL_FORCE_DWORD = 0x7fffffff,
} D3DCULL;

//////////////////////////////////////////////////////////////////////////////
// D3DFOGMODE
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DFOGMODE {
	D3DFOG_NONE        = 0,
	D3DFOG_EXP         = 1,
	D3DFOG_EXP2        = 2,
	D3DFOG_LINEAR      = 3,
	D3DFOG_FORCE_DWORD = 0x7fffffff,
} D3DFOGMODE;

//////////////////////////////////////////////////////////////////////////////
// D3DTEXTURESTAGESTATETYPE
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DTEXTURESTAGESTATETYPE {
	D3DTSS_COLOROP               = 1,
	D3DTSS_COLORARG1             = 2,
	D3DTSS_COLORARG2             = 3,
	D3DTSS_ALPHAOP               = 4,
	D3DTSS_ALPHAARG1             = 5,
	D3DTSS_ALPHAARG2             = 6,
	D3DTSS_BUMPENVMAT00          = 7,
	D3DTSS_BUMPENVMAT01          = 8,
	D3DTSS_BUMPENVMAT10          = 9,
	D3DTSS_BUMPENVMAT11          = 10,
	D3DTSS_TEXCOORDINDEX         = 11,
	D3DTSS_BUMPENVLSCALE         = 22,
	D3DTSS_BUMPENVLOFFSET        = 23,
	D3DTSS_TEXTURETRANSFORMFLAGS = 24,
	D3DTSS_COLORARG0             = 26,
	D3DTSS_ALPHAARG0             = 27,
	D3DTSS_RESULTARG             = 28,
	D3DTSS_CONSTANT              = 32,
	D3DTSS_FORCE_DWORD           = 0x7fffffff,
} D3DTEXTURESTAGESTATETYPE;

//////////////////////////////////////////////////////////////////////////////
// D3DTSS_TCI_* — texture coordinate index flags
//////////////////////////////////////////////////////////////////////////////
#define D3DTSS_TCI_PASSTHRU                       0x00000000
#define D3DTSS_TCI_CAMERASPACENORMAL              0x00010000
#define D3DTSS_TCI_CAMERASPACEPOSITION            0x00020000
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR    0x00030000
#define D3DTSS_TCI_SPHEREMAP                      0x00040000

//////////////////////////////////////////////////////////////////////////////
// D3DTEXTURETRANSFORMFLAGS
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DTEXTURETRANSFORMFLAGS {
	D3DTTFF_DISABLE     = 0,
	D3DTTFF_COUNT1      = 1,
	D3DTTFF_COUNT2      = 2,
	D3DTTFF_COUNT3      = 3,
	D3DTTFF_COUNT4      = 4,
	D3DTTFF_PROJECTED   = 256,
	D3DTTFF_FORCE_DWORD = 0x7fffffff,
} D3DTEXTURETRANSFORMFLAGS;

//////////////////////////////////////////////////////////////////////////////
// D3DTEXTUREOP
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DTEXTUREOP {
	D3DTOP_DISABLE                   = 1,
	D3DTOP_SELECTARG1                = 2,
	D3DTOP_SELECTARG2                = 3,
	D3DTOP_MODULATE                  = 4,
	D3DTOP_MODULATE2X                = 5,
	D3DTOP_MODULATE4X                = 6,
	D3DTOP_ADD                       = 7,
	D3DTOP_ADDSIGNED                 = 8,
	D3DTOP_ADDSIGNED2X               = 9,
	D3DTOP_SUBTRACT                  = 10,
	D3DTOP_ADDSMOOTH                 = 11,
	D3DTOP_BLENDDIFFUSEALPHA         = 12,
	D3DTOP_BLENDTEXTUREALPHA         = 13,
	D3DTOP_BLENDFACTORALPHA          = 14,
	D3DTOP_BLENDTEXTUREALPHAPM       = 15,
	D3DTOP_BLENDCURRENTALPHA         = 16,
	D3DTOP_PREMODULATE               = 17,
	D3DTOP_MODULATEALPHA_ADDCOLOR    = 18,
	D3DTOP_MODULATECOLOR_ADDALPHA    = 19,
	D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,
	D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,
	D3DTOP_BUMPENVMAP                = 22,
	D3DTOP_BUMPENVMAPLUMINANCE       = 23,
	D3DTOP_DOTPRODUCT3               = 24,
	D3DTOP_MULTIPLYADD               = 25,
	D3DTOP_LERP                      = 26,
	D3DTOP_FORCE_DWORD               = 0x7fffffff,
} D3DTEXTUREOP;

//////////////////////////////////////////////////////////////////////////////
// D3DTA_* — texture argument flags
//////////////////////////////////////////////////////////////////////////////
#define D3DTA_SELECTMASK     0x0000000f
#define D3DTA_DIFFUSE        0x00000000
#define D3DTA_CURRENT        0x00000001
#define D3DTA_TEXTURE        0x00000002
#define D3DTA_TFACTOR        0x00000003
#define D3DTA_SPECULAR       0x00000004
#define D3DTA_TEMP           0x00000005
#define D3DTA_CONSTANT       0x00000006
#define D3DTA_COMPLEMENT     0x00000010
#define D3DTA_ALPHAREPLICATE 0x00000020

////////////////////////////////////////////////////////////////////////////////
//// D3DSAMPLERSTATETYPE
////////////////////////////////////////////////////////////////////////////////
//typedef enum _D3DSAMPLERSTATETYPE {
//	//D3DSAMP_ADDRESSU      = 1,
//	//D3DSAMP_ADDRESSV      = 2,
//	//D3DSAMP_ADDRESSW      = 3,
//	D3DSAMP_BORDERCOLOR   = 4,
//	//D3DSAMP_MAGFILTER     = 5,
//	//D3DSAMP_MINFILTER     = 6,
//	//D3DSAMP_MIPFILTER     = 7,
//	D3DSAMP_MIPMAPLODBIAS = 8,
//	D3DSAMP_MAXMIPLEVEL   = 9,
//	D3DSAMP_MAXANISOTROPY = 10,
//	D3DSAMP_SRGBTEXTURE   = 11,
//	D3DSAMP_ELEMENTINDEX  = 12,
//	D3DSAMP_DMAPOFFSET    = 13,
//	D3DSAMP_FORCE_DWORD   = 0x7fffffff,
//} D3DSAMPLERSTATETYPE;

//////////////////////////////////////////////////////////////////////////////
// D3DTEXTUREFILTERTYPE
//////////////////////////////////////////////////////////////////////////////
//typedef enum _D3DTEXTUREFILTERTYPE {
//	TF11_NONE            = 0,
//	TF11_POINT           = 1,
//	TF11_LINEAR          = 2,
//	TF11_ANISOTROPIC     = 3,
//	D3DTEXF_PYRAMIDALQUAD   = 6,
//	D3DTEXF_GAUSSIANQUAD    = 7,
//	D3DTEXF_CONVOLUTIONMONO = 8,
//	D3DTEXF_FORCE_DWORD     = 0x7fffffff,
//} D3DTEXTUREFILTERTYPE;

//////////////////////////////////////////////////////////////////////////////
// D3DTEXTUREADDRESS
//////////////////////////////////////////////////////////////////////////////
//typedef enum _D3DTEXTUREADDRESS {
//	D3D11_TEXTURE_ADDRESS_WRAP        = 1,
//	D3DTADDRESS_MIRROR      = 2,
//	D3D11_TEXTURE_ADDRESS_CLAMP       = 3,
//	D3D11_TEXTURE_ADDRESS_BORDER      = 4,
//	D3DTADDRESS_MIRRORONCE  = 5,
//	D3DTADDRESS_FORCE_DWORD = 0x7fffffff,
//} D3DTEXTUREADDRESS;

//////////////////////////////////////////////////////////////////////////////
// D3DPRIMITIVETYPE
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DPRIMITIVETYPE {
	D3DPT_POINTLIST     = 1,
	D3DPT_LINELIST      = 2,
	D3DPT_LINESTRIP     = 3,
	D3DPT_TRIANGLELIST  = 4,
	D3DPT_TRIANGLESTRIP = 5,
	D3DPT_TRIANGLEFAN   = 6,
	D3DPT_FORCE_DWORD   = 0x7fffffff,
} D3DPRIMITIVETYPE;

//////////////////////////////////////////////////////////////////////////////
// D3DVERTEXBLENDFLAGS
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DVERTEXBLENDFLAGS {
	D3DVBF_DISABLE  = 0,
	D3DVBF_1WEIGHTS = 1,
	D3DVBF_2WEIGHTS = 2,
	D3DVBF_3WEIGHTS = 3,
	D3DVBF_TWEENING = 255,
	D3DVBF_0WEIGHTS = 256,
} D3DVERTEXBLENDFLAGS;

//////////////////////////////////////////////////////////////////////////////
// D3DCOLORWRITEENABLE flags
//////////////////////////////////////////////////////////////////////////////
#define D3DCOLORWRITEENABLE_RED   (1L << 0)
#define D3DCOLORWRITEENABLE_GREEN (1L << 1)
#define D3DCOLORWRITEENABLE_BLUE  (1L << 2)
#define D3DCOLORWRITEENABLE_ALPHA (1L << 3)

//////////////////////////////////////////////////////////////////////////////
// D3DSWAPEFFECT
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DSWAPEFFECT {
	D3DSWAPEFFECT_DISCARD     = 1,
	D3DSWAPEFFECT_FLIP        = 2,
	D3DSWAPEFFECT_COPY        = 3,
	D3DSWAPEFFECT_OVERLAY     = 4,
	D3DSWAPEFFECT_FLIPEX      = 5,
	D3DSWAPEFFECT_FORCE_DWORD = 0x7fffffff,
} D3DSWAPEFFECT;

// D3DMULTISAMPLE_TYPE
typedef enum _D3DMULTISAMPLE_TYPE {
	D3DMULTISAMPLE_NONE        = 0,
	D3DMULTISAMPLE_NONMASKABLE = 1,
	D3DMULTISAMPLE_2_SAMPLES   = 2,
	D3DMULTISAMPLE_3_SAMPLES   = 3,
	D3DMULTISAMPLE_4_SAMPLES   = 4,
	D3DMULTISAMPLE_5_SAMPLES   = 5,
	D3DMULTISAMPLE_6_SAMPLES   = 6,
	D3DMULTISAMPLE_7_SAMPLES   = 7,
	D3DMULTISAMPLE_8_SAMPLES   = 8,
	D3DMULTISAMPLE_9_SAMPLES   = 9,
	D3DMULTISAMPLE_10_SAMPLES  = 10,
	D3DMULTISAMPLE_11_SAMPLES  = 11,
	D3DMULTISAMPLE_12_SAMPLES  = 12,
	D3DMULTISAMPLE_13_SAMPLES  = 13,
	D3DMULTISAMPLE_14_SAMPLES  = 14,
	D3DMULTISAMPLE_15_SAMPLES  = 15,
	D3DMULTISAMPLE_16_SAMPLES  = 16,
	D3DMULTISAMPLE_FORCE_DWORD = 0x7fffffff,
} D3DMULTISAMPLE_TYPE;

// D3DPRESENT_PARAMETERS
typedef struct _D3DPRESENT_PARAMETERS_ {
	UINT                BackBufferWidth;
	UINT                BackBufferHeight;
	D3DFORMAT           BackBufferFormat;
	UINT                BackBufferCount;
	D3DMULTISAMPLE_TYPE MultiSampleType;
	DWORD               MultiSampleQuality;
	D3DSWAPEFFECT       SwapEffect;
	HWND                hDeviceWindow;
	BOOL                Windowed;
	BOOL                EnableAutoDepthStencil;
	D3DFORMAT           AutoDepthStencilFormat;
	DWORD               Flags;
	UINT                FullScreen_RefreshRateInHz;
	UINT                PresentationInterval;
} D3DPRESENT_PARAMETERS;

// D3DPRESENT_INTERVAL / D3DPRESENT_RATE constants
#define D3DPRESENT_INTERVAL_DEFAULT   0x00000000
#define D3DPRESENT_INTERVAL_ONE       0x00000001
#define D3DPRESENT_INTERVAL_TWO       0x00000002
#define D3DPRESENT_INTERVAL_THREE     0x00000004
#define D3DPRESENT_INTERVAL_FOUR      0x00000008
#define D3DPRESENT_INTERVAL_IMMEDIATE 0x80000000
#define D3DPRESENT_RATE_DEFAULT       0x00000000

// D3DDISPLAYMODE
typedef struct _D3DDISPLAYMODE {
	UINT      Width;
	UINT      Height;
	UINT      RefreshRate;
	D3DFORMAT Format;
} D3DDISPLAYMODE;

// D3DSCANLINEORDERING
typedef enum _D3DSCANLINEORDERING {
	D3DSCANLINEORDERING_UNKNOWN     = 0,
	D3DSCANLINEORDERING_PROGRESSIVE = 1,
	D3DSCANLINEORDERING_INTERLACED  = 2,
} D3DSCANLINEORDERING;

// D3DDISPLAYMODEEX
typedef struct _D3DDISPLAYMODEEX {
	UINT                 Size;
	UINT                 Width;
	UINT                 Height;
	UINT                 RefreshRate;
	D3DFORMAT            Format;
	D3DSCANLINEORDERING  ScanLineOrdering;
} D3DDISPLAYMODEEX;

// D3DCAPS9 (minimal — only fields actually accessed by the codebase)
typedef struct _D3DCAPS9 {
	DWORD DevCaps;
	DWORD TextureCaps;
	DWORD TextureFilterCaps;
	DWORD TextureAddressCaps;
	DWORD MaxTextureWidth;
	DWORD MaxTextureHeight;
	DWORD VertexProcessingCaps;
	DWORD MaxActiveLights;
	DWORD MaxVertexBlendMatrices;
	DWORD MaxVertexBlendMatrixIndex;
	DWORD VertexShaderVersion;
	DWORD PixelShaderVersion;
	DWORD MaxPrimitiveCount;
	DWORD MaxVertexIndex;
	DWORD MaxStreams;
	DWORD MaxStreamStride;
	DWORD MaxTextureBlendStages;
	DWORD MaxSimultaneousTextures;
	DWORD _padding[64];
} D3DCAPS9;

// D3DCAPS9 flag constants used in the codebase
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT 0x00010000
#define D3DDEVCAPS_PUREDEVICE          0x00000100
#define D3DVTXPCAPS_DIRECTIONALLIGHTS  0x00000008
#define D3DVTXPCAPS_POSITIONALLIGHTS   0x00000010
#define D3DVTXPCAPS_TEXGEN             0x00010000
#define D3DPTADDRESSCAPS_BORDER        0x00000008

// D3DCREATE_* flags
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x00000020
#define D3DCREATE_HARDWARE_VERTEXPROCESSING 0x00000040
#define D3DCREATE_MIXED_VERTEXPROCESSING    0x00000080
#define D3DCREATE_PUREDEVICE               0x00000010

// D3DADAPTER_IDENTIFIER9
typedef struct _D3DADAPTER_IDENTIFIER9 {
	char  Driver[512];
	char  Description[512];
	char  DeviceName[32];
	LARGE_INTEGER DriverVersion;
	DWORD VendorId;
	DWORD DeviceId;
	DWORD SubSysId;
	DWORD Revision;
	GUID  DeviceIdentifier;
	DWORD WHQLLevel;
} D3DADAPTER_IDENTIFIER9;

// D3DADAPTER_DEFAULT / D3DDEVTYPE
#define D3DADAPTER_DEFAULT 0

typedef enum _D3DDEVTYPE {
	D3DDEVTYPE_HAL       = 1,
	D3DDEVTYPE_REF       = 2,
	D3DDEVTYPE_SW        = 3,
	D3DDEVTYPE_NULLREF   = 4,
	D3DDEVTYPE_FORCE_DWORD = 0x7fffffff,
} D3DDEVTYPE;

// D3DRESOURCETYPE
typedef enum _D3DRESOURCETYPE {
	D3DRTYPE_SURFACE       = 1,
	D3DRTYPE_VOLUME        = 2,
	D3DRTYPE_TEXTURE       = 3,
	D3DRTYPE_VOLUMETEXTURE = 4,
	D3DRTYPE_CUBETEXTURE   = 5,
	D3DRTYPE_VERTEXBUFFER  = 6,
	D3DRTYPE_INDEXBUFFER   = 7,
	D3DRTYPE_FORCE_DWORD   = 0x7fffffff,
} D3DRESOURCETYPE;

//////////////////////////////////////////////////////////////////////////////
// D3DPOOL
//////////////////////////////////////////////////////////////////////////////
typedef enum _D3DPOOL {
	D3DPOOL_DEFAULT   = 0,
	D3DPOOL_MANAGED   = 1,
	D3DPOOL_SYSTEMMEM = 2,
	D3DPOOL_SCRATCH   = 3,
	D3DPOOL_FORCE_DWORD = 0x7fffffff,
} D3DPOOL;

//////////////////////////////////////////////////////////////////////////////
// D3DUSAGE flags
//////////////////////////////////////////////////////////////////////////////
#define D3DUSAGE_WRITEONLY 0x00000008
#define D3DUSAGE_DYNAMIC   0x00000200

// D3DLOCK flags
#define D3DLOCK_READONLY  0x00000010
#define D3DLOCK_DISCARD   0x00002000
#define D3DLOCK_NOOVERWRITE 0x00001000
#define D3DLOCK_NOSYSLOCK 0x00000800

//////////////////////////////////////////////////////////////////////////////
// D3DX_FILTER flags (from d3dx9tex.h)
//////////////////////////////////////////////////////////////////////////////
#define D3DX_FILTER_LINEAR (3 << 0)

// D3DCLEAR flags
#define D3DCLEAR_TARGET  0x00000001
#define D3DCLEAR_ZBUFFER 0x00000002
#define D3DCLEAR_STENCIL 0x00000004

// D3D_OK and error codes
#define D3D_OK                  S_OK
#define D3D_SDK_VERSION         32
#define D3DERR_INVALIDCALL      ((HRESULT)0x8876086C)
#define D3DERR_NOTAVAILABLE     ((HRESULT)0x8876086E)
#define D3DERR_OUTOFVIDEOMEMORY ((HRESULT)0x8876017C)
#define D3DERR_DEVICELOST       ((HRESULT)0x88760868)
#define D3DERR_DEVICENOTRESET   ((HRESULT)0x88760869)

// FVF flags
#define D3DFVF_RESERVED0       0x001
#define D3DFVF_XYZ             0x002
#define D3DFVF_XYZRHW          0x004
#define D3DFVF_XYZB1           0x006
#define D3DFVF_XYZB2           0x008
#define D3DFVF_XYZB3           0x00a
#define D3DFVF_XYZB4           0x00c
#define D3DFVF_XYZB5           0x00e
#define D3DFVF_XYZW            0x4002
#define D3DFVF_NORMAL          0x010
#define D3DFVF_PSIZE           0x020
#define D3DFVF_DIFFUSE         0x040
#define D3DFVF_SPECULAR        0x080
#define D3DFVF_TEXCOUNT_MASK   0xf00
#define D3DFVF_TEXCOUNT_SHIFT  8
#define D3DFVF_TEX0            0x000
#define D3DFVF_TEX1            0x100
#define D3DFVF_TEX2            0x200
#define D3DFVF_TEX3            0x300
#define D3DFVF_TEX4            0x400
#define D3DFVF_TEX5            0x500
#define D3DFVF_TEX6            0x600
#define D3DFVF_TEX7            0x700
#define D3DFVF_TEX8            0x800
#define D3DFVF_LASTBETA_UBYTE4 0x1000
#define D3DFVF_LASTBETA_D3DCOLOR 0x8000

// D3DFVF_TEXCOORDSIZE macros — encode float count per texture coord set
#define D3DFVF_TEXTUREFORMAT1 3
#define D3DFVF_TEXTUREFORMAT2 0
#define D3DFVF_TEXTUREFORMAT3 1
#define D3DFVF_TEXTUREFORMAT4 2
#define D3DFVF_TEXCOORDSIZE1(CoordIndex) (D3DFVF_TEXTUREFORMAT1 << (CoordIndex*2 + 16))
#define D3DFVF_TEXCOORDSIZE2(CoordIndex) (D3DFVF_TEXTUREFORMAT2 << (CoordIndex*2 + 16))
#define D3DFVF_TEXCOORDSIZE3(CoordIndex) (D3DFVF_TEXTUREFORMAT3 << (CoordIndex*2 + 16))
#define D3DFVF_TEXCOORDSIZE4(CoordIndex) (D3DFVF_TEXTUREFORMAT4 << (CoordIndex*2 + 16))

// D3DVERTEXELEMENT9 and vertex declaration types
#define MAX_FVF_DECL_SIZE 65

typedef enum _D3DDECLTYPE {
	D3DDECLTYPE_FLOAT1    = 0,
	D3DDECLTYPE_FLOAT2    = 1,
	D3DDECLTYPE_FLOAT3    = 2,
	D3DDECLTYPE_FLOAT4    = 3,
	D3DDECLTYPE_D3DCOLOR  = 4,
	D3DDECLTYPE_UBYTE4    = 5,
	D3DDECLTYPE_SHORT2    = 6,
	D3DDECLTYPE_SHORT4    = 7,
	D3DDECLTYPE_UBYTE4N   = 8,
	D3DDECLTYPE_SHORT2N   = 9,
	D3DDECLTYPE_SHORT4N   = 10,
	D3DDECLTYPE_USHORT2N  = 11,
	D3DDECLTYPE_USHORT4N  = 12,
	D3DDECLTYPE_UDEC3     = 13,
	D3DDECLTYPE_DEC3N     = 14,
	D3DDECLTYPE_FLOAT16_2 = 15,
	D3DDECLTYPE_FLOAT16_4 = 16,
	D3DDECLTYPE_UNUSED    = 17,
} D3DDECLTYPE;

typedef enum _D3DDECLMETHOD {
	D3DDECLMETHOD_DEFAULT          = 0,
	D3DDECLMETHOD_PARTIALU         = 1,
	D3DDECLMETHOD_PARTIALV         = 2,
	D3DDECLMETHOD_CROSSUV          = 3,
	D3DDECLMETHOD_UV               = 4,
	D3DDECLMETHOD_LOOKUP           = 5,
	D3DDECLMETHOD_LOOKUPPRESAMPLED = 6,
} D3DDECLMETHOD;

typedef enum _D3DDECLUSAGE {
	D3DDECLUSAGE_POSITION     = 0,
	D3DDECLUSAGE_BLENDWEIGHT  = 1,
	D3DDECLUSAGE_BLENDINDICES = 2,
	D3DDECLUSAGE_NORMAL       = 3,
	D3DDECLUSAGE_PSIZE        = 4,
	D3DDECLUSAGE_TEXCOORD     = 5,
	D3DDECLUSAGE_TANGENT      = 6,
	D3DDECLUSAGE_BINORMAL     = 7,
	D3DDECLUSAGE_TESSFACTOR   = 8,
	D3DDECLUSAGE_POSITIONT    = 9,
	D3DDECLUSAGE_COLOR        = 10,
	D3DDECLUSAGE_FOG          = 11,
	D3DDECLUSAGE_DEPTH        = 12,
	D3DDECLUSAGE_SAMPLE       = 13,
} D3DDECLUSAGE;

typedef struct _D3DVERTEXELEMENT9 {
	WORD Stream;
	WORD Offset;
	BYTE Type;
	BYTE Method;
	BYTE Usage;
	BYTE UsageIndex;
} D3DVERTEXELEMENT9;

#define D3DDECL_END() { 0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0 }

// Interface stubs — forward declarations and typedefs for D3D9 COM objects.
// Structs with stub methods are defined below for objects whose methods are
// still called in the codebase. Others are void pointers.
typedef void* LPDIRECT3DDEVICE9;
typedef void* LPDIRECT3DSURFACE9;

struct IDirect3DDevice9Ex;
typedef IDirect3DDevice9Ex* LPDIRECT3DDEVICE9EX;

struct IDirect3D9Ex;
typedef IDirect3D9Ex* LPDIRECT3D9EX;

// Vertex declaration stub — needs GetDeclaration method for StateManager
struct IDirect3DVertexDeclaration9
{
	D3DVERTEXELEMENT9 m_elements[MAX_FVF_DECL_SIZE];
	UINT m_numElements;

	HRESULT GetDeclaration(D3DVERTEXELEMENT9* pElement, UINT* pNumElements)
	{
		if (pNumElements) *pNumElements = m_numElements;
		if (pElement) memcpy(pElement, m_elements, sizeof(D3DVERTEXELEMENT9) * m_numElements);
		return S_OK;
	}

	ULONG Release() { return 0; }
};
typedef IDirect3DVertexDeclaration9* LPDIRECT3DVERTEXDECLARATION9;

// Shader stubs — minimal struct with Release() for cleanup code
struct IDirect3DVertexShader9 { ULONG Release() { return 0; } };
typedef IDirect3DVertexShader9* LPDIRECT3DVERTEXSHADER9;

struct IDirect3DPixelShader9 { ULONG Release() { return 0; } };
typedef IDirect3DPixelShader9* LPDIRECT3DPIXELSHADER9;

// IDirect3DDevice9Ex stub — only methods actually called in the codebase
struct IDirect3DDevice9Ex
{
	HRESULT TestCooperativeLevel() { return S_OK; }
	HRESULT Reset(D3DPRESENT_PARAMETERS*) { return S_OK; }
	HRESULT GetDeviceCaps(D3DCAPS9*) { return S_OK; }
	HRESULT GetViewport(D3DVIEWPORT9*) { return S_OK; }
	HRESULT Clear(DWORD, const void*, DWORD, D3DCOLOR, float, DWORD) { return S_OK; }
	HRESULT CreateVertexDeclaration(const D3DVERTEXELEMENT9*, LPDIRECT3DVERTEXDECLARATION9*) { return S_OK; }
	ULONG Release() { return 0; }
};

// IDirect3D9Ex stub — only methods actually called in the codebase
struct IDirect3D9Ex
{
	HRESULT GetAdapterIdentifier(UINT, DWORD, D3DADAPTER_IDENTIFIER9*) { return S_OK; }
	HRESULT GetAdapterDisplayMode(UINT, D3DDISPLAYMODE*) { return S_OK; }
	HRESULT GetDeviceCaps(UINT, D3DDEVTYPE, D3DCAPS9*) { return S_OK; }
	HRESULT CheckDeviceMultiSampleType(UINT, D3DDEVTYPE, D3DFORMAT, BOOL, D3DMULTISAMPLE_TYPE, DWORD*) { return S_OK; }
	HRESULT CheckDeviceFormat(UINT, D3DDEVTYPE, D3DFORMAT, DWORD, D3DRESOURCETYPE, D3DFORMAT) { return S_OK; }
	HRESULT CreateDeviceEx(UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*, IDirect3DDevice9Ex**) { return S_OK; }
	ULONG Release() { return 0; }
};

inline HRESULT Direct3DCreate9Ex(UINT, IDirect3D9Ex**) { return S_OK; }

// LPD3DXBUFFER — originally ID3DXBuffer, API-compatible with ID3DBlob.
// The codebase calls GetBufferPointer() and GetBufferSize() on it.
typedef ID3DBlob* LPD3DXBUFFER;

#endif // __D3D9_COMPAT_H__