/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/

#include "stdafx.h"
#include "Animation.h"



/************************************************************************************************/


fbxsdk::FbxNode* FindSkeletonRoot(fbxsdk::FbxMesh* M)
{
	auto DeformerCount  = M->GetDeformerCount();
	for (size_t I = 0; I < DeformerCount; ++I)
	{
		fbxsdk::FbxStatus S;
		auto D		= M->GetDeformer(I, &S);
		auto Type	= D->GetDeformerType();

		switch (Type)
		{
		case fbxsdk::FbxDeformer::EDeformerType::eSkin:
		{
			auto Skin			= (FbxSkin*)D;
			auto ClusterCount	= Skin->GetClusterCount();

			auto Cluster		= Skin->GetCluster(0);
			auto CLBone			= Cluster->GetLink();
			auto CLBoneAttrib	= CLBone->GetNodeAttribute();
			auto CLBoneName		= CLBone->GetName();
			auto* I				= CLBone;

			while (true)
			{
				if (I->GetParent()->GetSkeleton())
					I = I->GetParent();
				else
					return I;
			}

		}	break;
		default:
			break;
		}
	}

	return nullptr;
}


/************************************************************************************************/


JointAnimation GetJointAnimation(FbxNode* N, iAllocator* M)
{
	auto Scene = N->GetScene();
	auto AnimationStack = Scene->GetCurrentAnimationStack();
	auto TakeInfo = Scene->GetTakeInfo(AnimationStack->GetName());
	auto Begin = TakeInfo->mLocalTimeSpan.GetStart();
	auto End = TakeInfo->mLocalTimeSpan.GetStop();
	auto Duration = End - Begin;
	auto FrameRate = 1.0f / 60.0f;

	JointAnimation	A;
	A.FPS = 60;
	A.Poses = (JointAnimation::Pose*)M->_aligned_malloc(sizeof(JointAnimation::Pose) * Duration.GetFrameCount(FbxTime::eFrames60));
	A.FrameCount = Duration.GetFrameCount(FbxTime::eFrames60);

	for (size_t I = 0; I < size_t(Duration.GetFrameCount(FbxTime::eFrames60)); ++I)
	{
		FbxTime	CurrentFrame;
		CurrentFrame.SetFrame(I, FbxTime::eFrames60);
		A.Poses[I].JPose = GetPose(FBXMATRIX_2_XMMATRIX(N->EvaluateLocalTransform(CurrentFrame)));
	}

	return A;
}


/************************************************************************************************/


JointHandle GetJoint(static_vector<JointInfo, 1024>& Out, const char* ID)
{
	for (size_t I = 0; I < Out.size(); ++I)
		if (!strcmp(Out[I].Joint.mID, ID))
			return I;

	return 0XFFFF;
}


/************************************************************************************************/


FbxAMatrix GetGeometryTransformation(FbxNode* inNode)
{
	const FbxVector4 lT = inNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = inNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = inNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}


/************************************************************************************************/


void GetJointTransforms(JointList& Out, FbxMesh* M, iAllocator* MEM)
{
	using DirectX::XMMatrixRotationQuaternion;

	auto DeformerCount = M->GetDeformerCount();
	for (size_t I = 0; I < DeformerCount; ++I)
	{
		auto D = M->GetDeformer(I);
		if (D->GetDeformerType() == FbxDeformer::EDeformerType::eSkin)
		{
			auto Skin = (FbxSkin*)D;

			for (size_t II = 0; Skin->GetClusterCount() > II; ++II)
			{
				auto Cluster = Skin->GetCluster(II);
				auto ID = Cluster->GetLink()->GetName();

				JointHandle Handle = GetJoint(Out, ID);
				FbxAMatrix G = GetGeometryTransformation(Cluster->GetLink());
				FbxAMatrix transformMatrix;
				FbxAMatrix transformLinkMatrix;

				Cluster->GetTransformMatrix(transformMatrix);
				Cluster->GetTransformLinkMatrix(transformLinkMatrix);

				FbxAMatrix globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * G;
				XMMATRIX Inverse = FBXMATRIX_2_XMMATRIX(globalBindposeInverseMatrix);
				Out[Handle].Inverse = Inverse;
			}
		}
	}
}


/************************************************************************************************/


void FindAllJoints(JointList& Out, FbxNode* N, iAllocator* MEM, size_t Parent )
{
	if (N->GetNodeAttribute() && N->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton )
	{
		fbxsdk::FbxSkeleton* Sk = (fbxsdk::FbxSkeleton*)N->GetNodeAttribute();
		
		int JointIndex = Out.size();
		int ChildCount = N->GetChildCount();

		Joint NewJoint;
		NewJoint.mID		= N->GetName();
		NewJoint.mParent	= JointHandle(Parent);
		JointAnimation& Animations = GetJointAnimation(N, MEM);

		Out.push_back({ {NewJoint}, Animations, DirectX::XMMatrixIdentity() });

		for ( size_t I = 0; I < ChildCount; ++I )
			FindAllJoints(Out, N->GetChild( I ), MEM, JointIndex);
	}
}


