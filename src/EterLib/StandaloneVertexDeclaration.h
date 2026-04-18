#pragma once

//////////////////////////////////////////////////////////////////////////////
// StandaloneVertexDeclaration.h
//
// A lightweight IDirect3DVertexDeclaration9 factory that stores vertex
// elements without requiring a real D3D9 device. Used during the D3D9->D3D11
// migration to keep vertex declaration creation working after the D3D9 device
// is removed.
//////////////////////////////////////////////////////////////////////////////

#include "D3D9Compat.h"

inline LPDIRECT3DVERTEXDECLARATION9 CreateStandaloneVertexDeclaration(const D3DVERTEXELEMENT9* pElements)
{
	IDirect3DVertexDeclaration9* pDecl = new IDirect3DVertexDeclaration9();
	pDecl->m_numElements = 0;
	for (UINT i = 0; i < MAX_FVF_DECL_SIZE; ++i)
	{
		pDecl->m_elements[i] = pElements[i];
		++pDecl->m_numElements;
		if (pElements[i].Stream == 0xFF)
			break;
	}
	return pDecl;
}
