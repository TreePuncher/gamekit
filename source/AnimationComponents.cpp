#include "AnimationComponents.h"
#include <angelscript.h>

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
				return InvalidHandle;
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
				return InvalidHandle;
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


	float Animation::Duration()
	{
		float end = 0.0f;

		for (auto& t : tracks)
			end = Max(end, t.keyFrames.back().End);

		return end;
	}


	/************************************************************************************************/


	Animation* LoadAnimation(GUID_t resourceId, iAllocator& allocator)
	{
		auto asset = LoadGameAsset(resourceId);

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


	Animation* LoadAnimation(const char* resourceName, iAllocator& allocator)
	{
		auto guid = FindAssetGUID(resourceName);
		if (guid)
			return LoadAnimation(*guid, allocator);
		else
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

				if (joint == InvalidHandle)
					continue;
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


	void AnimatorComponent::AnimatorView::Stop(PlayID_t playID)
	{
		auto& componentData = GetComponent()[animator];

		auto res = std::find_if(componentData.animations.begin(), componentData.animations.end(),
			[&](AnimationState& clip)
			{
				return clip.ID == playID;
			});

		componentData.animations.remove_unstable(res);
	}


	/************************************************************************************************/


	void AnimatorComponent::AnimatorView::Pause(PlayID_t playID)
	{
		auto& componentData = GetComponent()[animator];

		auto res = std::find_if(
			componentData.animations.begin(),
			componentData.animations.end(),
			[&](AnimationState& clip)
			{
				return clip.ID == playID;
			});

		if(res != componentData.animations.end())
			res->state = AnimationState::State::Paused;
	}


	/************************************************************************************************/


	void AnimatorComponent::AnimatorView::SetProgress(PlayID_t playID, float p)
	{
		auto& componentData = GetComponent()[animator];

		auto res = std::find_if(
			componentData.animations.begin(),
			componentData.animations.end(),
			[&](AnimationState& clip)
			{
				return clip.ID == playID;
			});

		if (res != componentData.animations.end())
		{
			auto T = res->resource->Duration() * Saturate(p);
			res->T = T;
		}
	}


	/************************************************************************************************/


	AnimatorComponent::AnimatorState::AnimatorState(GameObject* IN_gameObject, iAllocator& allocator)
		: gameObject    { IN_gameObject }
		, animations    { &allocator    }
		, inputValues   { &allocator    }
		, inputIDs      { &allocator    } {}

	AnimatorComponent::AnimatorState::~AnimatorState()
	{
		if (obj)
			obj->Release();
	}


	/************************************************************************************************/


	std::optional<AnimatorComponent::InputValue*> AnimatorComponent::AnimatorView::GetInputValue(uint32_t idx) noexcept
	{
		auto& state = GetState();

		if (state.inputValues.size() > idx)
			return &state.inputValues[idx];
		else
			return {};
	}


	std::optional<AnimatorInputType> AnimatorComponent::AnimatorView::GetInputType(uint32_t idx) noexcept
	{
		auto& state = GetState();

		if(state.inputValues.size() > idx)
			return state.inputIDs[idx].type;
		else
			return {};
	}


	AnimatorComponent::AnimatorState& AnimatorComponent::AnimatorView::GetState() noexcept
	{
		return GetComponent()[animator];
	}


	AnimatorScriptState AnimatorComponent::AnimatorView::GetScriptState() noexcept
	{
		auto& state = GetComponent()[animator];

		return { state.gameObject, state.obj };
	}



	uint32_t AnimatorComponent::AnimatorView::AddInput(const char* name, AnimatorInputType type, void* _ptr) noexcept
	{
		auto& state = GetState();

		InputID ID;
		ID.type = type;
		strncpy_s(ID.stringID, name, 32);

		state.inputIDs.push_back(ID);
		auto idx = state.inputValues.emplace_back();

		if(_ptr)
			memcpy(&state.inputValues[idx], _ptr, sizeof(16));

		return (uint32_t)idx;
	}


	/************************************************************************************************/


	void AnimatorComponent::AnimatorView::SetAnimationState(uint32_t playId, uint32_t newState) noexcept
	{
		auto& state = GetState();

		for (auto& anim : state.animations)
		{
			if (anim.ID == playId)
			{
				anim.state = (AnimatorComponent::AnimationState::State)newState;
				return;
			}
		}
	}


	/************************************************************************************************/


	void AnimatorComponent::AnimatorView::SetObj(void* _ptr) noexcept
	{
		auto& state = GetState();
		if (state.obj)
			state.obj->Release();

		state.obj = (asIScriptObject*)_ptr;

		if(state.obj)
			state.obj->AddRef();
	}


	/************************************************************************************************/


	void AnimatorComponent::AnimationState::JointRotationTarget::Apply(AnimatorComponent::AnimationState::FrameRange range, float t, AnimationStateContext& ctx)
	{
		if (auto res = ctx.FindField<FlexKit::PoseState::Pose>(); res) {
			auto beginKey   = range.begin->Value;
			auto endKey     = range.end->Value;

			auto interpolate =
				[&]() -> Quaternion
				{
					const auto timepointBegin   = range.begin->Begin;
					const auto timepointEnd     = range.end->Begin;
					const auto timeRange        = timepointEnd - timepointBegin;

					const auto I = clamp(0.0f, (t - range.begin->Begin) / timeRange, 1.0f);

					const auto& AValue = range.begin->Value;
					const auto& BValue = range.end->Value;

					const auto A = Quaternion{ AValue[0], AValue[1], AValue[2], AValue[3] };
					const auto B = Quaternion{ BValue[0], BValue[1], BValue[2], BValue[3] };

					return Qlerp(A, B, I);
				};

			const auto value        = range.begin->Begin == range.end->Begin ? range.begin->Value : interpolate();
			const auto defaultPose  = res->sk->JointPoses[joint];

			res->jointPose[joint].r *= Quaternion{ value[0], value[1], value[2], value[3] };
		}
	}


	/************************************************************************************************/


	void AnimatorComponent::AnimationState::JointTranslationTarget::Apply(AnimatorComponent::AnimationState::FrameRange range, float t, AnimationStateContext& ctx)
	{
		if (auto res = ctx.FindField<FlexKit::PoseState::Pose>(); res)
		{
			const auto timepointBegin   = range.begin->Begin;
			const auto timepointEnd     = range.end->Begin;
			const auto timeRange        = timepointEnd - timepointBegin;
			const auto u                = clamp(0.0f, (t - range.begin->Begin) / timeRange, 1.0f);

			const auto defaultPose  = res->sk->JointPoses[joint];
			const auto xyz          = lerp(range.begin->Value.xyz(), range.end->Value.xyz(), u);

			//res->jointPose[joint].ts += xyz - defaultPose.ts.xyz();
		}
	}


	/************************************************************************************************/


	void AnimatorComponent::AnimationState::JointScaleTarget::Apply(AnimatorComponent::AnimationState::FrameRange range, float t, AnimationStateContext& ctx)
	{
		if (auto res = ctx.FindField<PoseState::Pose>(); res)
			res->jointPose[joint].ts.w = range.begin->Value.w;
	}


	/************************************************************************************************/


	AnimationKeyFrame* AnimatorComponent::AnimationState::TrackState::FindFrame(double T)
	{
		for (auto& frame : track->keyFrames)
		{
			if (frame.Begin <= T && frame.End > T)
				return &frame;
		}

		return &track->keyFrames.back();
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
		if (state == State::Finished)
			return state;

		float endT = 0;
		for (auto& track : tracks)
		{
			endT = Max(track.track->keyFrames.back().End, endT);

			if (auto frame = track.FindFrame(T); frame)
				track.target->Apply({ frame, track.FindNextFrame(frame) }, T, ctx);
		}

		if(state != State::Paused)
			T += dT;

		if (state == State::Looping && T >= endT)
			T = 0.0;
		else if (state == State::Playing && T >= endT)
			state = State::Finished;

		return state;
	}


	/************************************************************************************************/


	AnimatorView* GetAnimator(GameObject& gameObject)
	{
		return Apply(gameObject, [](AnimatorView& view) { return &view; }, []() -> AnimatorView* { return nullptr; });
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

				if (!animatorComponent.animators.size())
					return;

				auto ctx = GetContext();

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

					auto scriptObj = animator.obj;

					if(scriptObj)
					{
						auto api_obj    = static_cast<asIScriptObject*>(scriptObj);
						auto preUpdate  = api_obj->GetObjectType()->GetMethodByName("PreUpdate");
						auto postUpdate = api_obj->GetObjectType()->GetMethodByName("PostUpdate");

						ctx->Prepare(preUpdate);
						ctx->SetObject(animator.obj);
						ctx->SetArgAddress(0, animator.gameObject);
						ctx->SetArgDouble(1, dT);
						ctx->Execute();

						for (auto& animation : animator.animations)
							animation.Update(context, dT);

						ctx->Prepare(postUpdate);
						ctx->SetObject(animator.obj);
						ctx->SetArgAddress(0, animator.gameObject);
						ctx->SetArgDouble(1, dT);
						ctx->Execute();
					}
				}

				ReleaseContext(ctx);
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

						if (Intersects(F, BS))
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


	void SkeletonComponent::Release(SkeletonHandle handle)
	{
		FK_LOG_WARNING("SkeletonComponent::Release Unimplemented! Memory Leak!");
	}


	/************************************************************************************************/


	void SkeletonComponent::AddComponentView(GameObject& gameObject, ValueMap userValues, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
	{
		SkeletonComponentBlob blob;
		memcpy(&blob, buffer, bufferSize);

		if (blob.assetID != INVALIDHANDLE)
			auto& sk = gameObject.AddView<SkeletonView>(blob.assetID);
	}


	/************************************************************************************************/


	void SkeletonComponent::FreeComponentView(void* _ptr)
	{
		static_cast<SkeletonView*>(_ptr)->Release();
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

						if (poseState == nullptr || IKController.endEffector == InvalidHandle || IKController.targets.size() == 0)
							return;

						auto ikRoot = IKController.ikRoot;

						auto GetParentJoint = [&](JointHandle joint) -> JointHandle
							{
								return poseState->Sk->Joints[joint].mParent;
							};


						auto GetParentPosition = [&](JointHandle joint)
							{
								const auto parent = GetParentJoint(joint);

								if (parent != InvalidHandle)
									return (poseState->CurrentPose[parent] * float4{ 0, 0, 0, 1 }).xyz();
								else
									return float3{ 0, 0, 0 };
							};

						auto GetRootPosition = [&]
							{
								if(ikRoot != InvalidHandle)
								{
									return (poseState->CurrentPose[ikRoot] * float4{ 0, 0, 0, 1 }).xyz();
								}
								else
								{
									JointHandle itr = IKController.endEffector;
									while (GetParentJoint(itr) != InvalidHandle, itr = GetParentJoint(itr));
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
						for(auto joint = IKController.endEffector; GetParentJoint(joint) != InvalidHandle; joint = GetParentJoint(joint))
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
								if (parent != InvalidHandle)
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


	/************************************************************************************************/


	void AnimatorComponent::AddComponentView(GameObject& gameObject, ValueMap userValues, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
	{
		AnimatorBlobHeader header;
		memcpy(&header, buffer, sizeof(header));

		auto& animator = gameObject.AddView<AnimatorView>();

		AnimatorBlobInputState*     inputs = (AnimatorBlobInputState*)(buffer + sizeof(header));
		AnimatorBlobAnimatorState*  states = (AnimatorBlobAnimatorState*)(buffer + sizeof(header) + header.inputCount * sizeof(AnimatorBlobAnimatorState));

		for (size_t I = 0; I < header.inputCount; I++)
		{
			AnimatorBlobInputState input;
			memcpy(&input, inputs + I, sizeof(input));

			animator.AddInput(input.name, (AnimatorInputType)input.type, input.data);
		}

		for (size_t I = 0; I < header.stateCount; I++)
		{
			AnimatorBlobAnimatorState state;
			memcpy(&state, states + I, sizeof(state));

			auto animation  = LoadAnimation(state.animationResourceID, SystemAllocator);
			auto playId     = animator.Play(*animation);

			animator.SetAnimationState(playId, state.initialState);
		}

		if (header.scriptResource != -1)
		{
			auto scriptModule   = LoadByteCodeAsset(header.scriptResource);
			auto func           = scriptModule->GetFunctionByName("InitiateAnimator");

			if (func)
			{
				auto ctx = GetContext();
				ctx->Prepare(func);
				ctx->SetArgAddress(0, &gameObject);
				ctx->Execute();

				auto animatorObj = ctx->GetReturnAddress();

				ReleaseContext(ctx);

				if (animatorObj)
					animator.SetObj(animatorObj);
			}
		}
	}


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
