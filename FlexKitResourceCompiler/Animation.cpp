/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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


fbxsdk::FbxNode* FindSkeletonRoot(const fbxsdk::FbxMesh& mesh)
{
	auto DeformerCount  = mesh.GetDeformerCount();
	for (int32_t I = 0; I < DeformerCount; ++I)
	{
		fbxsdk::FbxStatus S;
		const auto D	= mesh.GetDeformer(I, &S);  FK_ASSERT(D != nullptr);
		const auto Type	= D->GetDeformerType();

		switch (Type)
		{
		case fbxsdk::FbxDeformer::EDeformerType::eSkin:
		{
			const auto Skin			= (FbxSkin*)D;
			const auto ClusterCount	= Skin->GetClusterCount();

			const auto Cluster		= Skin->GetCluster(0);
			const auto CLBone		= Cluster->GetLink();
			const auto CLBoneAttrib	= CLBone->GetNodeAttribute();
			const auto CLBoneName	= CLBone->GetName();
			auto* I				    = CLBone;

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


SkeletonJointAnimation GetJointAnimation(const FbxNode* node, const AnimationProperties properties)
{
	const auto Scene			= node->GetScene();
	const auto AnimationStack   = Scene->GetCurrentAnimationStack();
	const auto TakeInfo		    = Scene->GetTakeInfo(AnimationStack->GetName());
	const auto Begin			= TakeInfo->mLocalTimeSpan.GetStart();
	const auto End			    = TakeInfo->mLocalTimeSpan.GetStop();
	const auto Duration		    = (End - Begin).GetFrameCount(FbxTime::ConvertFrameRateToTimeMode(properties.frameRate));
	const auto FrameRate		= properties.frameRate;

    SkeletonJointAnimation A;
	A.FPS			= (uint32_t)properties.frameRate;
	A.FrameCount	= Duration;
	A.Poses.resize(Duration);

	for (size_t I = 0; I < size_t(Duration); ++I)
	{
		FbxTime	CurrentFrame;
		CurrentFrame.SetFrame(I, FbxTime::ConvertFrameRateToTimeMode(properties.frameRate));
		A.Poses[I].JPose = GetPose(FBXMATRIX_2_XMMATRIX(const_cast<FbxNode*>(node)->EvaluateLocalTransform(CurrentFrame)));
	}

	return A;
}


/************************************************************************************************/


JointHandle GetJoint(const JointList& joints, const char* ID)
{
	for (size_t I = 0; I < joints.size(); ++I)
		if (joints[I].Joint.mID == ID)
			return (JointHandle)I;

	return InvalidHandle_t;
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


void GetJointTransforms(JointList& Out, const FbxMesh& mesh)
{
	using DirectX::XMMatrixRotationQuaternion;

    FbxAMatrix Identity;
    Identity.FbxAMatrix::SetIdentity();

	const auto DeformerCount = mesh.GetDeformerCount();
	for (int I = 0; I < DeformerCount; ++I)
	{
		const auto D = mesh.GetDeformer(I);
		if (D->GetDeformerType() == FbxDeformer::EDeformerType::eSkin)
		{
			const auto Skin = (FbxSkin*)D;
            auto root = Skin->GetCluster(0);
            FbxAMatrix G = GetGeometryTransformation(root->GetLink());
            FbxAMatrix transformMatrix = root->GetTransformMatrix(FbxAMatrix{});
            FbxAMatrix rootLinkMatrix  = root->GetTransformLinkMatrix(FbxAMatrix{});



			for (int II = 0; Skin->GetClusterCount() > II; ++II)
			{
				const auto Cluster  = Skin->GetCluster(II);
				const auto ID       = Cluster->GetLink()->GetName();

				JointHandle Handle              = GetJoint(Out, ID);
				FbxAMatrix G                    = GetGeometryTransformation(Cluster->GetLink());
				FbxAMatrix transformMatrix      = Cluster->GetTransformMatrix(FbxAMatrix{});
				FbxAMatrix transformLinkMatrix  = Cluster->GetTransformLinkMatrix(FbxAMatrix{});

                const FbxAMatrix globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * G;
				const XMMATRIX Inverse  = FBXMATRIX_2_XMMATRIX(globalBindposeInverseMatrix);
                auto temp = GetPose(Inverse);

				Out[Handle].Inverse = Inverse;
			}
		}
	}
}


/************************************************************************************************/


void FindAllJoints(JointList& Out, const FbxNode* N, const size_t Parent )
{
	if (N->GetNodeAttribute() && N->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton )
	{
		fbxsdk::FbxSkeleton* Sk = (fbxsdk::FbxSkeleton*)N->GetNodeAttribute();
		
		const int JointIndex = (int)Out.size();
		const int ChildCount = N->GetChildCount();

		SkeletonJoint NewJoint;
		NewJoint.mID	    = N->GetName();
		NewJoint.mParent	= JointHandle(Parent);

		Out.emplace_back(SkeletonJointInfo{ { NewJoint }, GetJointAnimation(N), DirectX::XMMatrixIdentity() });

		for ( int I = 0; I < ChildCount; ++I )
			FindAllJoints(Out, N->GetChild( I ), JointIndex);
	}
}


/************************************************************************************************/


MetaDataList GetAllAnimationClipMetaData(const MetaDataList& metaDatas)
{
	return FilterList(
			[](auto metaData) 
			{ 
				return (metaData->type == MetaData::EMETAINFOTYPE::EMI_SKELETALANIMATION); 
			}, 
			metaDatas);
}


/************************************************************************************************/


CutList GetAnimationCuts(const MetaDataList& metaData, const std::string& ID)
{
	auto related	= FindRelatedMetaData(metaData, MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION, ID);

    CutList out     = transform_stl(
        GetAllAnimationClipMetaData(related),
        [](auto& ClipMetaData) -> AnimationCut
        {
            auto clip = std::static_pointer_cast<SkeletalAnimationClip_MetaData> (ClipMetaData);
            return {
                clip->T_Start,
                clip->T_End,
                clip->ClipID,
                clip->guid,
                { clip } };
        });

    return out;
}


/************************************************************************************************/


std::shared_ptr<Skeleton_MetaData> GetSkeletonMetaData(const MetaDataList& metaDatas)
{
	for (auto metaData : metaDatas)
		if (metaData->type == MetaData::EMETAINFOTYPE::EMI_SKELETAL)
			return std::static_pointer_cast<Skeleton_MetaData>(metaData);

	return nullptr;
}


/************************************************************************************************/


SkeletonResource_ptr CreateSkeletonResource(FbxMesh& mesh, const std::string& parentID, const MetaDataList& MD)
{
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;

    auto skeleton = std::make_unique<SkeletonResource>();

	// Gather MetaData
	auto Related		= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON, parentID);
	auto SkeletonInfo	= GetSkeletonMetaData(MD);
    skeleton->metaData  = Related;

	auto root	= FindSkeletonRoot(mesh);
	if (!root || !SkeletonInfo)
		return nullptr;

	std::vector<SkeletonJointInfo> joints;
	FindAllJoints		(joints, root);
	GetJointTransforms	(joints, mesh);

	for (auto j : joints)
		skeleton->AddJoint(j.Joint, j.Inverse);
	
	const std::string ID = SkeletonInfo->SkeletonID;
	skeleton->guid  = SkeletonInfo->SkeletonGUID;
    skeleton->ID    = ID;

	for (size_t joint = 0; joint < joints.size(); ++joint)
		skeleton->joints[joint].mID = skeleton->joints[joint].mID;

	const CutList cuts = GetAnimationCuts(MD, ID);
	for(const auto& cut : cuts)
	{
		const auto begin	= (size_t)(cut.T_Start);
		const auto end		= (size_t)(cut.T_End);

		AnimationClipResource clip;
		clip.FPS		= 60;
		clip.mID		= cut.ID;
		clip.guid		= cut.guid;
		clip.isLooping	= false;

		const size_t clipFrameCount = end - begin;

		for (size_t frame = 0; frame < clipFrameCount; ++frame)
		{
			AnimationKeyFrame keyFrame;
			keyFrame.FrameNumber = frame;

			for (size_t jointIdx = 0; jointIdx < joints.size(); ++jointIdx)
			{
                const auto& joint       = joints[jointIdx];
				const auto  inverse		= DirectX::XMMatrixInverse(nullptr, Float4x4ToXMMATIRX(GetPoseTransform(skeleton->jointPoses[jointIdx])));
				const auto  pose		= Float4x4ToXMMATIRX(GetPoseTransform(joints[jointIdx].Animation.Poses[(frame + begin) % (joint.Animation.FrameCount - 1)].JPose));
				const auto  localPose   = GetPose(pose * inverse);

				keyFrame.AddJointPose(JointHandle(jointIdx), localPose);
			}

			clip.AddKeyFrame(keyFrame, frame * 1.0 / 60.0f);
		}

        //clip.Compress();
	}

	return skeleton;
}


/************************************************************************************************/