/************************************************************************************************/


RelatedMetaData GetAllAnimationClipMetaData(MD_Vector* MD, RelatedMetaData* RD, iAllocator* Memory)
{
	RelatedMetaData Out(Memory);

	for (auto I : *RD)
		if (MD->at(I)->type == MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP)
			Out.push_back(I);

	return Out;
}


/************************************************************************************************/


void GetAnimationCuts(CutList* out, MD_Vector* MD, const char* ID, iAllocator* Mem)
{
	if (MD)
	{
		auto Related = FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION, ID, Mem);
		auto AnimationClips = GetAllAnimationClipMetaData(MD, &Related, Mem);

		for (auto I : AnimationClips)
		{
			AnimationClip_MetaData* Clip = (AnimationClip_MetaData*)MD->at(I);
			AnimationCut	NewCut = {};
			NewCut.ID = Clip->ClipID;
			NewCut.T_Start = Clip->T_Start;
			NewCut.T_End = Clip->T_End;
			NewCut.guid = Clip->guid;
			out->push_back(NewCut);
		}
	}
}


/************************************************************************************************/


Skeleton_MetaData* GetSkeletonMetaData(MD_Vector* MD, RelatedMetaData* RD)
{
	for (auto I : *RD)
		if (MD->at(I)->type == MetaData::EMETAINFOTYPE::EMI_SKELETAL)
			return (Skeleton_MetaData*)MD->at(I);

	return nullptr;
}


/************************************************************************************************/


FlexKit::Skeleton* LoadSkeleton(FbxMesh* M, iAllocator* Mem, iAllocator* Temp, const char* ParentID, MD_Vector* MD)
{
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;

	// Gather MetaData
	auto Related		= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON, ParentID, Temp);
	auto SkeletonInfo	= GetSkeletonMetaData(MD, &Related);

	auto Root	= FindSkeletonRoot(M);
	if (!Root || !SkeletonInfo)
		return nullptr;

	auto& Joints = Mem->allocate_aligned<static_vector<JointInfo, 1024>>();
	FindAllJoints		(Joints, Root, Temp);
	GetJointTransforms	(Joints, M, Temp);

	Skeleton* S = (Skeleton*)Mem->_aligned_malloc(0x40);
	S->InitiateSkeleton(Mem, Joints.size());

	for (auto J : Joints)
		S->AddJoint(J.Joint, J.Inverse);
	
	char* ID = SkeletonInfo->SkeletonID;
	S->guid  = SkeletonInfo->SkeletonGUID;

	for (size_t I = 0; I < Joints.size(); ++I)
	{
		size_t ID_Length = strnlen_s(S->Joints[I].mID, 64) + 1;
		char* ID		 = (char*)Mem->malloc(ID_Length);

		strcpy_s(ID, ID_Length, S->Joints[I].mID);
		S->Joints[I].mID = ID;
	}

	CutList Cuts(Mem);
	GetAnimationCuts(&Cuts, MD, ID, Mem);

	for(auto Cut : Cuts)
	{
		size_t Begin	= Cut.T_Start / (1.0f / 60.0f);
		size_t End		= Cut.T_End / (1.0f / 60.0f);

		AnimationClip Clip;
		Clip.Skeleton	= S;
		Clip.FPS		= 60;
		Clip.FrameCount	= End - Begin;
		Clip.mID		= Cut.ID;
		Clip.guid		= Cut.guid;
		Clip.isLooping	= false;
		Clip.Frames		= (AnimationClip::KeyFrame*)Mem->_aligned_malloc(Clip.FrameCount * sizeof(AnimationClip::KeyFrame));

		for (size_t I = 0; I < Clip.FrameCount; ++I)
		{
			Clip.Frames[I].Joints		= (JointHandle*)Mem->_aligned_malloc(sizeof(JointHandle) * Joints.size());
			Clip.Frames[I].Poses		=   (JointPose*)Mem->_aligned_malloc(sizeof(JointPose)	 * Joints.size());
			Clip.Frames[I].JointCount	= Joints.size();

			for (size_t II = 0; II < Joints.size(); ++II)
			{
				Clip.Frames[I].Joints[II]	= JointHandle(II);
				
				auto Inverse				= DirectX::XMMatrixInverse(nullptr, GetPoseTransform(S->JointPoses[II]));
				auto Pose					= GetPoseTransform(Joints[II].Animation.Poses[I + Begin].JPose);
				auto LocalPose				= GetPose(Pose * Inverse);
				Clip.Frames[I].Poses[II]	= LocalPose;
			}
		}

		Skeleton_PushAnimation(S, Mem, Clip);
	}
	Temp->clear();

	return S;
}


/************************************************************************************************/
