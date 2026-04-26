#pragma once

#include "Material.h"

extern granny_data_type_definition GrannyPNT3322VertexType[5];
extern granny_data_type_definition GrannySkinnedPNTVertexType[6];
extern granny_data_type_definition GrannySkinnedPNT2VertexType[7];

struct granny_pnt3322_vertex
{
    granny_real32 Position[3];
    granny_real32 Normal[3];
    granny_real32 UV0[2];
    granny_real32 UV1[2];
};

struct TGrannySkinnedPNTVertex
{
    granny_real32 Position[3];
    granny_real32 Normal[3];
    granny_real32 BoneWeights[4];
    granny_uint32 BoneIndices[4];
    granny_real32 UV0[2];
};

struct TGrannySkinnedPNT2Vertex
{
    granny_real32 Position[3];
    granny_real32 Normal[3];
    granny_real32 BoneWeights[4];
    granny_uint32 BoneIndices[4];
    granny_real32 UV0[2];
    granny_real32 UV1[2];
};

class CGrannyMesh
{
	public:
		enum EType
		{
			TYPE_RIGID,
			TYPE_DEFORM,
			TYPE_MAX_NUM
		};

		typedef struct STriGroupNode
		{
			STriGroupNode *		pNextTriGroupNode;
			int					idxPos;
			int					triCount;
			DWORD				mtrlIndex;						
		} TTriGroupNode;

	public:
		CGrannyMesh();
		virtual ~CGrannyMesh();

		bool					IsEmpty() const;
		bool					CreateFromGrannyMeshPointer(granny_skeleton* pgrnSkeleton, granny_mesh* pgrnMesh, int vtxBasePos, int idxBasePos, CGrannyMaterialPalette& rkMtrlPal);			
		void					LoadIndices(void* dstBaseIndices);
		void					LoadVertices(void* dstBaseVertices);
		void					LoadSkinnedVertices(void* dstBaseVertices, bool pnt2);
		bool					IsPNT2() const;
		void					Destroy();

		void					SetPNT2Mesh();
		bool					HasSkinnedVertices() const;

		bool					IsTwoSide() const;

		int						GetVertexCount() const;
		
		// WORK
		int *					GetDefaultBoneIndices() const;
		// END_OF_WORK

		int						GetVertexBasePosition() const; 
		int						GetIndexBasePosition() const;

		const granny_mesh *					GetGrannyMeshPointer() const;
		const CGrannyMesh::TTriGroupNode *	GetTriGroupNodeList(CGrannyMaterial::EType eMtrlType) const;

		void					RebuildTriGroupNodeList();
		void					ReloadMaterials();

	protected:
		void					Initialize();

		bool					LoadMaterials(CGrannyMaterialPalette& rkMtrlPal);
		bool					LoadTriGroupNodeList(CGrannyMaterialPalette& rkMtrlPal);

	protected:
		// Granny Mesh Data
		granny_data_type_definition *	m_pgrnMeshType;
		granny_mesh *			m_pgrnMesh;
		
		// WORK
		granny_mesh_binding *	m_pgrnMeshBindingTemp;
		// END_OF_WORK


		// Granny Material Data
		std::vector<DWORD>		m_mtrlIndexVector;
		
		// TriGroups Data
		TTriGroupNode *			m_triGroupNodes;
		TTriGroupNode *			m_triGroupNodeLists[CGrannyMaterial::TYPE_MAX_NUM];

		int						m_vtxBasePos;
		int						m_idxBasePos;

		bool					m_hasSkinnedVertex;
		bool					m_isTwoSide;
	private:
		bool						m_bHaveBlendThing;
	public:
		bool						HaveBlendThing() { return m_bHaveBlendThing; }
};