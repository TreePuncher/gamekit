#include "GraphicScene.h"
#include "GraphicsComponents.h"
#include "intersection.h"

#include "Components.h"
#include "componentBlobs.h"
#include "AnimationRuntimeUtilities.H"
#include "ProfilingUtilities.h"

#include <cmath>

namespace FlexKit
{
	/************************************************************************************************/


    std::pair<GameObject*, bool> FindGameObject(GraphicScene& scene, const char* id)
    {
        auto& visableComponent = SceneVisibilityComponent::GetComponent();

        for (auto& visable : scene)
        {
            auto& gameObject = *visableComponent[visable].entity;
            auto res = Apply(
                gameObject,
                [&]
                (
                    StringIDView& ID
                )
                {
                    auto goID = ID.GetString();
                    return strncmp(goID, id, 64) == 0;
                },
                []
                { return false; });

            if (res)
                return { &gameObject, true };
        }

        return { nullptr, false };
    }


    void DEBUG_ListSceneObjects(GraphicScene& scene)
    {
        auto& visableComponent = SceneVisibilityComponent::GetComponent();

        for (auto& visable : scene)
        {
            auto& gameObject = *visableComponent[visable].entity;
            Apply(
                gameObject,
                [&]
                (
                    StringIDView& ID
                )
                {
                    std::cout << ID.GetString() << '\n';
                });
        }
    }


    /************************************************************************************************/


    void SetVisable(GameObject& gameObject, bool v)
    {
        Apply(
            gameObject,
            [&]( SceneVisibilityView& visable )
            {
                visable.SetVisable(v);
            });
    }


    /************************************************************************************************/


    void DrawableComponentEventHandler::OnCreateView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
    {
        auto node = GetSceneNode(gameObject);
        if (node == InvalidHandle_t)
            return;

        DrawableComponentBlob drawableComponent;
        memcpy(&drawableComponent, buffer, bufferSize);

        auto [triMesh, loaded] = FindMesh(drawableComponent.resourceID);

        if (!loaded)
            triMesh = LoadTriMeshIntoTable(renderSystem, renderSystem.GetImmediateUploadQueue(), drawableComponent.resourceID);

        gameObject.AddView<DrawableView>(triMesh, node);
        SetBoundingSphereFromMesh(gameObject);
    }


    /************************************************************************************************/

    
    void PointLightEventHandler::OnCreateView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
    {
        PointLightComponentBlob pointLight;

        memcpy(&pointLight, buffer, sizeof(pointLight));

        gameObject.AddView<PointLightView>(pointLight.K, pointLight.IR[0], sqrt(pointLight.IR[0]), GetSceneNode(gameObject));
        SetBoundingSphereFromLight(gameObject);

        EnablePointLightShadows(gameObject);
    }


    /************************************************************************************************/


	void GraphicScene::AddGameObject(GameObject& go, NodeHandle node)
	{
		go.AddView<SceneVisibilityView>(go, node, sceneID);

		Apply(go, 
			[&](SceneVisibilityView& visibility)
			{
				sceneEntities.push_back(visibility);
			});
	}


	/************************************************************************************************/



	void GraphicScene::RemoveEntity(GameObject& go)
	{
		Apply(go, 
		[&, allocator = this->allocator](SceneVisibilityView& vis) 
		{
			const auto handle = vis.visibility;
			go.RemoveView(vis);

			sceneEntities.remove_unstable(
				find(sceneEntities, [&](auto i) { return i == handle; }));
		},	[] { });
	}


	/************************************************************************************************/


	void GraphicScene::ClearScene()
	{
		auto&	visables	= SceneVisibilityComponent::GetComponent();
		auto	visableID	= SceneVisibilityComponent::GetComponentID();

		for (auto visHandle : sceneEntities)
		{
			auto entity		= visables[visHandle].entity;
			auto visable	= entity->GetView(visableID);
			entity->RemoveView(visable);
		}

		sceneEntities.clear();
        bvh = SceneBVH(*allocator);
	}


