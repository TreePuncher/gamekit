
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

#ifndef ANIMATION_H
#define ANIMATION_H

#include "..\buildsettings.h"
#include "Common.h"
#include "MetaData.h"

/************************************************************************************************/

using FlexKit::Joint;
using FlexKit::JointPose;
using FlexKit::JointHandle;

using FlexKit::GUID_t;
using FlexKit::MetaDataList;

using namespace FlexKit;

struct	JointAnimation
{
	struct Pose
	{
		JointPose	JPose;
		double		T;
		double		_PAD;
	};// *Poses;

	std::vector<Pose> Poses;

	size_t FPS;
	size_t FrameCount;
	size_t _PAD;
};


struct JointInfo
{
	Joint			Joint;
	JointAnimation	Animation;
	XMMATRIX		Inverse;
};

struct AnimationCut
{
	double		T_Start;
	double		T_End;
	std::string	ID;
	GUID_t		guid;
};


struct SkeletonResource;

using JointList				= std::vector<JointInfo>;
using CutList				= std::vector<AnimationCut>;
using SkeletonResource_ptr	= std::shared_ptr<SkeletonResource>;


struct AnimationKeyFrame
{
	size_t			FrameNumber = 0;
	size_t			JointCount = 0;

	void AddJointPose(JointHandle joint, JointPose pose)
	{
		joints.push_back(joint);
		poses.push_back(pose);
	}

	bool hasJoint(JointHandle joint)
	{
		return joints.end() != 
			std::find_if(
				joints.begin(),
				joints.end(),
				[&](auto ajoint)
				{
					return ajoint == joint;
				});
	}

	std::vector<JointHandle>	joints;
	std::vector<JointPose>		poses;
};


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
	std::string						mID				= nullptr;
	bool							isLooping		= false;
};


struct SkeletonResource : public iResource
{
	ResourceBlob CreateBlob() override
	{
		FK_ASSERT(0);

		return {};
	}


	Skeleton CreateSkeletonProxy() // Creates a Skeleton that accesses the Values seen here
	{
		Skeleton skeleton;

		return skeleton;
	}


	JointHandle	FindJoint			(const char* id)
	{
		for (size_t itr = 0; itr < joints.size(); itr++)
		{
			auto& joint = joints[itr];

			if (!strncmp(joint.mID, id, 32))
				return JointHandle(itr);
		}
	}


	void AddAnimationClip(AnimationClipResource clip)
	{
		animations.push_back(clip);
	}


	void AddJoint(Joint joint, XMMATRIX IPose)
	{
		IPoses.push_back(IPose);
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

	std::vector<DirectX::XMMATRIX>		IPoses; // Global Inverse Space Pose
	std::vector<Joint>					joints;
	std::vector<std::string>			jointIDs;
	std::vector<JointPose>				jointPoses;
	std::vector<AnimationClipResource>	animations;
};




/************************************************************************************************/


fbxsdk::FbxNode*	FindSkeletonRoot	( fbxsdk::FbxMesh* M );
void				FindAllJoints		( JointList& Out, FbxNode* N, size_t Parent = 0xFFFF );

void								GetAnimationCuts			( CutList* out, MetaDataList& MD, const std::string& id);
JointAnimation						GetJointAnimation			( FbxNode* N );
JointHandle							GetJoint					( static_vector<JointInfo, 1024>& Out, const char* ID );
void								GetJointTransforms			( JointList& Out, FbxMesh* M, iAllocator* MEM );
FbxAMatrix							GetGeometryTransformation	( FbxNode* inNode );	
std::shared_ptr<Skeleton_MetaData>	GetSkeletonMetaData			(const MetaDataList& metaDatas);

SkeletonResource_ptr		LoadSkeletonResource		( FbxMesh* M, const char* ParentID = nullptr, MetaDataList& related = MetaDataList{});


/************************************************************************************************/

#endif