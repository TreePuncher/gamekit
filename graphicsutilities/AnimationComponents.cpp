#include "AnimationComponents.h"


namespace FlexKit
{   /************************************************************************************************/


    void GatherSkinned(GraphicScene* SM, CameraHandle Camera, PosedDrawableList& out_skinned)
    {
		FK_ASSERT(Camera	!= CameraHandle{(unsigned int)INVALIDHANDLE});
		FK_ASSERT(SM		!= nullptr);

		auto& cameraComponent = CameraComponent::GetComponent();

		const auto CameraNode	= cameraComponent.GetCameraNode(Camera);
		const float3	POS		= GetPositionW(CameraNode);
		const Quaternion Q		= GetOrientation(CameraNode);
		const auto F			= GetFrustum(Camera);
		const auto& Visibles	= SceneVisibilityComponent::GetComponent();

		for(auto handle : SM->sceneEntities)
		{
			const auto potentialVisible = Visibles[handle];

			if(	potentialVisible.visable && 
				potentialVisible.entity->hasView(DrawableComponent::GetComponentID()))
			{
				auto Ls	= GetLocalScale		(potentialVisible.node).x;
				auto Pw	= GetPositionW		(potentialVisible.node);
				auto Lq	= GetOrientation	(potentialVisible.node);
				auto BS = BoundingSphere{ 
					Lq * potentialVisible.boundingSphere.xyz() + Pw, 
					Ls * potentialVisible.boundingSphere.w };

				Apply(*potentialVisible.entity,
					[&](DrawableView& drawView,
                        SkeletonView& skeleton)
					{
                        auto& drawable = drawView.GetDrawable();
						if (drawable.Skinned && CompareBSAgainstFrustum(&F, BS))
                            out_skinned.push_back({ &drawable, &skeleton.GetPoseState() });
					});
			}
		}
    }


    /************************************************************************************************/


    GatherSkinnedTask& GatherSkinned(UpdateDispatcher& dispatcher, GraphicScene* scene, CameraHandle C, iAllocator* allocator)
    {
        auto& task = dispatcher.Add<GatherSkinnedTaskData>(
			[&](auto& builder, GatherSkinnedTaskData& data)
			{
				size_t taskMemorySize = KILOBYTE * 2048;
				data.taskMemory.Init((byte*)allocator->malloc(taskMemorySize), taskMemorySize);
				data.scene			= scene;
				data.skinned        = PosedDrawableList{ data.taskMemory };
				data.camera			= C;

                builder.SetDebugString("Gather Scene");
			},
			[](GatherSkinnedTaskData& data)
			{
                FK_LOG_INFO("Start gather skinned objects.\n");

                GatherSkinned(data.scene, data.camera, data.skinned);

                FK_LOG_INFO("End gather skinned objects.\n");
			});

		return task;
    }


    /************************************************************************************************/


    void UpdatePose(PoseState& pose, iAllocator* allocator)
    {
        Skeleton* skeleton      = pose.Sk;
        const size_t jointCount = skeleton->JointCount;

        float4x4* M = (float4x4*)allocator->_aligned_malloc(jointCount * sizeof(float4x4));

        for (size_t I = 0; I < jointCount; ++I)
            M[I] = GetPoseTransform(pose.Joints[I]) * GetPoseTransform(skeleton->JointPoses[I]);

        for (size_t I = 0; I < jointCount; ++I)
        {
            auto P              = (skeleton->Joints[I].mParent != 0xFFFF) ? M[skeleton->Joints[I].mParent] : float4x4::Identity();
            M[I]                = M[I] * P;
            pose.CurrentPose[I] = Float4x4ToXMMATIRX(M[I]);
        }
    }


    /************************************************************************************************/


    UpdatePoseTask& UpdatePoses(UpdateDispatcher& dispatcher, GatherSkinnedTask& skinnedObjects, iAllocator* allocator)
    {
        auto& task = dispatcher.Add<UpdatePosesTaskData>(
			[&](auto& builder, UpdatePosesTaskData& data)
			{
				size_t taskMemorySize = KILOBYTE * 2048;
				data.taskMemory.Init((byte*)allocator->malloc(taskMemorySize), taskMemorySize);
				data.skinned        = &skinnedObjects.GetData().skinned;

                builder.SetDebugString("Update Poses");
			},
			[](UpdatePosesTaskData& data)
			{
                FK_LOG_INFO("Start Pose Updates.\n");

                for (auto& skinnedObject : *data.skinned)
                {
                    UpdatePose(*skinnedObject.pose, data.taskMemory);
                    data.taskMemory.clear();
                }

                FK_LOG_INFO("End Pose Updates.\n");
			});

		return task;
    }




    /************************************************************************************************/

}


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
