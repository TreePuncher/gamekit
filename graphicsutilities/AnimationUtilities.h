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

#ifndef ANIMATIONUTILITIES_H_INCLUDED
#define ANIMATIONUTILITIES_H_INCLUDED

#include "buildsettings.h"
#include "Handle.h"
#include "Assets.h"
#include <DirectXMath.h>


namespace FlexKit
{   /************************************************************************************************/


	using DirectX::XMMATRIX;


    /************************************************************************************************/


	using JointHandle = FlexKit::Handle_t<16, GetTypeGUID(JointHandle)>;

	// PreDeclarations
	struct	Brush;
	struct	Skeleton;

	struct Joint
	{
		const char*			mID				= nullptr; // null terminated string
		JointHandle			mParent			= JointHandle(0);
		char				mPad[6]			={};
	};


	/************************************************************************************************/

	
	struct VShaderJoint
	{
		XMMATRIX I;
		XMMATRIX T;
	};

	struct JointPose
	{
		JointPose() {}
		JointPose(Quaternion Q, float4 TS) : r(Q), ts(TS) {}

		Quaternion	r;
		float4		ts;
	};


	FLEXKITAPI float4x4		GetPoseTransform(JointPose P);
	FLEXKITAPI JointPose	GetPose(DirectX::XMMATRIX M);


    /************************************************************************************************/


    struct SkeletonResourceBlob
    {
        struct JointEntry
        {
            FlexKit::float4x4		IPose;
            FlexKit::JointPose		Pose;
            FlexKit::JointHandle	Parent;
            uint16_t				Pad;
            char					ID[64];
        };

        struct Header
        {
            size_t			ResourceSize;
            EResourceType	Type;
            size_t			GUID;
            size_t			Pad;

            char ID[FlexKit::ID_LENGTH];


            size_t JointCount;
        } header;

        JointEntry Joints[];
    };


	/************************************************************************************************/


	struct AnimationClip
	{
		struct KeyFrame
		{
			JointHandle*	Joints		= nullptr;
			JointPose*		Poses		= nullptr;
			size_t			JointCount	= 0;
		};

		uint32_t		FPS				= 0;
		Skeleton*		Skeleton		= nullptr;
		size_t			FrameCount		= 0;
		KeyFrame*		Frames			= nullptr;
		size_t			guid			= 0;
		char*			mID				= nullptr;
		bool			isLooping		= false;
	};


    /************************************************************************************************/

    struct AnimationInterpolator
    {
        enum class InterpolatorType
        {
            Constant,
            Linear,
            Bezier, // FUTURE
            Hermite // FUTURE
        } Type;
    };


    struct AnimationKeyFrame
    {
        float Begin;
        float End;

        enum class KeyFrameType
        {
            FLOAT,
            FLOAT2,
            FLOAT3,
            FLOAT4,
            QUAT,
        };

        AnimationInterpolator   interpolator;
        float4                  Value;
    };

    enum class TrackType
    {
        Skeletal
    };

    struct AnimationTrackHeader
    {
        char        trackName[64];
        char        target[64];

        TrackType   type;

        uint32_t    frameCount;
        uint32_t    byteSize;
    };


    /************************************************************************************************/


    struct AnimationResourceBlob
	{
        struct AnimationResourceHeader
        {
            size_t			ResourceSize;
            EResourceType	Type;

            GUID_t					GUID;
            Resource::ResourceState	State;
            uint32_t				RefCount;

            char   ID[FlexKit::ID_LENGTH];

            uint32_t trackCount;

            static size_t size() noexcept { return sizeof(AnimationResourceHeader); }
        }   header;

		char	Buffer[];
	};

    	struct PoseState_DESC
	{
		size_t JointCount;
	};

    using AnimationFieldID_t = uint32_t;

    template<typename T>
    concept Animatable = requires () { T::GetAnimationFieldID(); };


    /************************************************************************************************/


	struct PoseState
	{
		JointPose*	Joints		= nullptr;
		float4x4*	CurrentPose	= nullptr;
		Skeleton*	Sk			= nullptr;
		size_t		JointCount	= 0;
		size_t		Dirty		= 0;
		size_t		padding[2];


        struct Pose
        {
            void Clear(size_t jointCount)
            {
                for (size_t I = 0; I < jointCount; ++I)
                    jointPose[I] = JointPose{ Quaternion(0, 0, 0, 1), float4(0, 0, 0, 1) };
            }

            JointPose*  jointPose;
            Skeleton*   sk;
            uint32_t    poseID;

            static AnimationFieldID_t GetAnimationFieldID() { return GetTypeGUID(Pose); }
        };

        Pose& CreateSubPose(uint32_t ID, iAllocator& allocator);
        Pose* FindPose(uint32_t poseID);

        Vector<Pose> poses;

        static AnimationFieldID_t GetAnimationFieldID() { return GetTypeGUID(PoseState); }
	};


	/************************************************************************************************/


	struct FLEXKITAPI Skeleton
	{
		Skeleton() = default;

		Skeleton(iAllocator* allocator, size_t jointCount = 64) 
		{
			InitiateSkeleton(allocator, jointCount);
		}

		Joint&			operator [] (JointHandle hndl);

		void			InitiateSkeleton(iAllocator* Allocator, size_t jointCount = 64);

		JointHandle		AddJoint			(Joint J, const float4x4& I);
		void			AddAnimationClip	(AnimationClip, iAllocator* allocator);

        PoseState       CreatePoseState(iAllocator& allocator);

		float4x4		GetInversePose		(const JointHandle H);
		JointHandle		FindJoint			(const char*);

		float4x4*	    IPose		= nullptr; // Global Inverse Space Pose
		Joint*			Joints		= nullptr;
		JointPose*		JointPoses	= nullptr;
		size_t			JointCount	= 0;
		GUID_t			guid		= INVALIDHANDLE;
		iAllocator*		Memory		= nullptr;

		struct AnimationList
		{
			AnimationClip	Clip;
			AnimationList*	Next;
			iAllocator*		Memory;
		};

		AnimationList* Animations		= nullptr;
	};


    
	/************************************************************************************************/


    PoseState   CreatePoseState(Skeleton& skeleton, iAllocator* allocator);
    bool        InitiatePoseState(RenderSystem* RS, PoseState* EAS, PoseState_DESC& Desc, VShaderJoint* InitialState);

    ResourceHandle   LoadMorphTarget(TriMesh* triMesh, const char* morphTargetName, RenderSystem&, CopyContextHandle handle, iAllocator&);


    /************************************************************************************************/


    struct AnimatorBlobInputState
    {
        uint32_t    type;
        char        data[16];
        char        name[32];
    };


    struct AnimatorBlobAnimatorState
    {
        uint64_t animationResourceID;
        uint32_t initialState;
    };


    struct AnimatorBlobHeader
    {
        uint32_t CRC32;
        uint32_t blockType;
        uint32_t blockSize;
        uint32_t componentID;

        uint64_t scriptResource;
        uint32_t inputCount;
        uint32_t stateCount;
    };


}	/************************************************************************************************/

#endif
