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
						if (drawable.Skinned && Intersects(F, BS))
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
			[](GatherSkinnedTaskData& data, iAllocator& threadAllocator)
			{
                FK_LOG_9("Start gather skinned objects.\n");

                GatherSkinned(data.scene, data.camera, data.skinned);

                FK_LOG_9("End gather skinned objects.\n");
			});

		return task;
    }


    /************************************************************************************************/


    void UpdatePose(PoseState& pose, iAllocator* allocator)
    {
        Skeleton* skeleton      = pose.Sk;
        const size_t jointCount = skeleton->JointCount;

        for (size_t I = 0; I < jointCount; ++I)
        {
            const auto poseT        = GetPoseTransform(pose.Joints[I]);
            const auto skeletonT    = GetPoseTransform(skeleton->JointPoses[I]);

            const auto parent       = skeleton->Joints[I].mParent;
            const auto P            = (parent != 0xFFFF) ? pose.CurrentPose[parent] : float4x4::Identity();

            pose.CurrentPose[I]     = poseT * skeletonT * P;
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
			[](UpdatePosesTaskData& data, iAllocator& threadAllocator)
			{
                FK_LOG_9("Start Pose Updates.\n");

                for (auto& skinnedObject : *data.skinned)
                {
                    UpdatePose(*skinnedObject.pose, data.taskMemory);
                    data.taskMemory.clear();
                }

                FK_LOG_9("End Pose Updates.\n");
			});

		return task;
    }



    /************************************************************************************************/


    void SkeletonComponent::AddComponentView(GameObject& GO, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
    {
        SkeletonComponentBlob blob;
        memcpy(&blob, buffer, bufferSize);

        auto triMeshHandle = GetTriMesh(GO);
        auto skeleton = Create(triMeshHandle, blob.assetID);

        GO.AddView<SkeletonView>(triMeshHandle, skeleton);
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
