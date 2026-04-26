#include "StdAfx.h"
#include "Eterbase/Debug.h"
#include "ModelInstance.h"
#include "Model.h"
#include "qMin32Lib/VertexBuffer.h"

void CGrannyModelInstance::Update(DWORD dwAniFPS)
{		
	if (!dwAniFPS)
		return;

	const DWORD c_dwCurUpdateFrame = (DWORD) (GetLocalTime() * static_cast<float>(ANIFPS_MAX));
	const DWORD ANIFPS_STEP = ANIFPS_MAX/dwAniFPS;
	if (c_dwCurUpdateFrame>ANIFPS_STEP && c_dwCurUpdateFrame/ANIFPS_STEP==m_dwOldUpdateFrame/ANIFPS_STEP)
		return;

	m_dwOldUpdateFrame=c_dwCurUpdateFrame;

	GrannyFreeCompletedModelControls(m_pgrnModelInstance); //Black screen fix

	//DWORD t1=timeGetTime();
	GrannySetModelClock(m_pgrnModelInstance, GetLocalTime());	
	//DWORD t2=timeGetTime();

#ifdef __PERFORMANCE_CHECKER__
	{
		static FILE* fp=fopen("perf_grn_setmodelclock.txt", "w");

		if (t2-t1>3)
		{
			fprintf(fp, "%f:%x:- GrannySetModelClock(time=%f) = %dms\n", timeGetTime()/1000.0f, this, GetLocalTime(), t2-t1);
			fflush(fp);
		}			
	}
#endif	

}

void CGrannyModelInstance::UpdateLocalTime(float fElapsedTime)
{
	m_fSecondsElapsed = fElapsedTime;
	m_fLocalTime += fElapsedTime;
}

void CGrannyModelInstance::UpdateTransform(D3DXMATRIX * pMatrix, float fSecondsElapsed)
{
	if (!m_pgrnModelInstance)
	{
		TraceError("CGrannyModelIstance::UpdateTransform - m_pgrnModelInstance = NULL");
		return;
	}
	GrannyUpdateModelMatrix(m_pgrnModelInstance, fSecondsElapsed, (const float *) pMatrix, (float *) pMatrix, false);
	//Tracef("%f %f %f",pMatrix->_41,pMatrix->_42,pMatrix->_43);
	
}

void CGrannyModelInstance::Deform(const D3DXMATRIX* c_pWorldMatrix)
{
	if (IsEmpty() || !m_pModel)
		return;

	UpdateWorldPose();
	UpdateWorldMatrices(c_pWorldMatrix);
}

//////////////////////////////////////////////////////
class CGrannyLocalPose
{
	public:
		CGrannyLocalPose()
		{
			m_pgrnLocalPose = NULL;
			m_boneCount = 0;
		}

		virtual ~CGrannyLocalPose()
		{
			if (m_pgrnLocalPose)
				GrannyFreeLocalPose(m_pgrnLocalPose);
		}

		granny_local_pose * Get(int boneCount)
		{
			if (m_pgrnLocalPose)
			{
				if (m_boneCount >= boneCount)
					return m_pgrnLocalPose;

				GrannyFreeLocalPose(m_pgrnLocalPose);
			}

			m_boneCount = boneCount;
			m_pgrnLocalPose = GrannyNewLocalPose(m_boneCount);
			return m_pgrnLocalPose;
		}

	private:
		granny_local_pose *	m_pgrnLocalPose;
		int					m_boneCount;
};
//////////////////////////////////////////////////////

void CGrannyModelInstance::UpdateSkeleton(const D3DXMATRIX * c_pWorldMatrix, float /*fLocalTime*/)
{	
	UpdateWorldPose();
	UpdateWorldMatrices(c_pWorldMatrix);
}

void CGrannyModelInstance::UpdateWorldPose()
{
	if (m_ppkSkeletonInst)
		if (*m_ppkSkeletonInst!=this)
			return;
	
	static CGrannyLocalPose s_SharedLocalPose;

	granny_skeleton * pgrnSkeleton = GrannyGetSourceSkeleton(m_pgrnModelInstance);
	granny_local_pose * pgrnLocalPose = s_SharedLocalPose.Get(pgrnSkeleton->BoneCount);	

	const float * pAttachBoneMatrix = (mc_pParentInstance) ? mc_pParentInstance->GetBoneMatrixPointer(m_iParentBoneIndex) : NULL;

	GrannySampleModelAnimationsAccelerated(m_pgrnModelInstance, pgrnSkeleton->BoneCount, pAttachBoneMatrix, pgrnLocalPose, __GetWorldPosePtr());
	GrannyFreeCompletedModelControls(m_pgrnModelInstance);	

}

void CGrannyModelInstance::UpdateWorldMatrices(const D3DXMATRIX* c_pWorldMatrix)
{
	if (!m_meshMatrices)
		return;

	assert(m_pModel != NULL);
	assert(ms_lpd3dMatStack != NULL);

	int meshCount = m_pModel->GetMeshCount();

	granny_matrix_4x4* pgrnMatCompositeBuffer = GrannyGetWorldPoseComposite4x4Array(__GetWorldPosePtr());
	D3DXMATRIX* boneMatrices = (D3DXMATRIX*)pgrnMatCompositeBuffer;

	for (int i = 0; i < meshCount; ++i)
	{
		D3DXMATRIX& rWorldMatrix = m_meshMatrices[i];

		const CGrannyMesh* pMesh = m_pModel->GetMeshPointer(i);

		int* boneIndices = __GetMeshBoneIndices(i);

		if (pMesh->HasSkinnedVertices())
		{
			rWorldMatrix = *c_pWorldMatrix;
		}
		else
		{
			int iBone = *boneIndices;
			D3DXMatrixMultiply(&rWorldMatrix, &boneMatrices[iBone], c_pWorldMatrix);
		}
	}

#ifdef _TEST
	TEST_matWorld = *c_pWorldMatrix;
#endif
}
