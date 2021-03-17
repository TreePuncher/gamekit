#include "AnimationComponents.h"


namespace FlexKit
{   /************************************************************************************************/

    
    AnimationResource* LoadAnimation(const char* resourceName, iAllocator& allocator)
    {
        auto asset = LoadGameAsset(resourceName);

        if (asset != -1)
        {
            auto animationBlob  = (AnimationResourceBlob*)GetAsset(asset);
            auto& animation     = allocator.allocate<AnimationResource>(allocator);

            const size_t trackCount     = animationBlob->header.trackCount;
            size_t currentFileOffset    = 0;

            for (size_t I = 0; I < trackCount; ++I)
            {
                const AnimationTrackHeader* header      = reinterpret_cast<AnimationTrackHeader*>(animationBlob->Buffer + currentFileOffset);
                const AnimationKeyFrame*    keyFrames   = reinterpret_cast<AnimationKeyFrame*>(animationBlob->Buffer + currentFileOffset + sizeof(AnimationTrackHeader));
                const uint32_t              frameCount  = header->frameCount;

                Vector<AnimationKeyFrame> frames{ &allocator, frameCount };


                for (uint32_t J = 0; J < frameCount; ++J)
                {
                    AnimationKeyFrame inputFrame;
                    memcpy(&inputFrame, &keyFrames[J], sizeof(inputFrame));
                    frames.emplace_back(inputFrame);
                }

                AnimationTrack track{
                    .keyFrames  = std::move(frames),
                    .type       = header->type,
                    .trackName  = header->trackName,
                    .target     = header->target,
                };

                currentFileOffset += header->byteSize;

                animation.tracks.emplace_back(std::move(track));
            }

            return &animation;
        }

        return nullptr;
    }


    /************************************************************************************************/


    PlayID_t AnimatorComponent::AnimatorView::Play(AnimationResource& anim, bool loop)
    {
        auto&           componentData   = GetComponent()[animator];
        const uint32_t  ID              = rand();

        auto& allocator = GetComponent().allocator;
        Vector<AnimationState::TrackState> tracks{ &allocator };


        auto& gameObject    = *componentData.gameObject;
        auto  skeleton      = GetSkeleton(gameObject);

        for (auto& track : anim.tracks)
        {
            switch (track.type)
            {
            case TrackType::Skeletal:
            {
                auto joint = skeleton->FindJoint(track.target.c_str());

                if (track.trackName == "translation")
                {
                    AnimationState::TrackState trackState;
                    trackState.target   = &allocator.allocate<AnimationState::JointTranslationTarget>(joint);
                    trackState.track    = &track;
                    tracks.push_back(trackState);
                }
                else if (track.trackName == "rotation")
                {
                    AnimationState::TrackState trackState;
                    trackState.target   = &allocator.allocate<AnimationState::JointRotationTarget>(joint);
                    trackState.track    = &track;
                    tracks.push_back(trackState);
                }
                else if (track.trackName == "scale")
                {
                    AnimationState::TrackState trackState;
                    trackState.target   = &allocator.allocate<AnimationState::JointScaleTarget>(joint);
                    trackState.track    = &track;
                    tracks.push_back(trackState);
                }
            }   break;
            }  
        }

        AnimationState animState{
            .T          = 0.0f,
            .ID         = ID,
            .state      = loop ? AnimationState::State::Looping : AnimationState::State::Playing,
            .tracks     = std::move(tracks),
            .resource   = &anim,
        };

        componentData.animations.emplace_back(std::move(animState));

        return ID;
    }


    /************************************************************************************************/


