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


JointAnimation GetJointAnimation(FbxNode* N)
{
	auto Scene			= N->GetScene();
	auto AnimationStack = Scene->GetCurrentAnimationStack();
	auto TakeInfo		= Scene->GetTakeInfo(AnimationStack->GetName());
	auto Begin			= TakeInfo->mLocalTimeSpan.GetStart();
	auto End			= TakeInfo->mLocalTimeSpan.GetStop();
	auto Duration		= End - Begin;
	auto FrameRate		= 1.0f / 60.0f;

	JointAnimation	A;
	A.FPS			= 60;
	A.FrameCount	= Duration.GetFrameCount(FbxTime::eFrames60);
	A.Poses.reserve(Duration.GetFrameCount(FbxTime::eFrames60));

	for (size_t I = 0; I < size_t(Duration.GetFrameCount(FbxTime::eFrames60)); ++I)
	{
		FbxTime	CurrentFrame;
		CurrentFrame.SetFrame(I, FbxTime::eFrames60);
		A.Poses[I].JPose = GetPose(FBXMATRIX_2_XMMATRIX(N->EvaluateLocalTransform(CurrentFrame)));
	}

	return A;
}


/************************************************************************************************/


JointHandle GetJoint(JointList& Out, const char* ID)
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


void GetJointTransforms(JointList& Out, FbxMesh* M)
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


void FindAllJoints(JointList& Out, FbxNode* N, size_t Parent )
{
	if (N->GetNodeAttribute() && N->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton )
	{
		fbxsdk::FbxSkeleton* Sk = (fbxsdk::FbxSkeleton*)N->GetNodeAttribute();
		
		int JointIndex = Out.size();
		int ChildCount = N->GetChildCount();

		Joint NewJoint;
		NewJoint.mID				= N->GetName();
		NewJoint.mParent			= JointHandle(Parent);

		Out.emplace_back(JointInfo{ {NewJoint}, GetJointAnimation(N), DirectX::XMMatrixIdentity() });

		for ( size_t I = 0; I < ChildCount; ++I )
			FindAllJoints(Out, N->GetChild( I ), JointIndex);
	}
}


/************************************************************************************************/


MetaDataList GetAllAnimationClipMetaData(const MetaDataList& metaDatas)
{
	return FilterList(
			[](auto* metaData) 
			{ 
				return (metaData->type == MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP); 
			}, 
			metaDatas);
}


/************************************************************************************************/


void GetAnimationCuts(CutList* out, const MetaDataList& metaData, const std::string& ID)
{
	auto Related		= FindRelatedMetaData(metaData, MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION, ID);
	auto AnimationClips = GetAllAnimationClipMetaData(metaData);

	for (auto clip : AnimationClips)
	{
		AnimationClip_MetaData* ClipMetaData = (AnimationClip_MetaData*)clip;
		AnimationCut NewCut = {};
		NewCut.ID			= ClipMetaData->ClipID;
		NewCut.T_Start		= ClipMetaData->T_Start;
		NewCut.T_End		= ClipMetaData->T_End;
		NewCut.guid			= ClipMetaData->guid;

		out->push_back(NewCut);
	}
}


/************************************************************************************************/


Skeleton_MetaData* GetSkeletonMetaData(const MetaDataList& metaDatas)
{
	for (auto metaData : metaDatas)
		if (metaData->type == MetaData::EMETAINFOTYPE::EMI_SKELETAL)
			return static_cast<Skeleton_MetaData*>(metaData);

	return nullptr;
}


/************************************************************************************************/


FlexKit::Skeleton* LoadSkeleton(FbxMesh* M, const std::string& parentID, const MetaDataList& MD)
{
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;

	// Gather MetaData
	auto Related		= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON, parentID);
	auto SkeletonInfo	= GetSkeletonMetaData(MD);

	auto Root	= FindSkeletonRoot(M);
	if (!Root || !SkeletonInfo)
		return nullptr;

	std::vector<JointInfo> Joints;
	FindAllJoints		(Joints, Root);
	GetJointTransforms	(Joints, M);

	Skeleton& skeleton = *(new Skeleton);
	skeleton.InitiateSkeleton(SystemAllocator, Joints.size());

	for (auto J : Joints)
		skeleton.AddJoint(J.Joint, J.Inverse);
	
	const std::string ID = SkeletonInfo->SkeletonID;
	skeleton.guid  = SkeletonInfo->SkeletonGUID;

	for (size_t I = 0; I < Joints.size(); ++I)
	{
		size_t ID_Length = strnlen_s(skeleton.Joints[I].mID, 64) + 1;
		char* ID		 = (char*)SystemAllocator.malloc(ID_Length);

		strcpy_s(ID, ID_Length, skeleton.Joints[I].mID);
		skeleton.Joints[I].mID = ID;
	}

	CutList Cuts;
	GetAnimationCuts(&Cuts, MD, ID);

	for(auto Cut : Cuts)
	{
		size_t Begin	= Cut.T_Start / (1.0f / 60.0f);
		size_t End		= Cut.T_End / (1.0f / 60.0f);

		AnimationClip Clip;
		Clip.Skeleton	= &skeleton;
		Clip.FPS		= 60;
		Clip.FrameCount	= End - Begin;
		Clip.mID		= Cut.ID;
		Clip.guid		= Cut.guid;
		Clip.isLooping	= false;
		Clip.Frames		= (AnimationClip::KeyFrame*)SystemAllocator._aligned_malloc(Clip.FrameCount * sizeof(AnimationClip::KeyFrame));

		for (size_t I = 0; I < Clip.FrameCount; ++I)
		{
			Clip.Frames[I].Joints		= (JointHandle*)SystemAllocator._aligned_malloc(sizeof(JointHandle) * Joints.size());
			Clip.Frames[I].Poses		= (JointPose*)SystemAllocator._aligned_malloc(sizeof(JointPose)	 * Joints.size());
			Clip.Frames[I].JointCount	= Joints.size();

			for (size_t II = 0; II < Joints.size(); ++II)
			{
				Clip.Frames[I].Joints[II]	= JointHandle(II);
				
				auto Inverse				= DirectX::XMMatrixInverse(nullptr, GetPoseTransform(skeleton.JointPoses[II]));
				auto Pose					= GetPoseTransform(Joints[II].Animation.Poses[I + Begin].JPose);
				auto LocalPose				= GetPose(Pose * Inverse);
				Clip.Frames[I].Poses[II]	= LocalPose;
			}
		}

		skeleton.AddAnimationClip(SystemAllocator, Clip);
	}

	return &skeleton;
}


/************************************************************************************************/