	/************************************************************************************************/


	//Drawable&	GraphicScene::GetDrawable(SceneEntityHandle EHandle) 
	//{ 
	//	return Drawables.at(HandleTable[EHandle]); 
	//}


	/************************************************************************************************/


	//BoundingSphere	GraphicScene::GetBoundingSphere(SceneEntityHandle EHandle)
	//{
	//	BoundingSphere out;
	//
	//	float3		position		= GetEntityPosition(EHandle);
	//	Quaternion	orientation		= GetOrientation(EHandle);
	//	auto BS = GetMeshBoundingSphere(GetMeshHandle(EHandle));
	//
	//	return BoundingSphere{ orientation * position + BS.xyz(), BS.w };
	//}


	/************************************************************************************************/

	//bool LoadAnimation(GraphicScene* GS, SceneEntityHandle EHandle, ResourceHandle RHndl, TriMeshHandle MeshHandle, float w = 1.0f)
	//{
		/*
		auto Resource = GetAsset(RHndl);
		if (Resource->Type == EResourceType::EResource_SkeletalAnimation)
		{
			auto AC = Resource2AnimationClip(Resource, GS->Memory);
			FreeAsset(RHndl);// No longer in memory once loaded

			auto mesh				= GetMeshResource(MeshHandle);
			AC.Skeleton				= mesh->Skeleton;
			mesh->AnimationData    |= EAnimationData::EAD_Skin;

			if (AC.Skeleton->Animations)
			{
				auto I = AC.Skeleton->Animations;
				while (I->Next)
					I = I->Next;

				I->Next				= &GS->Memory->allocate_aligned<Skeleton::AnimationList, 0x10>();
				I->Next->Clip		= AC;
				I->Next->Memory		= GS->Memory;
				I->Next->Next		= nullptr;
			}
			else
			{
				AC.Skeleton->Animations			= &GS->Memory->allocate_aligned<Skeleton::AnimationList, 0x10>();
				AC.Skeleton->Animations->Clip	= AC;
				AC.Skeleton->Animations->Next	= nullptr;
				AC.Skeleton->Animations->Memory = GS->Memory;
			}

			return true;
		}
		return false;
		*/
	//}


	/************************************************************************************************/

	/*
	GSPlayAnimation_RES GraphicScene::EntityPlayAnimation(SceneEntityHandle EHandle, GUID_t Guid, float W, bool Loop)
	{
		auto MeshHandle		= GetDrawable(EHandle).MeshHandle;
		bool SkeletonLoaded = IsSkeletonLoaded(MeshHandle);
		if (!SkeletonLoaded)
			return { false, -1 };

		if (SkeletonLoaded)
		{
			auto S = FlexKit::GetSkeleton(MeshHandle);
			Skeleton::AnimationList* I = S->Animations;
			bool AnimationLoaded = false;

			// Find if Animation is Already Loaded
			{
				
				while (I)
				{
					if (I->Clip.guid == Guid) {
						AnimationLoaded = true;
						break;
					}
					
					I = I->Next;
				}
			}
			if (!AnimationLoaded)
			{
				// Search Resources for Animation
				if (isAssetAvailable(Guid))
				{
					auto RHndl = LoadGameAsset(Guid);
					auto Res = LoadAnimation(this, EHandle, RHndl, MeshHandle, W);
					if(!Res)
						return{ false, -1 };
				}
				else
					return{ false, -1 };
			}

			int64_t ID = INVALIDHANDLE;
			auto Res = PlayAnimation(&GetDrawable(EHandle), Guid, Memory, Loop, W, ID);
			if(Res == EPLAY_ANIMATION_RES::EPLAY_SUCCESS)
				return { true, ID };

			return{ false, -1};
		}
		return{ false, -1 };
	}
	*/

	/************************************************************************************************/

