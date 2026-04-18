#pragma once
#ifndef __D3DX9MATH_H__
#define __D3DX9MATH_H__

//////////////////////////////////////////////////////////////////////////////
// D3DXMathCompat.h — D3DX math type/function replacements for D3D11 migration
//
// Provides binary-compatible D3DX math types (D3DXVECTOR2, D3DXVECTOR3,
// D3DXVECTOR4, D3DXMATRIX, D3DXCOLOR, D3DXQUATERNION, D3DXPLANE) and all
// D3DX math functions used by the codebase, implemented via DirectXMath.
//
// The guard macro __D3DX9MATH_H__ matches what d3dx9math.h uses, so code
// that checks for it (e.g. D3D9Compat.h) will see these types as available.
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <math.h>
#include <float.h>
#include <DirectXMath.h>
#include <dxgitype.h>    // D3DCOLORVALUE (needed by D3DXCOLOR conversion)

#ifndef D3DX_PI
#define D3DX_PI 3.14159265358979323846f
#endif

inline float D3DXToRadian(float degree) { return degree * (D3DX_PI / 180.0f); }
inline float D3DXToDegree(float radian) { return radian * (180.0f / D3DX_PI); }

//////////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////////
struct D3DXVECTOR2;
struct D3DXVECTOR3;
struct D3DXVECTOR4;
struct D3DXQUATERNION;
struct D3DXPLANE;
struct D3DXMATRIX;
struct D3DXCOLOR;

//////////////////////////////////////////////////////////////////////////////
// D3DXVECTOR2
//////////////////////////////////////////////////////////////////////////////
struct D3DXVECTOR2
{
	float x, y;

	D3DXVECTOR2() : x(0), y(0) {}
	D3DXVECTOR2(float _x, float _y) : x(_x), y(_y) {}
	D3DXVECTOR2(const float* pf) : x(pf[0]), y(pf[1]) {}

	operator float* () { return &x; }
	operator const float* () const { return &x; }

	D3DXVECTOR2 operator + (const D3DXVECTOR2& v) const { return D3DXVECTOR2(x + v.x, y + v.y); }
	D3DXVECTOR2 operator - (const D3DXVECTOR2& v) const { return D3DXVECTOR2(x - v.x, y - v.y); }
	D3DXVECTOR2 operator * (float f) const { return D3DXVECTOR2(x * f, y * f); }
	D3DXVECTOR2 operator / (float f) const { float inv = 1.0f / f; return D3DXVECTOR2(x * inv, y * inv); }

	D3DXVECTOR2& operator += (const D3DXVECTOR2& v) { x += v.x; y += v.y; return *this; }
	D3DXVECTOR2& operator -= (const D3DXVECTOR2& v) { x -= v.x; y -= v.y; return *this; }
	D3DXVECTOR2& operator *= (float f) { x *= f; y *= f; return *this; }
	D3DXVECTOR2& operator /= (float f) { float inv = 1.0f / f; x *= inv; y *= inv; return *this; }

	D3DXVECTOR2 operator - () const { return D3DXVECTOR2(-x, -y); }

	bool operator == (const D3DXVECTOR2& v) const { return x == v.x && y == v.y; }
	bool operator != (const D3DXVECTOR2& v) const { return x != v.x || y != v.y; }

	friend D3DXVECTOR2 operator * (float f, const D3DXVECTOR2& v) { return D3DXVECTOR2(f * v.x, f * v.y); }
};

//////////////////////////////////////////////////////////////////////////////
// D3DVECTOR — base 3-component vector (guarded, may also come from D3D9Compat.h)
//////////////////////////////////////////////////////////////////////////////
#ifndef D3DVECTOR_DEFINED
#define D3DVECTOR_DEFINED
typedef struct _D3DVECTOR {
	float x, y, z;
} D3DVECTOR;
#endif

//////////////////////////////////////////////////////////////////////////////
// D3DXVECTOR3 — inherits from _D3DVECTOR for type compatibility (matches D3DX SDK)
//////////////////////////////////////////////////////////////////////////////
struct D3DXVECTOR3 : public _D3DVECTOR
{
	D3DXVECTOR3() { x = 0; y = 0; z = 0; }
	D3DXVECTOR3(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }
	D3DXVECTOR3(const float* pf) { x = pf[0]; y = pf[1]; z = pf[2]; }
	D3DXVECTOR3(const _D3DVECTOR& v) { x = v.x; y = v.y; z = v.z; }

	operator float* () { return &x; }
	operator const float* () const { return &x; }

	D3DXVECTOR3 operator + (const D3DXVECTOR3& v) const { return D3DXVECTOR3(x + v.x, y + v.y, z + v.z); }
	D3DXVECTOR3 operator - (const D3DXVECTOR3& v) const { return D3DXVECTOR3(x - v.x, y - v.y, z - v.z); }
	D3DXVECTOR3 operator * (float f) const { return D3DXVECTOR3(x * f, y * f, z * f); }
	D3DXVECTOR3 operator / (float f) const { float inv = 1.0f / f; return D3DXVECTOR3(x * inv, y * inv, z * inv); }

