#ifndef ANIMATIONCOMPONENTS_H_INCLUDED
#define ANIMATIONCOMPONENTS_H_INCLUDED

#include "AnimationUtilities.h"
#include "AnimationRuntimeUtilities.h"

#include "Components.h"
#include "GraphicScene.h"
#include "Transforms.h"

namespace FlexKit
{	/************************************************************************************************/


    struct AnimationTrack
    {
        Vector<AnimationKeyFrame>   keyFrames;

        TrackType                   type;
        std::string                 trackName;
        std::string                 target;
    };

    struct AnimationResource
    {
        AnimationResource(iAllocator& IN_allocator) :
            tracks{ &IN_allocator } {}

        Vector<AnimationTrack> tracks;
    };


    struct AnimationState
    {
        AnimationResource* animation;
    };


    AnimationResource* LoadAnimation(GUID_t resourceId, iAllocator& allocator)
    {
        return nullptr;
    }


    AnimationResource* LoadAnimation(const char* resourceName, iAllocator& allocator);


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

                pose.r = pose.r * Q;

                skeleton.SetPose(jointID, pose);
            });
    }


    /************************************************************************************************/


    inline JointHandle GetJointParent(GameObject& gameObject, JointHandle jointID)
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


    inline size_t GetJointCount(GameObject& gameObject)
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
        uint32_t    lodLevel;
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
        const PosedDrawableList*	skinned;

        UpdateTask*         task;
        operator UpdateTask* () { return task; }
    };

    using GatherSkinnedTask = UpdateTaskTyped<GatherSkinnedTaskData>&;
    using UpdatePoseTask    = UpdateTaskTyped<UpdatePosesTaskData>&;

    void                GatherSkinned   (GraphicScene* SM, CameraHandle Camera, PosedDrawableList& out_skinned);
    GatherSkinnedTask&  GatherSkinned   (UpdateDispatcher& dispatcher, GraphicScene* scene, CameraHandle C, iAllocator* allocator);
    UpdatePoseTask&     UpdatePoses     (UpdateDispatcher& dispatcher, GatherSkinnedTask& skinnedObjects);


    void UpdatePose(PoseState& pose);


    /************************************************************************************************/
    

	constexpr ComponentID AnimatorComponentID = GetTypeGUID(Animator);
	using AnimatorHandle = Handle_t<32, AnimatorComponentID>;

    using PlayID_t = uint32_t;

    struct AnimationStateField
    {
        uint32_t    fieldID;
        void*       data;
    };


    template<Animatable TY>
    struct StateField_t : public AnimationStateField
    {
        StateField_t(TY& field)
        {
            data    = &field;
            fieldID = TY::GetAnimationFieldID();
        }

        static TY*          GetField(AnimationStateField& field)    { return reinterpret_cast<TY*>(field.data); }
        static uint32_t     GetFieldID()                            { return TY::GetFieldID(); }
    };

	class AnimatorComponent : public FlexKit::Component<AnimatorComponent, AnimatorComponentID>
	{
	public:
        class AnimatorView : public FlexKit::ComponentView_t<AnimatorComponent>
        {
        public:
            AnimatorView(GameObject& IN_gameObject, AnimatorHandle IN_animatorHandle = InvalidHandle_t) :
                animator{ IN_animatorHandle != InvalidHandle_t ? IN_animatorHandle : GetComponent().Create(IN_gameObject) } {}

            PlayID_t Play(AnimationResource& anim, bool loop = false)
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

            AnimatorHandle animator;
        };

		AnimatorComponent(iAllocator& IN_allocator) :
            allocator   {  IN_allocator },
            handles     { &IN_allocator },
			animators   { &IN_allocator } {}

        AnimatorHandle Create(GameObject& gameObject)
        {
            auto handle     = handles.GetNewHandle();
            handles[handle] = animators.emplace_back(&gameObject, Vector<AnimationState>{ &allocator });

            return handle;
        }


        struct AnimationStateContext
        {
            AnimationStateContext(iAllocator& allocator) :
                fields{ &allocator } {}

            template<Animatable Field_TY>
            void AddField(Field_TY& field)
            {
                fields.emplace_back(StateField_t{ field });
            }

            template<Animatable Field_TY>
            Field_TY* FindField()
            {
                auto res = std::find_if(fields.begin(), fields.end(),
                    [&](auto& e) -> bool
                    {
                        return e.fieldID == Field_TY::GetAnimationFieldID();
                    });

                if (res != fields.end())
                    return StateField_t<Field_TY>::GetField(*res);
                else
                    return nullptr;
            }

            Vector<AnimationStateField> fields;
        };

        struct AnimationState
        {
            float       T;
            uint32_t    ID;

            enum class State
            {
                Paused,
                Playing,
                Finished,
                Looping
            }   state = State::Playing;

            struct FrameRange
            {
                AnimationKeyFrame* begin;
                AnimationKeyFrame* end;
            };

            struct ITrackTarget
            {
                virtual void Apply(FrameRange range, float t, AnimationStateContext& ctx) = 0;
            };

            struct JointRotationTarget : public ITrackTarget
            {
                JointRotationTarget(JointHandle in_Joint) : joint { in_Joint }{}

                void Apply(FrameRange range, float t, AnimationStateContext& ctx) final override
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

                JointHandle joint;
            };

            struct JointTranslationTarget : public ITrackTarget
            {
                JointTranslationTarget(JointHandle in_Joint) : joint{ in_Joint } {}

                void Apply(FrameRange range, float t, AnimationStateContext& ctx) final override
                {
                    if (auto res = ctx.FindField<PoseState>(); res)
                    {
                        auto defaultPose = res->Sk->JointPoses[joint];
                        res->Joints[joint].ts += range.begin->Value.xyz() - defaultPose.ts.xyz();
                    }
                }

                JointHandle joint;
            };

            struct JointScaleTarget : public ITrackTarget
            {
                JointScaleTarget(JointHandle in_Joint) : joint{ in_Joint } {}

                void Apply(FrameRange range, float t, AnimationStateContext& ctx) final override
                {
                    if (auto res = ctx.FindField<PoseState>(); res)
                    {
                        res->Joints[joint].ts.w = range.begin->Value.w;
                    }
                }

                JointHandle joint;
            };

            struct TrackState
            {
                AnimationKeyFrame* FindFrame(double T)
                {
                    for (auto& frame : track->keyFrames)
                    {
                        if (frame.Begin <= T && frame.End > T)
                            return &frame;
                    }

                    return nullptr;
                }

                AnimationKeyFrame* FindNextFrame(AnimationKeyFrame* frame)
                {
                    if (frame + 1 < track->keyFrames.end())
                        return frame + 1;
                    else
                        return frame;
                }

                AnimationTrack* track;
                ITrackTarget*   target;
            };

            State Update(AnimationStateContext& ctx, double dT)
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

            Vector<TrackState>  tracks;
            AnimationResource*  resource;
        };

		struct AnimatorState
		{
            GameObject*             gameObject;
            Vector<AnimationState>  animations;
			AnimationStateMachine   ASM;
		};

        AnimatorState& operator [](AnimatorHandle handle)
        {
            return animators[handles[handle]];
        }

		Vector<AnimatorState>							animators;
		HandleUtilities::HandleTable<AnimatorHandle>	handles;
        iAllocator&                                     allocator;
	};


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


    using AnimatorView = AnimatorComponent::AnimatorView;

}	/************************************************************************************************/
#endif


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