	/*
	GSPlayAnimation_RES GraphicScene::EntityPlayAnimation(SceneEntityHandle EHandle, const char* Animation, float W, bool Loop)
	{
		auto MeshHandle		= GetDrawable(EHandle).MeshHandle;
		bool SkeletonLoaded = IsSkeletonLoaded(MeshHandle);
		if (!SkeletonLoaded)
			return { false, -1 };

		if (SkeletonLoaded && HasAnimationData(MeshHandle))
		{
			// TODO: Needs to Iterate Over Clips
			auto S = FlexKit::GetSkeleton(MeshHandle);
			if (!strcmp(S->Animations->Clip.mID, Animation))
			{
				int64_t AnimationID = -1;
				if(PlayAnimation(&GetDrawable(EHandle), Animation, Memory, Loop, W, &AnimationID))
					return { true, AnimationID };
				return{ false, -1 };
			}
		}

		// Search Resources for Animation
		if(isAssetAvailable(Animation))
		{
			auto RHndl = LoadGameAsset(Animation);
			int64_t AnimationID = -1;
			if (LoadAnimation(this, EHandle, RHndl, MeshHandle, W)) {
				if(PlayAnimation(&GetDrawable(EHandle), Animation, Memory, Loop, W, &AnimationID) == EPLAY_SUCCESS)
					return { true, AnimationID };
				return{ false, -1};
			}
			else
				return { false, -1 };
		}
		return { false, -1 };
	}
	*/

	/************************************************************************************************/


	/*
	void UpdateGraphicScenePoseTransform(GraphicScene* SM)
	{
		for(auto Tag : SM->TaggedJoints)
		{
			auto Entity = SM->GetDrawable(Tag.Source);

			auto WT		= GetJointPosed_WT(Tag.Joint, Entity.Node, Entity.PoseState);
			auto WT_t	= Float4x4ToXMMATIRX(&WT.Transpose());

			FlexKit::SetWT		(Tag.Target, &WT_t);
			FlexKit::SetFlag	(Tag.Target, SceneNodes::StateFlags::UPDATED);
		}
	}
	*/


	/************************************************************************************************/


    #define ComponentSize 9
    #define ComponentMask (1 << ComponentSize) - 1

    inline uint CreateMortonCode(const uint3 XYZ)
    {
        const uint X = uint(XYZ[0]) & ComponentMask;
        const uint Y = uint(XYZ[1]) & ComponentMask;
        const uint Z = uint(XYZ[2]) & ComponentMask;

        uint mortonCode = 0;

        for (uint I = 0; I < ComponentSize; I++)
        {
            const uint x_bit = X & (1 << I);
            const uint y_bit = Y & (1 << I);
            const uint z_bit = Z & (1 << I);

            const uint XYZ = x_bit << 2 | y_bit << 0 | z_bit << 1;
            //const uint XYZ = x_bit << 0 | y_bit << 1 | z_bit << 2;

            mortonCode |= XYZ << I * 3;
        }

        return mortonCode;
    }


    /************************************************************************************************/


