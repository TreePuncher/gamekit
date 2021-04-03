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


    inline AnimationResource* LoadAnimation(GUID_t resourceId, iAllocator& allocator)
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


        JointHandle FindJoint(const char* jointID)
        {
            auto& poseState = GetPoseState();
            return poseState.Sk->FindJoint(jointID);
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


    Skeleton*   GetSkeleton     (GameObject& gameObject);
    PoseState*  GetPoseState    (GameObject& gameObject);
    size_t      GetJointCount   (GameObject& gameObject);

    JointPose   GetJointPose    (GameObject& gameObject, JointHandle jointID);
    JointHandle GetJointParent  (GameObject& gameObject, JointHandle jointID);

    void        SetJointPose    (GameObject& gameObject, JointHandle jointID, JointPose pose);

    void        RotateJoint         (GameObject& gameObject, JointHandle jointID, const Quaternion Q);
    bool        hasSkeletonLoaded   (GameObject& gameObject);


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

            PlayID_t Play(AnimationResource& anim, bool loop = false);

            AnimatorHandle animator;
        };

		AnimatorComponent(iAllocator& IN_allocator) :
            allocator   {  IN_allocator },
            handles     { &IN_allocator },
			animators   { &IN_allocator } {}

        AnimatorHandle Create(GameObject& gameObject)
        {
            auto handle     = handles.GetNewHandle();
            handles[handle] = (index_t)animators.emplace_back(&gameObject, Vector<AnimationState>{ &allocator });

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

                void Apply(FrameRange range, float t, AnimationStateContext& ctx) final override;

                JointHandle joint;
            };

            struct JointTranslationTarget : public ITrackTarget
            {
                JointTranslationTarget(JointHandle in_Joint) : joint{ in_Joint } {}

                void Apply(FrameRange range, float t, AnimationStateContext& ctx) final override
                {
                    if (auto res = ctx.FindField<PoseState::Pose>(); res)
                    {
                        auto defaultPose = res->sk->JointPoses[joint];
                        res->jointPose[joint].ts += range.begin->Value.xyz() - defaultPose.ts.xyz();
                    }
                }

                JointHandle joint;
            };

            struct JointScaleTarget : public ITrackTarget
            {
                JointScaleTarget(JointHandle in_Joint) : joint{ in_Joint } {}

                void Apply(FrameRange range, float t, AnimationStateContext& ctx) final override
                {
                    if (auto res = ctx.FindField<PoseState::Pose>(); res)
                    {
                        res->jointPose[joint].ts.w = range.begin->Value.w;
                    }
                }

                JointHandle joint;
            };

            struct TrackState
            {
                AnimationKeyFrame* FindFrame(double T);
                AnimationKeyFrame* FindNextFrame(AnimationKeyFrame* frame);

                AnimationTrack* track;
                ITrackTarget*   target;
            };


            State Update(AnimationStateContext& ctx, double dT);

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

    using AnimatorView = AnimatorComponent::AnimatorView;

    UpdateTask& UpdateAnimations(UpdateDispatcher& updateTask, double dT);

    constexpr ComponentID FABRIKComponentID         = GetTypeGUID(FABRIK);
    constexpr ComponentID FABRIKTargetComponentID   = GetTypeGUID(FABRIKTarget);

    using FABRIKHandle          = Handle_t<32, FABRIKComponentID>;
    using FABRIKTargetHandle    = Handle_t<32, FABRIKTargetComponentID>;

    struct FABRIKTarget
    {
        FABRIKTarget(NodeHandle IN_node = InvalidHandle_t, iAllocator* IN_allocator = nullptr) :
            node    { IN_node },
            users   { IN_allocator }{}

        ~FABRIKTarget();

        NodeHandle              node;
        Vector<FABRIKHandle>    users;
    };

    using FABRIKTargetComponent = BasicComponent_t<FABRIKTarget, FABRIKTargetHandle, FABRIKTargetComponentID>;
    using FABRIKTargetView      = FABRIKTargetComponent::View;

    class FABRIKComponent : public FlexKit::Component<FABRIKComponent, FABRIKComponentID>
    {
    public:
        struct FABRIK;

        class FABRIKView : public FlexKit::ComponentView_t<FABRIKComponent>
        {
        public:
            FABRIKView(GameObject& IN_gameObject) :
                ikHandle{ GetComponent().Create(IN_gameObject) } {}



            void AddTarget(GameObject& gameObject) 
            {
                Apply(
                    gameObject,
                    [&](FABRIKTargetView& view)
                    {
                        view->users.push_back(ikHandle);
                        GetComponent()[ikHandle].AddTarget(view->node);
                    });
            }

            FABRIK* operator -> ()
            {
                return &GetComponent()[ikHandle];
            }


            void SetEndEffector(JointHandle endEffector)
            {
                GetComponent()[ikHandle].endEffector = endEffector;
            }

            FABRIKHandle ikHandle;
        };


        FABRIKComponent(iAllocator& IN_allocator) :
            instances   { &IN_allocator },
            handles     { &IN_allocator },
            allocator   { IN_allocator } {}

        FABRIKHandle Create(GameObject& gameObject)
        {
            auto handle     = handles.GetNewHandle();
            auto index      = (index_t)instances.emplace_back(allocator, gameObject);
            handles[handle] = index;

            GetPoseState(gameObject)->CreateSubPose(instances[index].poseID, allocator);

            return handle;
        }


        struct FABRIK
        {
            FABRIK(iAllocator& allocator, GameObject& gameObject) :
                targets     { &allocator },
                gameObject  { &gameObject },
                Debug       { &allocator } {}

            struct Target
            {
                NodeHandle target;
            };


            void AddTarget(NodeHandle target)
            {
                targets.emplace_back(target);
            }

            void RemoveTarget(NodeHandle target)
            {

            }

            Vector<Target>  targets;
            GameObject*     gameObject;

            JointHandle     endEffector     = InvalidHandle_t;
            JointHandle     ikRoot          = InvalidHandle_t;
            uint32_t        iterationCount  = 5;
            uint32_t        poseID          = rand();

            Vector<float3>  Debug;
        };


        FABRIK& operator [](FABRIKHandle handle)
        {
            return instances[handles[handle]];
        }

        Vector<FABRIK>                              instances;
        HandleUtilities::HandleTable<FABRIKHandle>  handles;
        iAllocator&                                 allocator;
    };


    UpdateTask& UpdateIKControllers(UpdateDispatcher& dispatcher, double dt);

    using FABRIKView = FABRIKComponent::FABRIKView;


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