    void AnimatorComponent::AnimationState::JointRotationTarget::Apply(AnimatorComponent::AnimationState::FrameRange range, float t, AnimationStateContext& ctx)
    {
        if (auto res = ctx.FindField<PoseState>(); res) {
            auto beginKey   = range.begin->Value;
            auto endKey     = range.end->Value;

            auto interpolate =
                [&]() -> Quaternion
                {
                    const auto timepointBegin   = range.begin->Begin;
                    const auto timepointEnd     = range.end->Begin;
                    const auto timeRange        = timepointEnd - timepointBegin;

                    const auto I = (t - range.begin->Begin) / timeRange;

                    const auto& AValue = range.begin->Value;
                    const auto& BValue = range.end->Value;

                    const auto A = Quaternion{ AValue[0], AValue[1], AValue[2], AValue[3] };
                    const auto B = Quaternion{ BValue[0], BValue[1], BValue[2], BValue[3] };

                    return Qlerp(A, B, I);
                };

            const auto value        = range.begin->Begin == range.end->Begin ? range.begin->Value : interpolate();

            res->Joints[joint].r = Quaternion{ value[0], value[1], value[2], value[3] };
        }
    }


    /************************************************************************************************/


    AnimationKeyFrame* AnimatorComponent::AnimationState::TrackState::FindFrame(double T)
    {
        for (auto& frame : track->keyFrames)
        {
            if (frame.Begin <= T && frame.End > T)
                return &frame;
        }

        return nullptr;
    }


    /************************************************************************************************/


    AnimationKeyFrame* AnimatorComponent::AnimationState::TrackState::FindNextFrame(AnimationKeyFrame* frame)
    {
        if (frame + 1 < track->keyFrames.end())
            return frame + 1;
        else
            return frame;
    }


    /************************************************************************************************/


    AnimatorComponent::AnimationState::State AnimatorComponent::AnimationState::Update(AnimationStateContext& ctx, double dT)
    {
        T += 1.0f / 600.0f;
        bool animationPlayed = false;

        if (state == State::Paused || state == State::Finished)
            return state;

        for (auto& track : tracks)
        {
            if (auto frame = track.FindFrame(T); frame)
            {
                track.target->Apply({ frame, track.FindNextFrame(frame) }, T, ctx);
                animationPlayed = true;
            }
        }

        if (state == State::Looping && !animationPlayed)
            T = 0.0;

        if (state == State::Playing && !animationPlayed)
            state = State::Finished;

        return state;
    }


    /************************************************************************************************/


    UpdateTask& UpdateAnimations(UpdateDispatcher& updateTask, double dT)
    {
        struct _ {};
        return updateTask.Add<_>(
            [](auto&, auto&) {},
            [dT = dT](auto&, auto& temporaryAllocator)
            {
                auto& animator = AnimatorComponent::GetComponent();

                for (AnimatorComponent::AnimatorState& animator : animator.animators)
                {
                    AnimatorComponent::AnimationStateContext context{ temporaryAllocator };

                    Apply(*animator.gameObject,
                        [&](SkeletonView& poseView)
                        {
                            auto& pose = poseView.GetPoseState();
                            context.AddField(pose);
                        });

                    for (auto& animation : animator.animations)
                        animation.Update(context, dT);
                }
            });
    }


    /************************************************************************************************/


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
                        {
                            const auto res = ComputeLOD(drawable, POS, 10'000.0f);
                            out_skinned.push_back({ &drawable, &skeleton.GetPoseState(), res.recommendedLOD });
                        }
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


    void UpdatePose(PoseState& pose)
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


    UpdatePoseTask& UpdatePoses(UpdateDispatcher& dispatcher, GatherSkinnedTask& skinnedObjects)
    {
        auto& task = dispatcher.Add<UpdatePosesTaskData>(
			[&](auto& builder, UpdatePosesTaskData& data)
			{
				data.skinned        = &skinnedObjects.GetData().skinned;

                builder.SetDebugString("Update Poses");
			},
			[](UpdatePosesTaskData& data, iAllocator& threadAllocator)
			{
                FK_LOG_9("Start Pose Updates.\n");

                for (auto& skinnedObject : *data.skinned)
                    UpdatePose(*skinnedObject.pose);

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