    SceneBVH SceneBVH::Build(GraphicScene& scene, iAllocator& allocator)
    {
        ProfileFunction();

        auto& visables              = scene.sceneEntities;
        auto& visibilityComponent   = SceneVisibilityComponent::GetComponent();

        auto elements   = Vector<BVHElement>{ &allocator, visables.size() };
        auto nodes      = Vector<BVHNode>{ &allocator, visables.size() * 2 };

        AABB worldAABB;

        for (auto& visable : visables)
            worldAABB += visibilityComponent[visable].GetAABB();

        const float3 worldSpan  = worldAABB.Span();
        const float3 offset     = -worldAABB.Min;

        for (auto& visable : visables)
        {
            const auto POS          = GetPositionW(visibilityComponent[visable].node);
            const auto normalizePOS = (offset + POS) / worldSpan;
            const auto ID           = CreateMortonCode({
                                        (uint)(normalizePOS.x * ComponentMask),
                                        (uint)(normalizePOS.y * ComponentMask),
                                        (uint)(normalizePOS.z * ComponentMask)});

            elements.push_back({ ID, visable });
        }

        std::sort(elements.begin(), elements.end());

        // Phase 1 - Build Leaf Nodes -
        const size_t end            = std::ceil(elements.size() / 4.0f);
        const size_t elementCount   = elements.size();

        for (uint16_t I = 0; I < end; ++I)
        {
            BVHNode node;

            const uint16_t begin  = 4 * I;
            const uint16_t end    = (uint16_t)Min(elementCount, 4 * I + 4);

            for (uint16_t II = 4 * I; II < end; II++) {
                const uint16_t idx  = II;
                auto& visable       =  visables[elements[idx].handle];
                const auto aabb     = visibilityComponent[visable].GetAABB();
                node.boundingVolume += aabb;
            }

            node.children   = 4 * I;
            node.count      = end - begin;
            node.Leaf       = true;

            nodes.push_back(node);
        }

        // Phase 2 - Build Interior Nodes -
        uint begin = 0;
        while (true)
        {
            const uint end  = (uint)nodes.size();
            const uint temp = (uint)end;

            if (end - begin > 1)
            {
                for (uint I = begin; I < end; I += 4)
                {
                    BVHNode node;

                    const size_t localEnd = Min(end, I + 4);
                    for (size_t II = I; II < localEnd; II++)
                        node.boundingVolume += nodes[II].boundingVolume;

                    node.children   = I;
                    node.count      = uint8_t(localEnd - I);
                    node.Leaf       = false;

                    nodes.push_back(node);
                }
            }
            else
                break;

            begin = temp;
        }

        SceneBVH BVH{ allocator };
        BVH.elements    = elements.Copy(allocator);
        BVH.nodes       = nodes.Copy(allocator);
        BVH.root        = begin;

        return BVH;
    }


    /************************************************************************************************/


