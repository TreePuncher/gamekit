#ifndef ANIMATION_H
#define ANIMATION_H

#include "..\buildsettings.h"
#include "Common.h"
#include "MetaData.h"
#include "ResourceUtilities.h"
#include "..\coreutilities\XMMathConversion.h"


/************************************************************************************************/


using FlexKit::Joint;
using FlexKit::JointPose;
using FlexKit::JointHandle;

using FlexKit::GUID_t;
using FlexKit::MetaDataList;

using namespace FlexKit;

struct SkeletonJointAnimation
{
	struct Pose
	{
		JointPose	JPose;
		double		T;
		double		_PAD;
	};

	std::vector<Pose> Poses;

	size_t FPS;
	size_t FrameCount;
	size_t _PAD;
};


/************************************************************************************************/


struct SkeletonJoint
{
    std::string         mID;
    JointHandle			mParent = JointHandle(0);
    char				mPad[6] = {};
};


/************************************************************************************************/


struct SkeletonJointInfo
{
    SkeletonJoint           Joint;
	SkeletonJointAnimation	Animation;
	XMMATRIX		        Inverse;
};


/************************************************************************************************/


struct AnimationCut
{
	double		    T_Start;
	double		    T_End;
	std::string	    ID;
	GUID_t		    guid;
    MetaDataList    metaData;
};


/************************************************************************************************/


struct SkeletonResource;

using JointList				= std::vector<SkeletonJointInfo>;
using CutList				= std::vector<AnimationCut>;
using SkeletonResource_ptr	= std::shared_ptr<SkeletonResource>;


/************************************************************************************************/


struct AnimationKeyFrame
{
	size_t FrameNumber  = 0;
	size_t JointCount   = 0;

	void AddJointPose(const JointHandle joint, const JointPose pose)
	{
		joints.push_back(joint);
		poses.push_back(pose);
	}

	bool hasJoint(const JointHandle joint) const noexcept
	{
		return joints.end() != 
			std::find_if(
				joints.begin(),
				joints.end(),
				[&](const auto& ajoint)
				{
					return ajoint == joint;
				});
	}

	std::vector<JointHandle>	joints;
	std::vector<JointPose>		poses;
};


/************************************************************************************************/


struct AnimationClipResource : public iResource
{
	ResourceBlob CreateBlob()
	{
		FK_ASSERT(0);

		return {};
	}
	

	void AddKeyFrame(AnimationKeyFrame keyFrame)
	{
		Frames.push_back(keyFrame);
	}


	std::vector<AnimationKeyFrame>	Frames;
	uint32_t						FPS				= 0;
	size_t							guid			= 0;
	size_t							skeletonGuid	= 0;
	std::string						mID				= "";
	bool							isLooping		= false;
};


/************************************************************************************************/


struct SkeletonResource : public iResource
{
	ResourceBlob CreateBlob() override
	{
        Blob jointBlob;
        for (size_t jointIdx = 0; jointIdx < joints.size(); ++jointIdx)
        {
            SkeletonResourceBlob::JointEntry jointOut;
            auto& jointLinkage = joints[jointIdx];

            
            jointOut.IPose     = FlexKit::XMMatrixToFloat4x4(&IPoses[jointIdx]);
            jointOut.Parent    = jointLinkage.mParent;
            jointOut.Pose      = jointPoses[jointIdx];

            strncpy_s(jointOut.ID, jointLinkage.mID.c_str(), jointLinkage.mID.size());

            jointBlob += jointOut;
        }

        SkeletonResourceBlob::Header header;

        header.GUID         = guid;
        strncpy_s(header.ID, ID.c_str(), ID.size());
        header.JointCount   = joints.size();
        header.ResourceSize = sizeof(header) + jointBlob.size();
        header.Type         = EResourceType::EResource_Skeleton;

        Blob resource = Blob{ header } + jointBlob;
        ResourceBlob out;

        out.buffer          = (char*)malloc(resource.size());
        out.bufferSize      = resource.size();
        out.resourceType    = EResourceType::EResource_SkeletalAnimation;
        out.GUID            = guid;
        out.ID              = ID;

        memcpy(out.buffer, resource.data(), resource.size());

        return out;
	}


	JointHandle	FindJoint(const std::string& id) const noexcept
	{
		for (size_t itr = 0; itr < joints.size(); itr++)
		{
			auto& joint = joints[itr];

            if (joint.mID == id)
				return JointHandle(itr);
		}

        return InvalidHandle_t;
	}


	void AddAnimationClip(AnimationClipResource clip)
	{
		animations.push_back(clip);
	}


	void AddJoint(const SkeletonJoint joint, const XMMATRIX IPose)
	{
		IPoses.push_back(IPose);
        jointPoses.push_back(GetPose(IPose));
		joints.push_back(joint);
		jointIDs.push_back("");

		JointCount++;
	}


	void SetJointID(JointHandle joint, std::string& ID)
	{
		jointIDs[joint] = ID;
	}

	size_t								JointCount;
	GUID_t								guid;
    std::string                         ID;

	std::vector<DirectX::XMMATRIX>		IPoses; // Global Inverse Space Pose
	std::vector<SkeletonJoint>			joints;
	std::vector<std::string>			jointIDs;
	std::vector<JointPose>				jointPoses;
	std::vector<AnimationClipResource>	animations;
    MetaDataList                        metaData;
};


/************************************************************************************************/


struct AnimationProperties
{
    double frameRate = 60;
};


/************************************************************************************************/


fbxsdk::FbxNode*	 FindSkeletonRoot	(const fbxsdk::FbxMesh& M);
void				 FindAllJoints		(JointList& Out, const FbxNode* N, const size_t Parent = 0xFFFF);

CutList				    GetAnimationCuts			(const MetaDataList& MD,  const std::string& id);
SkeletonJointAnimation	GetJointAnimation			(const FbxNode* N,        const AnimationProperties properties = AnimationProperties{});
JointHandle             GetJoint					(const JointList& joints, const char* ID);
void				    GetJointTransforms			(JointList& joints, const FbxMesh& M);
FbxAMatrix			    GetGeometryTransformation	(const FbxNode* inNode);	
SkeletonMetaData_ptr    GetSkeletonMetaData		    (const MetaDataList& metaDatas);
SkeletonResource_ptr    CreateSkeletonResource		(FbxMesh& M, const std::string& ParentID = "", const MetaDataList& related = MetaDataList{});


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


#endif
