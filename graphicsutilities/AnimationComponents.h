#ifndef ANIMATIONCOMPONENTS_H_INCLUDED
#define ANIMATIONCOMPONENTS_H_INCLUDED

#include "AnimationUtilities.h"
#include "AnimationRuntimeUtilities.h"

#include "Components.h"
#include "GraphicScene.h"
#include "Transforms.h"

namespace FlexKit
{	/************************************************************************************************/


	constexpr ComponentID AnimatorComponentID = GetTypeGUID(Animator);
	using AnimatorHandle = Handle_t<32, AnimatorComponentID>;

	class AnimatorComponent : public FlexKit::Component<AnimatorComponent, AnimatorComponentID>
	{
	public:
		AnimatorComponent(iAllocator* allocator) : 
			animators{ allocator } {}

		struct AnimatorState
		{
			AnimationStateMachine ASM;
		};

		Vector<AnimatorState>							animators;
		HandleUtilities::HandleTable<AnimatorHandle>	handles;
	};


	/************************************************************************************************/


	constexpr ComponentID SkeletonComponentID = GetTypeGUID(Skeleton);
	using SkeletonHandle = Handle_t<32, SkeletonComponentID>;

    class SkeletonComponent : public FlexKit::Component<SkeletonComponent, SkeletonComponentID>
    {
    public:
        SkeletonComponent(iAllocator* allocator) :
            skeletons   { allocator },
            handles     { allocator },
            allocator   { allocator } {}

        SkeletonHandle Create(const TriMeshHandle triMesh, const AssetHandle asset)
        {
            auto mesh = GetMeshResource(triMesh);
            if (!mesh)
                return InvalidHandle_t;

            if (!mesh->Skeleton)
            {
                const auto skeletonGuid  = asset;
                const auto available     = isAssetAvailable(skeletonGuid);

                if (!available)
                    return InvalidHandle_t;

                const auto resource = LoadGameAsset(skeletonGuid);
                mesh->Skeleton      = Resource2Skeleton(resource, allocator);
                mesh->SkeletonGUID  = asset;
            }


            const auto handle = handles.GetNewHandle();
            handles[handle] = (uint32_t)skeletons.push_back({ mesh->Skeleton, CreatePoseState(*mesh->Skeleton, allocator) });

            return handle;
        }


        void AddComponentView(GameObject& GO, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);


        struct SkeletonState
        {
            Skeleton*   skeleton;
            PoseState	poseState;
        };

        SkeletonState& operator [](SkeletonHandle handle)
        {
            return skeletons[handles[handle]];
        }

		Vector<SkeletonState>							skeletons;
		HandleUtilities::HandleTable<SkeletonHandle>	handles;
        iAllocator*                                     allocator;
	};


    class SkeletonView : public FlexKit::ComponentView_t<SkeletonComponent>
    {
    public:
        SkeletonView(const TriMeshHandle triMesh, const AssetHandle asset) : handle{ GetComponent().Create(triMesh, asset) }
        {

        }

        PoseState& GetPoseState()
        {
            return GetComponent()[handle].poseState;
        }


        auto GetSkeleton()
        {
            return GetComponent()[handle].skeleton;
        }

        void SetPose(JointHandle jointId, JointPose pose)
        {
            GetPoseState().Joints[jointId] = pose;
        }


        JointPose GetPose(JointHandle jointId)
        {
            return GetPoseState().Joints[jointId];
        }


        SkeletonHandle handle;
    };

	/************************************************************************************************/


	constexpr ComponentID BindPointComponentID = GetTypeGUID(BindPoint);
	using BindPointHandle = Handle_t<32, BindPointComponentID>;

	class BindPointComponent : public FlexKit::Component<BindPointComponent, BindPointComponentID>
	{
	public:
		BindPointComponent(iAllocator* allocator) : bindPoints{ allocator } {}

		struct BindPoint
		{
			SkeletonHandle	skeleton;
			NodeHandle		node;
		};

		Vector<BindPoint>								bindPoints;
		HandleUtilities::HandleTable<SkeletonHandle>	handles;
	};

    using BindPointComponentView = BasicComponentView_t<BindPointComponent>;


    /************************************************************************************************/


    inline bool hasSkeletonLoaded(GameObject& gameObject)
    {
        return Apply(
            gameObject,
            [](SkeletonView& skeleton){ return true; },
            []{ return false; });
    }


    /************************************************************************************************/


    inline Skeleton* GetSkeleton(GameObject& gameObject)
    {
        return Apply(
            gameObject,
            [](SkeletonView& view) -> Skeleton* { return view.GetSkeleton(); },
            []() -> Skeleton* { return nullptr; });
    }


    /************************************************************************************************/


    inline PoseState* GetPoseState(GameObject& gameObject)
    {
        return Apply(
            gameObject,
            [](SkeletonView& view) -> PoseState* { return &view.GetPoseState(); },
            []() -> PoseState* { return nullptr; });
    }


    /************************************************************************************************/


    inline void RotateJoint(GameObject& gameObject, JointHandle jointID, const Quaternion Q)
    {
        Apply(
            gameObject,
            [&](SkeletonView& skeleton)
            {
                auto pose = skeleton.GetPose(jointID);

                pose.r = pose.r.normalized() * Q.normalized();

                skeleton.SetPose(jointID, pose);
            });
    }


    /************************************************************************************************/


    inline JointPose GetJointPose(GameObject& gameObject, JointHandle jointID)
    {
        return Apply(
            gameObject,
            [&](SkeletonView& skeleton) -> JointPose
            {
                return skeleton.GetPose(jointID);
            },
            [] { return JointPose{}; });
    }


    /************************************************************************************************/


    inline void SetJointPose(GameObject& gameObject, JointHandle jointID, JointPose pose)
    {
        Apply(
            gameObject,
            [&](SkeletonView& skeleton)
            {
                skeleton.SetPose(jointID, pose);
            });
    }


    /************************************************************************************************/


    struct PosedDrawable
    {
        Drawable*   drawable;
        PoseState*  pose;
    };


    using PosedDrawableList = Vector<PosedDrawable>;

    struct GatherSkinnedTaskData
    {
        CameraHandle	    camera;
        GraphicScene*       scene; // Source Scene
        StackAllocator	    taskMemory;
        PosedDrawableList	skinned;

        UpdateTask*         task;

        operator UpdateTask* () { return task; }
    };

    struct UpdatePosesTaskData
    {
        StackAllocator	            taskMemory;
        const PosedDrawableList*	skinned;

        UpdateTask*         task;
        operator UpdateTask* () { return task; }
    };

    using GatherSkinnedTask = UpdateTaskTyped<GatherSkinnedTaskData>&;
    using UpdatePoseTask    = UpdateTaskTyped<UpdatePosesTaskData>&;

    void                GatherSkinned   (GraphicScene* SM, CameraHandle Camera, PosedDrawableList& out_skinned);
    GatherSkinnedTask&  GatherSkinned   (UpdateDispatcher& dispatcher, GraphicScene* scene, CameraHandle C, iAllocator* allocator);
    UpdatePoseTask&     UpdatePoses     (UpdateDispatcher& dispatcher, GatherSkinnedTask& skinnedObjects, iAllocator* allocator);


    void UpdatePose(PoseState& pose, iAllocator* );


}	/************************************************************************************************/
#endif


/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

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