	void GatherScene(GraphicScene* SM, CameraHandle Camera, PVS& out, PVS& T_out)
	{
        ProfileFunction();

		FK_ASSERT(&out		!= &T_out);
		FK_ASSERT(Camera	!= CameraHandle{(unsigned int)INVALIDHANDLE});
		FK_ASSERT(SM		!= nullptr);
		FK_ASSERT(&out		!= nullptr);
		FK_ASSERT(&T_out	!= nullptr);


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
					[&](DrawableView& view)
					{
                        auto& drawable = view.GetDrawable();

						if (!drawable.Skinned && Intersects(F, BS))
						{
							float distance = 0;
							if (potentialVisible.transparent)
								PushPV(drawable, T_out, POS);
							else
								PushPV(drawable, out, POS);
						}
					});
			}
		}
	}


	/************************************************************************************************/


    UpdateTaskTyped<GetPVSTaskData>& GatherScene(UpdateDispatcher& dispatcher, GraphicScene* scene, CameraHandle C, iAllocator& allocator)
	{
		auto& task = dispatcher.Add<GetPVSTaskData>(
			[&](auto& builder, auto& data)
			{
				data.scene			= scene;
				data.camera			= C;

                builder.SetDebugString("Gather Scene");
			},
			[&allocator = allocator](GetPVSTaskData& data, iAllocator& threadAllocator)
			{
                ProfileFunction();

                PVS solid       { &threadAllocator };
                PVS transparent { &threadAllocator };

                GatherScene(data.scene, data.camera, solid, transparent);
				SortPVS(&solid, &CameraComponent::GetComponent().GetCamera(data.camera));

                data.solid          = solid.Copy(allocator);
                data.transparent    = transparent.Copy(allocator);
			});

		return task;
	}


	/************************************************************************************************/


	void ReleaseSceneAnimation(AnimationClip* AC, iAllocator* Memory)
	{
		for (size_t II = 0; II < AC->FrameCount; ++II) 
		{
			Memory->free(AC->Frames[II].Joints);
			Memory->free(AC->Frames[II].Poses);
		}

		Memory->free(AC->Frames);
		Memory->free(AC->mID);
	}


	/************************************************************************************************/


	void BindJoint(GraphicScene* SM, JointHandle Joint, SceneEntityHandle Entity, NodeHandle TargetNode)
	{
		//SM->TaggedJoints.push_back({ Entity, Joint, TargetNode });
	}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, GUID_t Guid, GraphicScene& GS_out, iAllocator* allocator, iAllocator* temp)
	{
		bool Available = isAssetAvailable(Guid);
		if (Available)
		{
			auto RHandle = LoadGameAsset(Guid);
			auto R		 = GetAsset(RHandle);

			EXITSCOPE(FreeAsset(RHandle));

			if (R != nullptr) {
				SceneResourceBlob* sceneBlob = (SceneResourceBlob*)R;

				const auto blockCount = sceneBlob->blockCount;
				
				size_t						offset                  = 0;
				size_t						currentBlock            = 0;
				SceneNodeBlock*				nodeBlock               = nullptr;
				ComponentRequirementBlock*	componentRequirement    = nullptr;
				Vector<NodeHandle>			nodes{ temp };


				while (offset < sceneBlob->ResourceSize && currentBlock < blockCount)
				{
					SceneBlock* block = reinterpret_cast<SceneBlock*>(sceneBlob->Buffer + offset);
					switch (block->blockType)
					{
						case SceneBlockType::NodeTable:
						{
							// TODO: CRC Check
							// TODO: more error checking
							FK_ASSERT(nodeBlock == nullptr, "Multiple Node Blocks Defined!");
							nodeBlock = reinterpret_cast<SceneNodeBlock*>(block);

							const auto nodeCount = nodeBlock->header.nodeCount;
							nodes.reserve(nodeCount);


							for (size_t itr = 0; itr < nodeCount; ++itr)
							{
								float3		position;
								Quaternion	orientation;
								float3		scale;

								auto sceneNode = &nodeBlock->nodes[itr];
								memcpy(&position,		&sceneNode->position, sizeof(float3));
								memcpy(&orientation,	&sceneNode->orientation, sizeof(orientation));
								memcpy(&scale,			&sceneNode->scale, sizeof(float3));

								auto newNode = GetNewNode();
								SetOrientationL	(newNode, orientation);
								SetPositionL	(newNode, position);
								SetScale		(newNode, { scale[0], scale[0], scale[0] }); // Engine only supports uniform scaling, still stores all 3 for future stuff
								SetFlag			(newNode, SceneNodes::StateFlags::SCALE);

								if (sceneNode->parent != INVALIDHANDLE)
									SetParentNode(nodes[sceneNode->parent], newNode);

								nodes.push_back(newNode);
							}
						}	break;
						case SceneBlockType::ComponentRequirementTable:
						case SceneBlockType::Entity:
						{
							FK_ASSERT(nodeBlock != nullptr, "No Node Block defined!");
							//FK_ASSERT(componentRequirement != nullptr, "No Component Requirement Block defined!");

                            EntityBlock::Header entityBlock;
                            memcpy(&entityBlock, block, sizeof(entityBlock));
							auto& gameObject = allocator->allocate<GameObject>(allocator);

                            size_t itr                  = 0;
                            size_t componentOffset      = 0;
                            const size_t componentCount = entityBlock.componentCount;

                            while(true)
                            {
                                if (block->buffer + componentOffset >= reinterpret_cast<byte*>(block) + entityBlock.blockSize)
                                    break;

                                ComponentBlock::Header component;
                                memcpy(&component, (std::byte*)block + sizeof(entityBlock) + componentOffset, sizeof(component));

                                const ComponentID ID = component.componentID;
                                if (component.blockType != EntityComponent) // malformed blob?
                                    break;
                                else if (ID == SceneNodeView<>::GetComponentID())
                                {
                                    SceneNodeComponentBlob blob;
                                    memcpy(&blob, (std::byte*)block + sizeof(entityBlock) + componentOffset, sizeof(blob));

                                    auto node = nodes[blob.nodeIdx];
                                    gameObject.AddView<SceneNodeView<>>(node);

                                    if (!blob.excludeFromScene)
                                        GS_out.AddGameObject(gameObject, node);
                                }
                                else if (ComponentAvailability(ID) == true)
                                {
                                    GetComponent(ID).AddComponentView(
                                                        gameObject,
                                                        (std::byte*)(block) + sizeof(entityBlock) + componentOffset,
                                                        component.blockSize,
                                                        allocator);
                                }

                                ++itr;
                                componentOffset += component.blockSize;
                                if (itr >= componentCount)
                                    break;
                            }

                            SetBoundingSphereFromMesh(gameObject);
						}
						case SceneBlockType::EntityComponent:
							break;
					default:
						break;
					}
					currentBlock++;
					offset += block->blockSize;
				}


                UpdateTransforms();

				return true;
			}
		}


		return false;
	}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, const char* LevelName, GraphicScene& GS_out, iAllocator* allocator, iAllocator* Temp)
	{
		if (isAssetAvailable(LevelName))
		{
			auto RHandle = LoadGameAsset(LevelName);
			auto R = GetAsset(RHandle);

			FINALLY
			{
				FreeAsset(RHandle);
			}
			FINALLYOVER

			return LoadScene(RS, R->GUID, GS_out, allocator, Temp);
		}
		return false;
	}


	/************************************************************************************************/


	Vector<PointLightHandle> GraphicScene::FindPointLights(const Frustum &f, iAllocator* tempMemory) const
	{
		Vector<PointLightHandle> lights{tempMemory};

		auto& visables = SceneVisibilityComponent::GetComponent();

		for (auto entity : sceneEntities)
			Apply(*visables[entity].entity,
				[&](PointLightView&			pointLight,
					SceneVisibilityView&	visibility,
					SceneNodeView<>&		sceneNode)
				{
					const auto position	    = sceneNode.GetPosition();
					const auto scale		= sceneNode.GetScale();
					const auto radius		= pointLight.GetRadius();

					if (Intersects(f, BoundingSphere{ position, radius * scale }))
						lights.emplace_back(pointLight);
				});

		return lights;
	}


    /************************************************************************************************/


    BuildBVHTask& GraphicScene::UpdateSceneBVH(UpdateDispatcher& dispatcher, UpdateTask& transformDependency, iAllocator* allocator)
    {
        return dispatcher.Add<SceneBVHBuild>(
            [&](UpdateDispatcher::UpdateBuilder& builder, SceneBVHBuild& data)
			{
                builder.SetDebugString("BVH");
                builder.AddInput(transformDependency);
			},
			[this, allocator = allocator](SceneBVHBuild& data, iAllocator& threadAllocator)
			{
                FK_LOG_9("Build BVH");
                ProfileFunction();

                bvh         = bvh.Build(*this, threadAllocator).Copy(*allocator);
                data.bvh    = &bvh;
			}
		);
    }


    /************************************************************************************************/


    PointLightGatherTask& GraphicScene::GetPointLights(UpdateDispatcher& dispatcher, iAllocator* tempMemory) const
	{
		return dispatcher.Add<PointLightGather>(
			[&](UpdateDispatcher::UpdateBuilder& builder, PointLightGather& data)
			{
				data.pointLights	= Vector<PointLightHandle>{ tempMemory, 64 };
				data.scene			= this;

                builder.SetDebugString("Point Light Gather");
			},
			[this](PointLightGather& data, iAllocator& threadAllocator)
			{
                FK_LOG_9("Point Light Gather");
                ProfileFunction();

                Vector<PointLightHandle> tempList{ &threadAllocator, 64 };
			    auto& visables = SceneVisibilityComponent::GetComponent();

				for (auto entity : sceneEntities)
				{

					Apply(*visables[entity].entity,
						[&](PointLightView&         pointLight,
							SceneVisibilityView&    visibility)
						{
                            tempList.emplace_back(pointLight);
						});
				}

                data.pointLights = tempList;
			}
		);
	}


    /************************************************************************************************/


    PointLightShadowGatherTask& GraphicScene::GetVisableLights(UpdateDispatcher& dispatcher, CameraHandle camera, BuildBVHTask& bvh, iAllocator* temporaryMemory) const
    {
        return dispatcher.Add<PointLightShadowGather>(
			[&](UpdateDispatcher::UpdateBuilder& builder, PointLightShadowGather& data)
			{
                builder.SetDebugString("Point Light Shadow Gather");
                builder.AddInput(bvh);

                data.pointLightShadows = Vector<PointLightHandle>{ temporaryMemory };
			},
            [this, &bvh = bvh.GetData().bvh, camera = camera](PointLightShadowGather& data, iAllocator& threadAllocator)
			{
                FK_LOG_9("Point Light Shadow Gather");
                ProfileFunction();

                Vector<PointLightHandle> visablePointLights{ &threadAllocator };
                auto& visabilityComponent = SceneVisibilityComponent::GetComponent();

                const auto frustum = GetFrustum(camera);

                auto& lights = PointLightComponent::GetComponent();


                if constexpr (false)
                {
                    for (auto& light : lights)
                        visablePointLights.push_back(light.handle);
                }
                else
                {
                    bvh->TraverseBVH(frustum,
                        [&](VisibilityHandle intersector)
                        {
                            GameObject& go = *visabilityComponent[intersector].entity;
                            Apply(go,
                                [&](PointLightView& light)
                                {
                                    visablePointLights.push_back(light);
                                }
                            );
                        });
                }
                data.pointLightShadows = visablePointLights;
			}
		);
    }


	/************************************************************************************************/


    PointLightUpdate& GraphicScene::UpdatePointLights(UpdateDispatcher& dispatcher, BuildBVHTask& bvh, PointLightShadowGatherTask& visablePointLights, iAllocator* temporaryMemory, iAllocator* persistentMemory) const
    {
        class Task : public iWork
        {
        public:
            Task(PointLightHandle IN_light, SceneBVH& IN_bvh, iAllocator& IN_allocator) :
                iWork           {},
                persistentMemory{ IN_allocator },
                lightHandle     { IN_light },
                bvh             { IN_bvh }
            {
                _debugID = "PointLightUpdate_Task";
            }

            void Run(iAllocator& threadLocalAllocator)
            {
                ProfileFunction();

                auto& visables  = SceneVisibilityComponent::GetComponent();
                auto& lights    = PointLightComponent::GetComponent();

                auto& light         = lights[lightHandle];
                const float3 POS    = GetPositionW(light.Position);
                const float  r      = light.R;
                AABB lightAABB{ POS - r, POS + r };

                Vector<VisibilityHandle> PVS{ &threadLocalAllocator };

                bvh.TraverseBVH(lightAABB,
                    [&](VisibilityHandle visable)
                    {
                        PVS.push_back(visable);
                    });

                if(const auto flags = GetFlags(light.Position); flags & (SceneNodes::DIRTY | SceneNodes::UPDATED))
                    light.state = LightStateFlags::Dirty;

                std::sort(
                    std::begin(PVS), std::end(PVS),
                    [](const auto& lhs, const auto& rhs)
                    {
                        return lhs < rhs;
                    });

                auto markDirty =
                    [&]
                    {
                        light.state = LightStateFlags::Dirty;
                        light.shadowState->visableObjects = PVS;
                    };


                if (light.shadowState)
                {   // Compare previousPVS with current PVS to see if shadow map needs update
                    auto& previousPVS = light.shadowState->visableObjects;

                    if (previousPVS.size() == PVS.size())
                    {
                        for (size_t I = 0; I < PVS.size(); ++I)
                        {
                            if (PVS[I] != previousPVS[I])
                            {
                                markDirty();
                                break; // do full update
                            }
                            const auto flags = GetFlags(visables[PVS[I]].node);

                            if (flags & (SceneNodes::UPDATED | SceneNodes::DIRTY))
                            {
                                markDirty();
                                return;
                            }
                        }
                    }
                    else
                    {
                        markDirty();
                        return;
                    }

                    return;
                }

                if (!light.shadowState)
                    light.shadowState = &persistentMemory.allocate<CubeMapState>(&persistentMemory);

                light.state = LightStateFlags::Dirty;
                light.shadowState->shadowMapSize = 128;
                light.shadowState->visableObjects = PVS;
            }

            void Release()
            {

            }

            iAllocator&             persistentMemory;
            SceneBVH&               bvh;
            const PointLightHandle  lightHandle = InvalidHandle_t;
        };

        return dispatcher.Add<PointLightUpdate_DATA>(
			[&](UpdateDispatcher::UpdateBuilder& builder, PointLightUpdate_DATA& data)
			{
                builder.SetDebugString("Point Light Shadow Gather");
                builder.AddInput(bvh);
                builder.AddInput(visablePointLights);

                data.dirtyList = Vector<PointLightHandle>{ temporaryMemory };
			},
            [   this,
                &bvh                = bvh.GetData().bvh,
                &visablePointLights = visablePointLights.GetData().pointLightShadows,
                persistentMemory    = persistentMemory,
                &threads            = *dispatcher.threads
            ]
            (PointLightUpdate_DATA& data, iAllocator& threadAllocator)
			{
                WorkBarrier     barrier{ threads, &threadAllocator };
                Vector<Task*>   taskList{ &threadAllocator, visablePointLights.size() };

                for (auto visableLight : visablePointLights)
                {
                    auto& task = threadAllocator.allocate<Task>(visableLight, *bvh, *persistentMemory);
                    barrier.AddWork(task);
                    taskList.push_back(&task);
                }

                for (auto& task : taskList)
                    threads.AddWork(task);

                barrier.Join();

                auto& lights = PointLightComponent::GetComponent();
                for (auto visableLight : visablePointLights)
                {
                    auto& light = lights[visableLight];
                    if (lights[visableLight].state == LightStateFlags::Dirty && light.shadowState)
                        data.dirtyList.push_back(visableLight);
                }
			}
		);
    }


    /************************************************************************************************/


	size_t	GraphicScene::GetPointLightCount()
	{
		auto& visables		= SceneVisibilityComponent::GetComponent();
		size_t lightCount	= 0;

		for (auto entity : sceneEntities)
			Apply(*visables[entity].entity,
				[&](PointLightView& pointLight) { lightCount++; });

		return lightCount;
	}


	/************************************************************************************************/


	Drawable::VConstantsLayout Drawable::GetConstants() const
	{
		DirectX::XMMATRIX WT;
		FlexKit::GetTransform(Node, &WT);

		Drawable::VConstantsLayout	Constants;

		Constants.MP        = MatProperties;
		Constants.Transform = XMMatrixToFloat4x4(WT).Transpose();

        if (material != InvalidHandle_t)
        {
            auto textures           = MaterialComponent::GetComponent()[material].Textures;
            Constants.textureCount  = (uint32_t)textures.size();

            for (auto& texture : textures)
                Constants.textureHandles[std::distance(std::begin(textures), &texture)] = uint4{ 256, 256, texture.to_uint() };
        }
        else
            Constants.textureCount = 0;

		return Constants;
	}


	/************************************************************************************************/


	void Release(PoseState* EPS, iAllocator* allocator)
	{
		if (EPS->Joints)		allocator->free(EPS->Joints);
		if (EPS->CurrentPose)	allocator->free(EPS->CurrentPose);

		EPS->Joints = nullptr;
		EPS->CurrentPose = nullptr;
	}


}	/************************************************************************************************/