	D3DXVECTOR3& operator += (const D3DXVECTOR3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	D3DXVECTOR3& operator -= (const D3DXVECTOR3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	D3DXVECTOR3& operator *= (float f) { x *= f; y *= f; z *= f; return *this; }
	D3DXVECTOR3& operator /= (float f) { float inv = 1.0f / f; x *= inv; y *= inv; z *= inv; return *this; }

	D3DXVECTOR3 operator - () const { return D3DXVECTOR3(-x, -y, -z); }

	bool operator == (const D3DXVECTOR3& v) const { return x == v.x && y == v.y && z == v.z; }
	bool operator != (const D3DXVECTOR3& v) const { return x != v.x || y != v.y || z != v.z; }

	friend D3DXVECTOR3 operator * (float f, const D3DXVECTOR3& v) { return D3DXVECTOR3(f * v.x, f * v.y, f * v.z); }
};

//////////////////////////////////////////////////////////////////////////////
// D3DXVECTOR4
//////////////////////////////////////////////////////////////////////////////
struct D3DXVECTOR4
{
	float x, y, z, w;

	D3DXVECTOR4() : x(0), y(0), z(0), w(0) {}
	D3DXVECTOR4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	D3DXVECTOR4(const float* pf) : x(pf[0]), y(pf[1]), z(pf[2]), w(pf[3]) {}

	operator float* () { return &x; }
	operator const float* () const { return &x; }

	D3DXVECTOR4 operator + (const D3DXVECTOR4& v) const { return D3DXVECTOR4(x + v.x, y + v.y, z + v.z, w + v.w); }
	D3DXVECTOR4 operator - (const D3DXVECTOR4& v) const { return D3DXVECTOR4(x - v.x, y - v.y, z - v.z, w - v.w); }
	D3DXVECTOR4 operator * (float f) const { return D3DXVECTOR4(x * f, y * f, z * f, w * f); }
	D3DXVECTOR4 operator / (float f) const { float inv = 1.0f / f; return D3DXVECTOR4(x * inv, y * inv, z * inv, w * inv); }

	D3DXVECTOR4& operator += (const D3DXVECTOR4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	D3DXVECTOR4& operator -= (const D3DXVECTOR4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	D3DXVECTOR4& operator *= (float f) { x *= f; y *= f; z *= f; w *= f; return *this; }

	D3DXVECTOR4 operator - () const { return D3DXVECTOR4(-x, -y, -z, -w); }

	bool operator == (const D3DXVECTOR4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
	bool operator != (const D3DXVECTOR4& v) const { return !(*this == v); }

	friend D3DXVECTOR4 operator * (float f, const D3DXVECTOR4& v) { return D3DXVECTOR4(f * v.x, f * v.y, f * v.z, f * v.w); }
};

//////////////////////////////////////////////////////////////////////////////
// D3DXQUATERNION
//////////////////////////////////////////////////////////////////////////////
struct D3DXQUATERNION
{
	float x, y, z, w;

	D3DXQUATERNION() : x(0), y(0), z(0), w(1) {}
	D3DXQUATERNION(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	D3DXQUATERNION(const float* pf) : x(pf[0]), y(pf[1]), z(pf[2]), w(pf[3]) {}

	operator float* () { return &x; }
	operator const float* () const { return &x; }

	D3DXQUATERNION operator + (const D3DXQUATERNION& q) const { return D3DXQUATERNION(x + q.x, y + q.y, z + q.z, w + q.w); }
	D3DXQUATERNION operator - (const D3DXQUATERNION& q) const { return D3DXQUATERNION(x - q.x, y - q.y, z - q.z, w - q.w); }
	D3DXQUATERNION operator * (float f) const { return D3DXQUATERNION(x * f, y * f, z * f, w * f); }
	D3DXQUATERNION operator / (float f) const { float inv = 1.0f / f; return D3DXQUATERNION(x * inv, y * inv, z * inv, w * inv); }

	D3DXQUATERNION& operator += (const D3DXQUATERNION& q) { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
	D3DXQUATERNION& operator -= (const D3DXQUATERNION& q) { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
	D3DXQUATERNION& operator *= (float f) { x *= f; y *= f; z *= f; w *= f; return *this; }

	D3DXQUATERNION operator - () const { return D3DXQUATERNION(-x, -y, -z, -w); }

	bool operator == (const D3DXQUATERNION& q) const { return x == q.x && y == q.y && z == q.z && w == q.w; }
	bool operator != (const D3DXQUATERNION& q) const { return !(*this == q); }

	friend D3DXQUATERNION operator * (float f, const D3DXQUATERNION& q) { return D3DXQUATERNION(f * q.x, f * q.y, f * q.z, f * q.w); }
};

//////////////////////////////////////////////////////////////////////////////
// D3DXPLANE
//////////////////////////////////////////////////////////////////////////////
struct D3DXPLANE
{
	float a, b, c, d;

	D3DXPLANE() : a(0), b(0), c(0), d(0) {}
	D3DXPLANE(float _a, float _b, float _c, float _d) : a(_a), b(_b), c(_c), d(_d) {}
	D3DXPLANE(const float* pf) : a(pf[0]), b(pf[1]), c(pf[2]), d(pf[3]) {}

	operator float* () { return &a; }
	operator const float* () const { return &a; }

	bool operator == (const D3DXPLANE& p) const { return a == p.a && b == p.b && c == p.c && d == p.d; }
	bool operator != (const D3DXPLANE& p) const { return !(*this == p); }
};

//////////////////////////////////////////////////////////////////////////////
// D3DXMATRIX — row-major 4x4 matrix, binary-compatible with D3DX
//////////////////////////////////////////////////////////////////////////////
struct D3DXMATRIX
{
	union {
		struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};

	D3DXMATRIX() { memset(this, 0, sizeof(D3DXMATRIX)); }

	D3DXMATRIX(float m11, float m12, float m13, float m14,
	           float m21, float m22, float m23, float m24,
	           float m31, float m32, float m33, float m34,
	           float m41, float m42, float m43, float m44)
	{
		_11=m11; _12=m12; _13=m13; _14=m14;
		_21=m21; _22=m22; _23=m23; _24=m24;
		_31=m31; _32=m32; _33=m33; _34=m34;
		_41=m41; _42=m42; _43=m43; _44=m44;
	}

	float& operator () (UINT row, UINT col) { return m[row][col]; }
	float operator () (UINT row, UINT col) const { return m[row][col]; }

	operator float* () { return &_11; }
	operator const float* () const { return &_11; }

	D3DXMATRIX operator * (const D3DXMATRIX& rhs) const
	{
		D3DXMATRIX r;
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				r.m[i][j] = m[i][0]*rhs.m[0][j] + m[i][1]*rhs.m[1][j] + m[i][2]*rhs.m[2][j] + m[i][3]*rhs.m[3][j];
		return r;
	}

	D3DXMATRIX& operator *= (const D3DXMATRIX& rhs) { *this = *this * rhs; return *this; }

	D3DXMATRIX operator + (const D3DXMATRIX& rhs) const
	{
		D3DXMATRIX r;
		const float* a = &_11; const float* b = &rhs._11; float* c = &r._11;
		for (int i = 0; i < 16; i++) c[i] = a[i] + b[i];
		return r;
	}

	D3DXMATRIX operator - (const D3DXMATRIX& rhs) const
	{
		D3DXMATRIX r;
		const float* a = &_11; const float* b = &rhs._11; float* c = &r._11;
		for (int i = 0; i < 16; i++) c[i] = a[i] - b[i];
		return r;
	}

	D3DXMATRIX operator * (float f) const
	{
		D3DXMATRIX r;
		const float* a = &_11; float* c = &r._11;
		for (int i = 0; i < 16; i++) c[i] = a[i] * f;
		return r;
	}

	D3DXMATRIX operator / (float f) const { return *this * (1.0f / f); }

	bool operator == (const D3DXMATRIX& rhs) const { return memcmp(this, &rhs, sizeof(D3DXMATRIX)) == 0; }
	bool operator != (const D3DXMATRIX& rhs) const { return !(*this == rhs); }

	friend D3DXMATRIX operator * (float f, const D3DXMATRIX& mat) { return mat * f; }
};

//////////////////////////////////////////////////////////////////////////////
// D3DXCOLOR — float4 color with DWORD conversion
// D3DCOLORVALUE comes from <dxgitype.h> included above.
//////////////////////////////////////////////////////////////////////////////
struct D3DXCOLOR
{
	float r, g, b, a;

	D3DXCOLOR() : r(0), g(0), b(0), a(0) {}
	D3DXCOLOR(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
	D3DXCOLOR(const float* pf) : r(pf[0]), g(pf[1]), b(pf[2]), a(pf[3]) {}
	D3DXCOLOR(const D3DCOLORVALUE& cv) : r(cv.r), g(cv.g), b(cv.b), a(cv.a) {}

	D3DXCOLOR(DWORD argb)
	{
		const float f = 1.0f / 255.0f;
		a = f * (float)(unsigned char)(argb >> 24);
		r = f * (float)(unsigned char)(argb >> 16);
		g = f * (float)(unsigned char)(argb >> 8);
		b = f * (float)(unsigned char)(argb);
	}

	operator D3DCOLORVALUE () const { D3DCOLORVALUE c = {r, g, b, a}; return c; }

	operator DWORD () const
	{
		DWORD dwR = (r >= 1.0f) ? 0xff : (r <= 0.0f) ? 0x00 : (DWORD)(r * 255.0f + 0.5f);
		DWORD dwG = (g >= 1.0f) ? 0xff : (g <= 0.0f) ? 0x00 : (DWORD)(g * 255.0f + 0.5f);
		DWORD dwB = (b >= 1.0f) ? 0xff : (b <= 0.0f) ? 0x00 : (DWORD)(b * 255.0f + 0.5f);
		DWORD dwA = (a >= 1.0f) ? 0xff : (a <= 0.0f) ? 0x00 : (DWORD)(a * 255.0f + 0.5f);
		return (dwA << 24) | (dwR << 16) | (dwG << 8) | dwB;
	}

	operator float* () { return &r; }
	operator const float* () const { return &r; }

	D3DXCOLOR operator + (const D3DXCOLOR& c) const { return D3DXCOLOR(r+c.r, g+c.g, b+c.b, a+c.a); }
	D3DXCOLOR operator - (const D3DXCOLOR& c) const { return D3DXCOLOR(r-c.r, g-c.g, b-c.b, a-c.a); }
	D3DXCOLOR operator * (float f) const { return D3DXCOLOR(r*f, g*f, b*f, a*f); }
	D3DXCOLOR operator / (float f) const { float inv = 1.0f/f; return D3DXCOLOR(r*inv, g*inv, b*inv, a*inv); }

	D3DXCOLOR& operator += (const D3DXCOLOR& c) { r+=c.r; g+=c.g; b+=c.b; a+=c.a; return *this; }
	D3DXCOLOR& operator -= (const D3DXCOLOR& c) { r-=c.r; g-=c.g; b-=c.b; a-=c.a; return *this; }
	D3DXCOLOR& operator *= (float f) { r*=f; g*=f; b*=f; a*=f; return *this; }

	D3DXCOLOR operator - () const { return D3DXCOLOR(-r, -g, -b, -a); }

	bool operator == (const D3DXCOLOR& c) const { return r==c.r && g==c.g && b==c.b && a==c.a; }
	bool operator != (const D3DXCOLOR& c) const { return !(*this == c); }

	friend D3DXCOLOR operator * (float f, const D3DXCOLOR& c) { return D3DXCOLOR(f*c.r, f*c.g, f*c.b, f*c.a); }
};

//////////////////////////////////////////////////////////////////////////////
// D3DXColorModulate
//////////////////////////////////////////////////////////////////////////////
inline D3DXCOLOR* D3DXColorModulate(D3DXCOLOR* pOut, const D3DXCOLOR* pC1, const D3DXCOLOR* pC2)
{
	pOut->r = pC1->r * pC2->r;
	pOut->g = pC1->g * pC2->g;
	pOut->b = pC1->b * pC2->b;
	pOut->a = pC1->a * pC2->a;
	return pOut;
}

//////////////////////////////////////////////////////////////////////////////
// Internal helpers: load/store between our types and XMVECTOR/XMMATRIX
//////////////////////////////////////////////////////////////////////////////
namespace D3DXMathInternal
{
	using namespace DirectX;
	inline XMVECTOR LoadVec2(const D3DXVECTOR2* p) { return XMLoadFloat2((const XMFLOAT2*)p); }
	inline XMVECTOR LoadVec3(const D3DXVECTOR3* p) { return XMLoadFloat3((const XMFLOAT3*)p); }
	inline XMVECTOR LoadVec4(const D3DXVECTOR4* p) { return XMLoadFloat4((const XMFLOAT4*)p); }
	inline XMVECTOR LoadQuat(const D3DXQUATERNION* p) { return XMLoadFloat4((const XMFLOAT4*)p); }
	inline XMVECTOR LoadPlane(const D3DXPLANE* p) { return XMLoadFloat4((const XMFLOAT4*)p); }
	inline XMMATRIX LoadMat(const D3DXMATRIX* p) { return XMLoadFloat4x4((const XMFLOAT4X4*)p); }
	inline void StoreVec2(D3DXVECTOR2* p, FXMVECTOR v) { XMStoreFloat2((XMFLOAT2*)p, v); }
	inline void StoreVec3(D3DXVECTOR3* p, FXMVECTOR v) { XMStoreFloat3((XMFLOAT3*)p, v); }
	inline void StoreVec4(D3DXVECTOR4* p, FXMVECTOR v) { XMStoreFloat4((XMFLOAT4*)p, v); }
	inline void StoreQuat(D3DXQUATERNION* p, FXMVECTOR v) { XMStoreFloat4((XMFLOAT4*)p, v); }
	inline void StorePlane(D3DXPLANE* p, FXMVECTOR v) { XMStoreFloat4((XMFLOAT4*)p, v); }
	inline void StoreMat(D3DXMATRIX* p, CXMMATRIX m) { XMStoreFloat4x4((XMFLOAT4X4*)p, m); }
}

//////////////////////////////////////////////////////////////////////////////
// Vec2 functions
//////////////////////////////////////////////////////////////////////////////
inline float D3DXVec2Length(const D3DXVECTOR2* pV)
{
	return sqrtf(pV->x * pV->x + pV->y * pV->y);
}

inline float D3DXVec2LengthSq(const D3DXVECTOR2* pV)
{
	return pV->x * pV->x + pV->y * pV->y;
}

inline float D3DXVec2Dot(const D3DXVECTOR2* pV1, const D3DXVECTOR2* pV2)
{
	return pV1->x * pV2->x + pV1->y * pV2->y;
}

inline float D3DXVec2CCW(const D3DXVECTOR2* pV1, const D3DXVECTOR2* pV2)
{
	return pV1->x * pV2->y - pV1->y * pV2->x;
}

inline D3DXVECTOR2* D3DXVec2Normalize(D3DXVECTOR2* pOut, const D3DXVECTOR2* pV)
{
	float len = D3DXVec2Length(pV);
	if (len > FLT_EPSILON) { float inv = 1.0f / len; pOut->x = pV->x * inv; pOut->y = pV->y * inv; }
	else { pOut->x = 0; pOut->y = 0; }
	return pOut;
}

//////////////////////////////////////////////////////////////////////////////
// Vec3 functions
//////////////////////////////////////////////////////////////////////////////
inline float D3DXVec3Length(const D3DXVECTOR3* pV)
{
	return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
}

inline float D3DXVec3LengthSq(const D3DXVECTOR3* pV)
{
	return pV->x * pV->x + pV->y * pV->y + pV->z * pV->z;
}

inline float D3DXVec3Dot(const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2)
{
	return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

inline D3DXVECTOR3* D3DXVec3Cross(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2)
{
	D3DXVECTOR3 tmp;
	tmp.x = pV1->y * pV2->z - pV1->z * pV2->y;
	tmp.y = pV1->z * pV2->x - pV1->x * pV2->z;
	tmp.z = pV1->x * pV2->y - pV1->y * pV2->x;
	*pOut = tmp;
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV)
{
	float len = D3DXVec3Length(pV);
	if (len > FLT_EPSILON) { float inv = 1.0f / len; pOut->x = pV->x*inv; pOut->y = pV->y*inv; pOut->z = pV->z*inv; }
	else { pOut->x = 0; pOut->y = 0; pOut->z = 0; }
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3Scale(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, float s)
{
	pOut->x = pV->x * s; pOut->y = pV->y * s; pOut->z = pV->z * s;
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3Add(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2)
{
	pOut->x = pV1->x + pV2->x; pOut->y = pV1->y + pV2->y; pOut->z = pV1->z + pV2->z;
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3Subtract(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2)
{
	pOut->x = pV1->x - pV2->x; pOut->y = pV1->y - pV2->y; pOut->z = pV1->z - pV2->z;
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3Lerp(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2, float s)
{
	pOut->x = pV1->x + s * (pV2->x - pV1->x);
	pOut->y = pV1->y + s * (pV2->y - pV1->y);
	pOut->z = pV1->z + s * (pV2->z - pV1->z);
	return pOut;
}

inline D3DXVECTOR4* D3DXVec3Transform(D3DXVECTOR4* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
	using namespace DirectX;
	XMVECTOR v = D3DXMathInternal::LoadVec3(pV);
	XMMATRIX m = D3DXMathInternal::LoadMat(pM);
	XMVECTOR r = XMVector3Transform(v, m);
	D3DXMathInternal::StoreVec4(pOut, r);
	return pOut;
}

//////////////////////////////////////////////////////////////////////////////
// Vec3 transform functions
//////////////////////////////////////////////////////////////////////////////
inline D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
	using namespace DirectX;
	XMVECTOR v = D3DXMathInternal::LoadVec3(pV);
	XMMATRIX m = D3DXMathInternal::LoadMat(pM);
	XMVECTOR r = XMVector3TransformCoord(v, m);
	D3DXMathInternal::StoreVec3(pOut, r);
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3TransformNormal(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
	using namespace DirectX;
	XMVECTOR v = D3DXMathInternal::LoadVec3(pV);
	XMMATRIX m = D3DXMathInternal::LoadMat(pM);
	XMVECTOR r = XMVector3TransformNormal(v, m);
	D3DXMathInternal::StoreVec3(pOut, r);
	return pOut;
}

// D3DVIEWPORT9 — needed by project/unproject functions.
// May already be defined by D3D9Compat.h; guarded to avoid redefinition.
#ifndef _D3DVIEWPORT9_DEFINED
#define _D3DVIEWPORT9_DEFINED
typedef struct _D3DVIEWPORT9 {
	DWORD X, Y;
	DWORD Width, Height;
	float MinZ, MaxZ;
} D3DVIEWPORT9;
#endif

inline D3DXVECTOR3* D3DXVec3Project(
	D3DXVECTOR3* pOut,
	const D3DXVECTOR3* pV,
	const D3DVIEWPORT9* pViewport,
	const D3DXMATRIX* pProjection,
	const D3DXMATRIX* pView,
	const D3DXMATRIX* pWorld)
{
	using namespace DirectX;
	XMVECTOR v = D3DXMathInternal::LoadVec3(pV);
	XMMATRIX world = pWorld ? D3DXMathInternal::LoadMat(pWorld) : XMMatrixIdentity();
	XMMATRIX view = pView ? D3DXMathInternal::LoadMat(pView) : XMMatrixIdentity();
	XMMATRIX proj = pProjection ? D3DXMathInternal::LoadMat(pProjection) : XMMatrixIdentity();
	XMMATRIX wvp = world * view * proj;
	XMVECTOR r = XMVector3TransformCoord(v, wvp);

	float halfW = pViewport->Width * 0.5f;
	float halfH = pViewport->Height * 0.5f;
	pOut->x = XMVectorGetX(r) * halfW + (pViewport->X + halfW);
	pOut->y = -XMVectorGetY(r) * halfH + (pViewport->Y + halfH);
	pOut->z = XMVectorGetZ(r) * (pViewport->MaxZ - pViewport->MinZ) + pViewport->MinZ;
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3Unproject(
	D3DXVECTOR3* pOut,
	const D3DXVECTOR3* pV,
	const D3DVIEWPORT9* pViewport,
	const D3DXMATRIX* pProjection,
	const D3DXMATRIX* pView,
	const D3DXMATRIX* pWorld)
{
	using namespace DirectX;
	XMMATRIX world = pWorld ? D3DXMathInternal::LoadMat(pWorld) : XMMatrixIdentity();
	XMMATRIX view = pView ? D3DXMathInternal::LoadMat(pView) : XMMatrixIdentity();
	XMMATRIX proj = pProjection ? D3DXMathInternal::LoadMat(pProjection) : XMMatrixIdentity();
	XMMATRIX wvp = world * view * proj;
	XMMATRIX inv = XMMatrixInverse(nullptr, wvp);

	XMVECTOR v;
	float halfW = pViewport->Width * 0.5f;
	float halfH = pViewport->Height * 0.5f;
	float nx = (pV->x - pViewport->X - halfW) / halfW;
	float ny = -(pV->y - pViewport->Y - halfH) / halfH;
	float nz = (pViewport->MaxZ != pViewport->MinZ) ? (pV->z - pViewport->MinZ) / (pViewport->MaxZ - pViewport->MinZ) : pV->z;
	v = XMVectorSet(nx, ny, nz, 1.0f);
	XMVECTOR r = XMVector3TransformCoord(v, inv);
	D3DXMathInternal::StoreVec3(pOut, r);
	return pOut;
}

//////////////////////////////////////////////////////////////////////////////
// Vec4 transform
//////////////////////////////////////////////////////////////////////////////
inline D3DXVECTOR4* D3DXVec4Transform(D3DXVECTOR4* pOut, const D3DXVECTOR4* pV, const D3DXMATRIX* pM)
{
	using namespace DirectX;
	XMVECTOR v = D3DXMathInternal::LoadVec4(pV);
	XMMATRIX m = D3DXMathInternal::LoadMat(pM);
	XMVECTOR r = XMVector4Transform(v, m);
	D3DXMathInternal::StoreVec4(pOut, r);
	return pOut;
}

//////////////////////////////////////////////////////////////////////////////
// Matrix functions
//////////////////////////////////////////////////////////////////////////////
inline D3DXMATRIX* D3DXMatrixIdentity(D3DXMATRIX* pOut)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixIdentity());
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixMultiply(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2)
{
	using namespace DirectX;
	XMMATRIX m1 = D3DXMathInternal::LoadMat(pM1);
	XMMATRIX m2 = D3DXMathInternal::LoadMat(pM2);
	D3DXMathInternal::StoreMat(pOut, XMMatrixMultiply(m1, m2));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixTranspose(D3DXMATRIX* pOut, const D3DXMATRIX* pM)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixTranspose(D3DXMathInternal::LoadMat(pM)));
	return pOut;
}

inline float D3DXMatrixDeterminant(const D3DXMATRIX* pM)
{
	using namespace DirectX;
	XMVECTOR det = XMMatrixDeterminant(D3DXMathInternal::LoadMat(pM));
	return XMVectorGetX(det);
}

inline D3DXMATRIX* D3DXMatrixInverse(D3DXMATRIX* pOut, float* pDeterminant, const D3DXMATRIX* pM)
{
	using namespace DirectX;
	XMVECTOR det;
	XMMATRIX inv = XMMatrixInverse(&det, D3DXMathInternal::LoadMat(pM));
	if (pDeterminant) *pDeterminant = XMVectorGetX(det);
	D3DXMathInternal::StoreMat(pOut, inv);
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixTranslation(D3DXMATRIX* pOut, float x, float y, float z)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixTranslation(x, y, z));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixScaling(D3DXMATRIX* pOut, float sx, float sy, float sz)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixScaling(sx, sy, sz));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationX(D3DXMATRIX* pOut, float angle)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixRotationX(angle));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationY(D3DXMATRIX* pOut, float angle)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixRotationY(angle));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationZ(D3DXMATRIX* pOut, float angle)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixRotationZ(angle));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationAxis(D3DXMATRIX* pOut, const D3DXVECTOR3* pV, float angle)
{
	using namespace DirectX;
	XMVECTOR axis = D3DXMathInternal::LoadVec3(pV);
	D3DXMathInternal::StoreMat(pOut, XMMatrixRotationAxis(axis, angle));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationQuaternion(D3DXMATRIX* pOut, const D3DXQUATERNION* pQ)
{
	using namespace DirectX;
	XMVECTOR q = D3DXMathInternal::LoadQuat(pQ);
	D3DXMathInternal::StoreMat(pOut, XMMatrixRotationQuaternion(q));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationYawPitchRoll(D3DXMATRIX* pOut, float yaw, float pitch, float roll)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixOrthoOffCenterRH(D3DXMATRIX* pOut, float l, float r, float b, float t, float zn, float zf)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixOrthographicOffCenterRH(l, r, b, t, zn, zf));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixOrthoRH(D3DXMATRIX* pOut, float w, float h, float zn, float zf)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixOrthographicRH(w, h, zn, zf));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixLookAtRH(D3DXMATRIX* pOut, const D3DXVECTOR3* pEye, const D3DXVECTOR3* pAt, const D3DXVECTOR3* pUp)
{
	using namespace DirectX;
	XMVECTOR eye = D3DXMathInternal::LoadVec3(pEye);
	XMVECTOR at = D3DXMathInternal::LoadVec3(pAt);
	XMVECTOR up = D3DXMathInternal::LoadVec3(pUp);
	D3DXMathInternal::StoreMat(pOut, XMMatrixLookAtRH(eye, at, up));
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixPerspectiveFovRH(D3DXMATRIX* pOut, float fovy, float aspect, float zn, float zf)
{
	using namespace DirectX;
	D3DXMathInternal::StoreMat(pOut, XMMatrixPerspectiveFovRH(fovy, aspect, zn, zf));
	return pOut;
}

//////////////////////////////////////////////////////////////////////////////
// Plane functions
//////////////////////////////////////////////////////////////////////////////
inline float D3DXPlaneDotCoord(const D3DXPLANE* pP, const D3DXVECTOR3* pV)
{
	return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z + pP->d;
}

inline D3DXPLANE* D3DXPlaneNormalize(D3DXPLANE* pOut, const D3DXPLANE* pP)
{
	float len = sqrtf(pP->a * pP->a + pP->b * pP->b + pP->c * pP->c);
	if (len > FLT_EPSILON) { float inv = 1.0f / len; pOut->a = pP->a*inv; pOut->b = pP->b*inv; pOut->c = pP->c*inv; pOut->d = pP->d*inv; }
	else { pOut->a = 0; pOut->b = 0; pOut->c = 0; pOut->d = 0; }
	return pOut;
}

//////////////////////////////////////////////////////////////////////////////
// Quaternion functions
//////////////////////////////////////////////////////////////////////////////
inline D3DXQUATERNION* D3DXQuaternionRotationMatrix(D3DXQUATERNION* pOut, const D3DXMATRIX* pM)
{
	using namespace DirectX;
	XMVECTOR q = XMQuaternionRotationMatrix(D3DXMathInternal::LoadMat(pM));
	D3DXMathInternal::StoreQuat(pOut, q);
	return pOut;
}

inline D3DXQUATERNION* D3DXQuaternionRotationAxis(D3DXQUATERNION* pOut, const D3DXVECTOR3* pV, float angle)
{
	using namespace DirectX;
	XMVECTOR axis = D3DXMathInternal::LoadVec3(pV);
	XMVECTOR q = XMQuaternionRotationAxis(axis, angle);
	D3DXMathInternal::StoreQuat(pOut, q);
	return pOut;
}

inline D3DXQUATERNION* D3DXQuaternionSlerp(D3DXQUATERNION* pOut, const D3DXQUATERNION* pQ1, const D3DXQUATERNION* pQ2, float t)
{
	using namespace DirectX;
	XMVECTOR q1 = D3DXMathInternal::LoadQuat(pQ1);
	XMVECTOR q2 = D3DXMathInternal::LoadQuat(pQ2);
	XMVECTOR r = XMQuaternionSlerp(q1, q2, t);
	D3DXMathInternal::StoreQuat(pOut, r);
	return pOut;
}

inline D3DXQUATERNION* D3DXQuaternionMultiply(D3DXQUATERNION* pOut, const D3DXQUATERNION* pQ1, const D3DXQUATERNION* pQ2)
{
	using namespace DirectX;
	XMVECTOR q1 = D3DXMathInternal::LoadQuat(pQ1);
	XMVECTOR q2 = D3DXMathInternal::LoadQuat(pQ2);
	XMVECTOR r = XMQuaternionMultiply(q1, q2);
	D3DXMathInternal::StoreQuat(pOut, r);
	return pOut;
}

inline D3DXQUATERNION* D3DXQuaternionConjugate(D3DXQUATERNION* pOut, const D3DXQUATERNION* pQ)
{
	using namespace DirectX;
	XMVECTOR q = D3DXMathInternal::LoadQuat(pQ);
	XMVECTOR r = XMQuaternionConjugate(q);
	D3DXMathInternal::StoreQuat(pOut, r);
	return pOut;
}

inline D3DXQUATERNION* D3DXQuaternionRotationYawPitchRoll(D3DXQUATERNION* pOut, float yaw, float pitch, float roll)
{
	using namespace DirectX;
	XMVECTOR q = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
	D3DXMathInternal::StoreQuat(pOut, q);
	return pOut;
}

//////////////////////////////////////////////////////////////////////////////
// D3DX stubs — matrix stack and mesh interfaces used by CGraphicBase
//////////////////////////////////////////////////////////////////////////////

struct ID3DXMatrixStack
{
	D3DXMATRIX m_stack[32];
	int m_top;

	ID3DXMatrixStack() : m_top(0) { D3DXMatrixIdentity(&m_stack[0]); }
	void LoadIdentity() { D3DXMatrixIdentity(&m_stack[m_top]); }
	void LoadMatrix(const D3DXMATRIX* pM) { m_stack[m_top] = *pM; }
	void MultMatrix(const D3DXMATRIX* pM) { D3DXMatrixMultiply(&m_stack[m_top], &m_stack[m_top], pM); }
	void MultMatrixLocal(const D3DXMATRIX* pM) { D3DXMatrixMultiply(&m_stack[m_top], pM, &m_stack[m_top]); }
	void Scale(float x, float y, float z)
	{
		D3DXMATRIX s; D3DXMatrixScaling(&s, x, y, z);
		D3DXMatrixMultiply(&m_stack[m_top], &m_stack[m_top], &s);
	}
	void Translate(float x, float y, float z)
	{
		D3DXMATRIX t; D3DXMatrixTranslation(&t, x, y, z);
		D3DXMatrixMultiply(&m_stack[m_top], &m_stack[m_top], &t);
	}
	void RotateAxis(const D3DXVECTOR3* pV, float angle)
	{
		D3DXMATRIX r; D3DXMatrixRotationAxis(&r, pV, angle);
		D3DXMatrixMultiply(&m_stack[m_top], &m_stack[m_top], &r);
	}
	void RotateAxisLocal(const D3DXVECTOR3* pV, float angle)
	{
		D3DXMATRIX r; D3DXMatrixRotationAxis(&r, pV, angle);
		D3DXMatrixMultiply(&m_stack[m_top], &r, &m_stack[m_top]);
	}
	void RotateYawPitchRollLocal(float yaw, float pitch, float roll)
	{
		D3DXMATRIX r; D3DXMatrixRotationYawPitchRoll(&r, yaw, pitch, roll);
		D3DXMatrixMultiply(&m_stack[m_top], &r, &m_stack[m_top]);
	}
	void Push() { if (m_top < 31) { m_stack[m_top + 1] = m_stack[m_top]; ++m_top; } }
	void Pop() { if (m_top > 0) --m_top; }
	D3DXMATRIX* GetTop() { return &m_stack[m_top]; }
	ULONG Release() { return 0; }
};

inline HRESULT D3DXCreateMatrixStack(DWORD, ID3DXMatrixStack** ppStack)
{
	*ppStack = new ID3DXMatrixStack();
	return S_OK;
}

// LPD3DXMESH stub — used for debug sphere/cylinder rendering
struct ID3DXMesh
{
	ULONG Release() { return 0; }
};
typedef ID3DXMesh* LPD3DXMESH;

inline HRESULT D3DXCreateSphere(void*, float, UINT, UINT, LPD3DXMESH*, void*) { return S_OK; }
inline HRESULT D3DXCreateCylinder(void*, float, float, float, UINT, UINT, LPD3DXMESH*, void*) { return S_OK; }

#endif // __D3DX9MATH_H__