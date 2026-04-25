#pragma once

#include "qMin32Lib/Core.h"

#include "Mesh.h"

class CGrannyModel : public CReferenceObject
{
	public:
		typedef struct SMeshNode
		{
			int					iMesh;
			const CGrannyMesh * pMesh;
			SMeshNode *			pNextMeshNode;
		} TMeshNode;

	public:
		CGrannyModel();
		virtual ~CGrannyModel();

		bool IsEmpty() const;
		bool CreateFromGrannyModelPointer(granny_model* pgrnModel);
		bool CreateDeviceObjects();
		void DestroyDeviceObjects();
		void Destroy();

		int GetRigidVertexCount() const;
		int GetDeformVertexCount() const;
		int GetVertexCount() const;

		bool CanDeformPNTVertices() const;
		bool HasSkinnedMesh() const;
		void DeformPNTVertices(void* dstBaseVertices, D3DXMATRIX* boneMatrices, const std::vector<granny_mesh_binding*>& c_rvct_pgrnMeshBinding) const;

		int GetIdxCount();
		int GetMeshCount() const;
		CGrannyMesh * GetMeshPointer(int iMesh);
		granny_model * GetGrannyModelPointer();
		const CGrannyMesh* GetMeshPointer(int iMesh) const;

		VBufferPtr GetVertexBuffer() const;
		VBufferPtr GetSkinnedVertexBuffer() const;
		UINT GetSkinnedVertexStride() const;
		IBufferPtr GetIndexBuffer() const;

		const CGrannyModel::TMeshNode*  GetMeshNodeList(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType) const;

		const CGrannyMaterialPalette& GetMaterialPalette() const;

	protected:
		bool LoadMeshs();		
		bool LoadVertices();
		bool LoadSkinnedVertices();
		bool LoadIndices();
		void Initialize();

		BOOL CheckMeshIndex(int iIndex) const;
		void AppendMeshNode(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType, int iMesh);

	protected:
		// Granny Data
		granny_model *			m_pgrnModel;
		bool					m_hasPNT2;
		// Static Data
		CGrannyMesh *			m_meshs;

		VBufferPtr				m_pntVtxBuf;	// for rigid mesh
		VBufferPtr				m_skinnedVtxBuf;
		IBufferPtr				m_idxBuf;

		TMeshNode *				m_meshNodes;
		TMeshNode *				m_meshNodeLists[CGrannyMesh::TYPE_MAX_NUM][CGrannyMaterial::TYPE_MAX_NUM];

		int						m_deformVtxCount;
		int						m_rigidVtxCount;
		int						m_vtxCount;
		int						m_idxCount;

		int						m_meshNodeSize;
		int						m_meshNodeCapacity;

		bool					m_canDeformPNVertices;
		
		CGrannyMaterialPalette	m_kMtrlPal;
	private:
		bool					m_bHaveBlendThing;
	public:
		bool					HaveBlendThing() { return m_bHaveBlendThing; }
	
	protected:
		UINT m_stride;
		UINT m_skinnedStride;
};
