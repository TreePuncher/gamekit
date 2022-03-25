#include "AnimationComponents.h"


namespace FlexKit
{   /************************************************************************************************/


    bool hasSkeletonLoaded(GameObject& gameObject)
    {
        return Apply(
            gameObject,
            [](SkeletonView& skeleton){ return true; },
            []{ return false; });
    }


    /************************************************************************************************/


    Skeleton* GetSkeleton(GameObject& gameObject)
    {
        return Apply(
            gameObject,
            [](SkeletonView& view) -> Skeleton* { return view.GetSkeleton(); },
            []() -> Skeleton* { return nullptr; });
    }


    /************************************************************************************************/


    PoseState* GetPoseState(GameObject& gameObject)
    {
        return Apply(
            gameObject,
            [](SkeletonView& view) -> PoseState* { return &view.GetPoseState(); },
            []() -> PoseState* { return nullptr; });
    }


    /************************************************************************************************/


    void RotateJoint(GameObject& gameObject, JointHandle jointID, const Quaternion Q)
    {
        Apply(
            gameObject,
            [&](SkeletonView& skeleton)
            {
                auto pose = skeleton.GetPose(jointID);

                pose.r = pose.r * Q;

                skeleton.SetPose(jointID, pose);
            });
    }


    /************************************************************************************************/


    JointHandle GetJoint(GameObject& gameObject, const char* ID)
    {
        return Apply(
            gameObject,
            [&](SkeletonView& skeleton) -> JointHandle
            {
                return skeleton.GetSkeleton()->FindJoint(ID);
            },
            []() -> JointHandle
            {
                return InvalidHandle_t;
            });
    }


    /************************************************************************************************/


    JointHandle GetJointParent(GameObject& gameObject, JointHandle jointID)
    {
        return Apply(
            gameObject,
            [&](SkeletonView& skeleton) -> JointHandle
            {
                return skeleton.GetSkeleton()->Joints[jointID].mParent;
            },
            []() -> JointHandle
            {
                return InvalidHandle_t;
            });
    }


    /************************************************************************************************/


    size_t GetJointCount(GameObject& gameObject)
    {
        return Apply(
            gameObject,
            [&](SkeletonView& skeleton) -> size_t
            {
                return skeleton.GetSkeleton()->JointCount;
            },
            []() -> size_t
            {
                return 0;
            });
    }


    /************************************************************************************************/


    JointPose GetJointPose(GameObject& gameObject, JointHandle jointID)
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


    void SetJointPose(GameObject& gameObject, JointHandle jointID, JointPose pose)
    {
        Apply(
            gameObject,
            [&](SkeletonView& skeleton)
            {
                skeleton.SetPose(jointID, pose);
            });
    }


    /************************************************************************************************/


