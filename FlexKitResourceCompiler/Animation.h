
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
#include "MeshProcessing.h"


/************************************************************************************************/

using FlexKit::Joint;
using FlexKit::JointPose;
using FlexKit::JointHandle;

using FlexKit::GUID_t;
using FlexKit::MD_Vector;

using namespace FlexKit;

struct	JointAnimation
{
	struct Pose
	{
		JointPose	JPose;
		double		T;
		double		_PAD;
	}* Poses;

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
	double	T_Start;
	double	T_End;
	char*	ID;
	GUID_t	guid;
};


typedef static_vector<JointInfo, 1024> JointList;
typedef Vector<AnimationCut> CutList;


/************************************************************************************************/


fbxsdk::FbxNode*	FindSkeletonRoot	( fbxsdk::FbxMesh* M );
void				FindAllJoints		( JointList& Out, FbxNode* N, iAllocator* MEM, size_t Parent = 0xFFFF );

void				GetAnimationCuts			( CutList* out, MD_Vector* MD, const char* ID, iAllocator* Mem );
JointAnimation		GetJointAnimation			( FbxNode* N, iAllocator* M );
JointHandle			GetJoint					( static_vector<JointInfo, 1024>& Out, const char* ID );
void				GetJointTransforms			( JointList& Out, FbxMesh* M, iAllocator* MEM );
FbxAMatrix			GetGeometryTransformation	( FbxNode* inNode );	
Skeleton_MetaData*	GetSkeletonMetaData			( MD_Vector* MD, RelatedMetaData* RD );

FlexKit::Skeleton*	LoadSkeleton				( FbxMesh* M, iAllocator* Mem, iAllocator* Temp, const char* ParentID = nullptr, MD_Vector* MD = nullptr );


/************************************************************************************************/

#endif