    Animation* LoadAnimation(const char* resourceName, iAllocator& allocator)
    {
        auto asset = LoadGameAsset(resourceName);

        if (asset != -1)
        {
            auto animationBlob  = (AnimationResourceBlob*)GetAsset(asset);
            auto& animation     = allocator.allocate<Animation>(allocator);

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


    PlayID_t AnimatorComponent::AnimatorView::Play(Animation& anim, bool loop)
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
        if (auto res = ctx.FindField<PoseState::Pose>(); res) {
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

            const auto value = range.begin->Begin == range.end->Begin ? range.begin->Value : interpolate();

            res->jointPose[joint].r *= Quaternion{ value[0], value[1], value[2], value[3] };
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
            [dT = dT](auto&, iAllocator& temporaryAllocator)
            {
                ProfileFunction();

                auto& animatorComponent = AnimatorComponent::GetComponent();

                for (AnimatorComponent::AnimatorState& animator : animatorComponent.animators)
                {
                    AnimatorComponent::AnimationStateContext context{ temporaryAllocator };

                    Apply(*animator.gameObject,
                        [&](SkeletonView& poseView)
                        {
                            auto& pose = poseView.GetPoseState();

                            if (auto res = pose.FindPose(GetTypeGUID(AnimationPose)); res)
                            {
                                res->Clear(pose.JointCount);
                                context.AddField(*res);
                            }
                            else
                            {
                                auto& subPose = pose.CreateSubPose(GetTypeGUID(AnimationPose), animatorComponent.allocator);
                                context.AddField(subPose);
                            }

                            context.AddField(pose);
                        });

                    for (auto& animation : animator.animations)
                        animation.Update(context, dT);
                }
            });
    }


    /************************************************************************************************/


    void GatherSkinned(Scene* SM, CameraHandle Camera, PosedBrushList& out_skinned)
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
				potentialVisible.entity->hasView(BrushComponent::GetComponentID()))
			{
				auto Ls	= GetLocalScale		(potentialVisible.node).x;
				auto Pw	= GetPositionW		(potentialVisible.node);
				auto Lq	= GetOrientation	(potentialVisible.node);
				auto BS = BoundingSphere{ 
					Lq * potentialVisible.boundingSphere.xyz() + Pw, 
					Ls * potentialVisible.boundingSphere.w };

				Apply(*potentialVisible.entity,
					[&](BrushView& drawView,
                        SkeletonView& skeleton)
					{
                        auto& brush = drawView.GetBrush();

                        if (brush.Skinned && Intersects(F, BS))
                        {
                            const auto res = ComputeLOD(brush, POS, 10'000.0f);
                            out_skinned.push_back({ &brush, &skeleton.GetPoseState(), res.recommendedLOD });
                        }
					});
			}
		}
    }


    /************************************************************************************************/


    GatherSkinnedTask& GatherSkinned(UpdateDispatcher& dispatcher, Scene* scene, CameraHandle C, iAllocator* allocator)
    {
        auto& task = dispatcher.Add<GatherSkinnedTaskData>(
			[&](auto& builder, GatherSkinnedTaskData& data)
			{
				size_t taskMemorySize = KILOBYTE * 2048;
				data.taskMemory.Init((byte*)allocator->malloc(taskMemorySize), taskMemorySize);
				data.scene			= scene;
				data.skinned        = PosedBrushList{ data.taskMemory };
				data.camera			= C;

                builder.SetDebugString("Gather Scene");
			},
			[](GatherSkinnedTaskData& data, iAllocator& threadAllocator)
			{
                ProfileFunction();

                FK_LOG_9("Start gather skinned objects.\n");

                GatherSkinned(data.scene, data.camera, data.skinned);

                FK_LOG_9("End gather skinned objects.\n");
			});

		return task;
    }


    /************************************************************************************************/


    void UpdatePose(PoseState& pose, iAllocator& tempMemory)
    {
        ProfileFunction();

        Skeleton* skeleton      = pose.Sk;
        const size_t jointCount = skeleton->JointCount;

        auto temp = (JointPose*)tempMemory._aligned_malloc(sizeof(PoseState) * pose.JointCount);
        for (size_t I = 0; I < pose.JointCount; ++I)
            temp[I] = JointPose{ { 0, 0, 0, 1 }, { 0, 0, 0 , 1 } };

        // Add all poses up
        for (auto& subPose : pose.poses)
        {
            for (size_t I = 0; I < pose.JointCount; ++I)
            {
                temp[I].r       *= subPose.jointPose[I].r;
                temp[I].ts.w    *= subPose.jointPose[I].ts.w;
                temp[I].ts      += subPose.jointPose[I].ts.xyz();
            }
        }

        for (size_t I = 0; I < pose.JointCount; ++I)
            pose.Joints[I] = temp[I];

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
                ProfileFunction();

                FK_LOG_9("Start Pose Updates.\n");

                for (auto& skinnedObject : *data.skinned)
                    UpdatePose(*skinnedObject.pose, threadAllocator);

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



    FABRIKTarget::~FABRIKTarget()
    {
        auto& IKComponent = FABRIKComponent::GetComponent();

        for (auto& user : users)
            IKComponent[user].RemoveTarget(node);
    }


    /************************************************************************************************/


    UpdateTask& UpdateIKControllers(UpdateDispatcher& dispatcher, double dt)
    {
        struct _ {};
        return dispatcher.Add<_>(
            [](auto&, auto&) {},
            [&](auto&, iAllocator& allocator)
            {
                ProfileFunction();

                auto& IKControllers = FABRIKComponent::GetComponent();
                using IKInstance    = FABRIKComponent::FABRIK;

                Parallel_For(
                    *dispatcher.threads,
                    allocator,
                    IKControllers.instances.begin(),
                    IKControllers.instances.end(), 
                    1,
                    [&](IKInstance& IKController, iAllocator& allocator)
                    {
                        ProfileFunction();

                        auto* poseState   = GetPoseState(*IKController.gameObject);
                        auto* skeleton    = poseState->Sk;

                        if (poseState == nullptr || IKController.endEffector == InvalidHandle_t || IKController.targets.size() == 0)
                            return;

                        auto ikRoot = IKController.ikRoot;

                        auto GetParentJoint = [&](JointHandle joint) -> JointHandle
                            {
                                return poseState->Sk->Joints[joint].mParent;
                            };


                        auto GetParentPosition = [&](JointHandle joint)
                            {
                                const auto parent = GetParentJoint(joint);

                                if (parent != InvalidHandle_t)
                                    return (poseState->CurrentPose[parent] * float4{ 0, 0, 0, 1 }).xyz();
                                else
                                    return float3{ 0, 0, 0 };
                            };

                        auto GetRootPosition = [&]
                            {
                                if(ikRoot != InvalidHandle_t)
                                {
                                    return (poseState->CurrentPose[ikRoot] * float4{ 0, 0, 0, 1 }).xyz();
                                }
                                else
                                {
                                    JointHandle itr = IKController.endEffector;
                                    while (GetParentJoint(itr) != InvalidHandle_t, itr = GetParentJoint(itr));
                                    return (poseState->CurrentPose[itr] * float4{ 0, 0, 0, 1 }).xyz();
                                }
                            };


                        const float4x4  WT              = GetWT(*IKController.gameObject);
                        const float4x4  IT              = FastInverse(WT);
                        const float3    targetPosition  = (IT * float4{ GetPositionW(IKController.targets.front().target), 1 }).xyz();
                        const float3    rootPosition    = GetRootPosition();
                        const size_t    jointCount      = poseState->JointCount;


                        Vector<float3>      jointPositions  { &allocator, poseState->JointCount };
                        Vector<float>       boneLengths     { &allocator, poseState->JointCount };
                        Vector<JointHandle> joints          { &allocator, poseState->JointCount };

                        // Get Initial Position in pose space
                        for(auto joint = IKController.endEffector; GetParentJoint(joint) != InvalidHandle_t; joint = GetParentJoint(joint))
                        {
                            const float3        jointPosition   = (poseState->CurrentPose[joint] * float4{ 0, 0, 0, 1 }).xyz();
                            const JointHandle   parent          = GetParentJoint(joint);
                            const float3        parentPosition  = GetParentPosition(joint);
                            const float         boneLength      = (parentPosition - jointPosition).magnitude();

                            jointPositions.emplace_back(parentPosition);
                            boneLengths.emplace_back(boneLength);
                            joints.emplace_back(parent);
                        }

                        const float chainLength = std::accumulate(boneLengths.begin(), boneLengths.end(), 0.0f);

                        if (const float D = (jointPositions.back() - targetPosition).magnitude(); D > chainLength)
                        {// Target farther than reach of chain
                            const size_t jointCount     = poseState->JointCount;
                            const float3 targetVector   = (targetPosition - rootPosition).normal();
                            float3 positionOffset       = 0.0f;

                            for (size_t I = 0; I < jointPositions.size(); I++)
                            {
                                jointPositions[jointPositions.size() - I - 1] = rootPosition + positionOffset;

                                const auto boneVector = targetVector * boneLengths[jointPositions.size() - I - 1];
                                positionOffset += boneVector;
                            }
                        }
                        else
                        {
                            const size_t iterationCount = IKController.iterationCount;

                            for (size_t I = 0; I < iterationCount; ++I)
                            {
                                // Forward
                                for (size_t J = 0; J < jointCount - 1; ++J)
                                {
                                    const float3 position         = jointPositions[J];
                                    const float3 boneOrientation  = (position - jointPositions[J - 1]).normal();

                                    float3 boneTarget{ 0 };

                                    if (J == 0)
                                    {
                                        float3 span             = position - targetPosition;
                                        float3 boneTargetAdj    = { 0 };

                                        if ( span.magnitudeSq() == 0.0f ||
                                             span.normal().dot(boneOrientation) < 0.0001f)
                                                boneTargetAdj += float3{ 0.00001f, 0.00001f, 0.00001f };

                                        boneTarget = targetPosition + boneTargetAdj;
                                    }
                                    else
                                        boneTarget = jointPositions[J - 1];

                                    const float3 boneLength     = boneLengths[J];
                                    const float3 span           = boneTarget - position;

                                    const float3 orientation    = span.normal();
                                    const float3 D              = span.magnitude();

                                    const float3 newPosition    = position + orientation * (D - boneLength);
                                    
                                    jointPositions[J]           = newPosition;
                                }

                                // Backward
                                for (int J = (int)jointPositions.size() - 1; J >= 0; --J)
                                {
                                    const float3 boneTarget   = J == (jointPositions.size() - 1) ? rootPosition : jointPositions[J + 1];
                                    const float3 position     = jointPositions[J];
                                    const float3 span         = boneTarget - position;
                                    const float3 orientation  = span.normal();
                                    const float  boneLength   = J == (jointCount - 2) ? 0.0f : boneLengths[J];
                                    const float  D            = span.magnitude();

                                    const auto newPosition  = position + orientation * (D - boneLength);

                                    if(D != 0.0f)
                                        jointPositions[J] = newPosition;
                                }
                            }
                        }

                        // convert solution to joint transforms
                        Vector<float3> solution{ &allocator, poseState->JointCount };
                        const float3 T      = (targetPosition - jointPositions.front()).normal();
                        const float3 tail   = jointPositions.front() + T * boneLengths.front();

                        solution.emplace_back(tail);
                        solution += jointPositions;


                        Vector<float4x4> transforms{ &allocator, poseState->JointCount };

                        auto GetParentTransform =
                            [&](JointHandle node)
                            {
                                const auto parent = skeleton->Joints[node].mParent;
                                if (parent != InvalidHandle_t)
                                    return transforms[parent];
                                else
                                    return float4x4::Identity();
                            };

                        auto* pose = poseState->FindPose(IKController.poseID);
                        if (!pose)
                            return;

                        for (size_t I = 0; I < jointCount; I++)
                        {
                            const float4x4 parentT = GetParentTransform(JointHandle{ (uint32_t)I });
                            const float4x4 parentI = Inverse(parentT);
                            const float4x4 IP      = skeleton->IPose[I];

                            const float3 L  = (parentI * float4{ 1, 0, 0, 0 }).xyz();
                            const float3 F  = (parentI * float4{ 0, 0, 1, 0 }).xyz();

                            const float3 A  = (parentI * float4(solution[solution.size() - 2 - I], 0)).xyz();
                            const float3 B  = (parentI * float4(solution[solution.size() - 1 - I], 0)).xyz();

                            const float3 U          = (A - B).normal();
                            const float3 left       = U.cross(F).normal();
                            const float3 forward    = left.cross(U).normal();
                            const Quaternion q      = Vector2Quaternion(left, U, forward);

                            transforms.emplace_back(Quaternion2Matrix(q) * parentT);
                            pose->jointPose[I].r = q;
                        }


                        IKController.Debug = solution;
                    });
            });
    }


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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
