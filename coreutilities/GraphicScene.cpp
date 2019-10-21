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

#include "GraphicScene.h"
#include "GraphicsComponents.h"
#include "intersection.h"

#include "..\coreutilities\Components.h"
#include "..\coreutilities\componentBlobs.h"
#include "..\graphicsutilities\AnimationRuntimeUtilities.H"

namespace FlexKit
{
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
            triMesh = LoadTriMeshIntoTable(renderSystem, drawableComponent.resourceID);

        gameObject.AddView<DrawableView>(triMesh, node);
        SetBoundingSphereFromMesh(gameObject);
    }


    /************************************************************************************************/

    
    void PointLightEventHandler::OnCreateView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
    {
        PointLightComponentBlob pointLight;

        memcpy(&pointLight, buffer, sizeof(pointLight));

        gameObject.AddView<PointLightView>(pointLight.K, pointLight.IR[0], GetSceneNode(gameObject));
        SetBoundingSphereFromLight(gameObject);
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

		//sceneManagement.AddEntity(vis);
	}


	/************************************************************************************************/



	void GraphicScene::RemoveEntity(GameObject& go)
	{
		Apply(go, 
		[&, allocator = this->allocator](SceneVisibilityView& vis) 
		{
			//sceneManagement.RemoveEntity(*vis);
			go.RemoveView(vis);

			auto handle = vis.visibility;
			sceneEntities.remove_unstable(
				find(sceneEntities, [&](auto i) { return i == handle; }));

			allocator->release(&vis);
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

			allocator->release(visable);
		}

		sceneEntities.clear();
		sceneManagement.clear();
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
		auto Resource = GetResource(RHndl);
		if (Resource->Type == EResourceType::EResource_SkeletalAnimation)
		{
			auto AC = Resource2AnimationClip(Resource, GS->Memory);
			FreeResource(RHndl);// No longer in memory once loaded

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
				if (isResourceAvailable(Guid))
				{
					auto RHndl = LoadGameResource(Guid);
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
		if(isResourceAvailable(Animation))
		{
			auto RHndl = LoadGameResource(Animation);
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


	void UpdateQuadTree(QuadTreeNode* Node, GraphicScene* Scene)
	{
		/*
		for (const auto D : Scene->Drawables) {
			if (GetFlag(D.Node, SceneNodes::StateFlags::UPDATED))
			{
				if()
				{	// Moved Branch
					if()
					{	// Space in Node Available
					}
					else
					{	// Node Split Needed
					}
				}
			}
		}
		*/
	}


	/************************************************************************************************/


	void QuadTreeNode::ExpandNode(iAllocator* allocator)
	{
		ChildNodes.push_back(&allocator->allocate_aligned<QuadTreeNode>(allocator));
		ChildNodes.push_back(&allocator->allocate_aligned<QuadTreeNode>(allocator));
		ChildNodes.push_back(&allocator->allocate_aligned<QuadTreeNode>(allocator));
		ChildNodes.push_back(&allocator->allocate_aligned<QuadTreeNode>(allocator));

		const auto SpanVector	= upperRight - lowerLeft;
		const auto CenterPoint	= (lowerLeft + upperRight) / 2;

		ChildNodes[(int)QuadTreeNodeLocation::UpperRight]->upperRight	= CenterPoint + SpanVector / 2;
		ChildNodes[(int)QuadTreeNodeLocation::UpperRight]->lowerLeft	= CenterPoint;

		ChildNodes[(int)QuadTreeNodeLocation::UpperLeft]->upperRight	= CenterPoint + (SpanVector / 2) * float2( 0.0f,  1.0f);
		ChildNodes[(int)QuadTreeNodeLocation::UpperLeft]->lowerLeft		= CenterPoint + (SpanVector / 2) * float2(-1.0f,  0.0f);

		ChildNodes[(int)QuadTreeNodeLocation::LowerLeft]->upperRight	= CenterPoint;
		ChildNodes[(int)QuadTreeNodeLocation::LowerLeft]->lowerLeft		= CenterPoint - (SpanVector / 2);

		ChildNodes[(int)QuadTreeNodeLocation::LowerRight]->upperRight	= CenterPoint + (SpanVector / 2) * float2( 1.0f,  0.0f);
		ChildNodes[(int)QuadTreeNodeLocation::LowerRight]->lowerLeft	= CenterPoint + (SpanVector / 2) * float2( 0.0f, -1.0f);

		for (auto& node : ChildNodes)
			node->centerPoint = (node->upperRight + node->lowerLeft) / 2;
	}


	/************************************************************************************************/


	float4 QuadTreeNode::GetArea() const
	{
		const auto areaLL = (lowerLeft.y <  upperRight.y) ? lowerLeft : upperRight;
		const auto areaUR = (lowerLeft.y >= upperRight.y) ? lowerLeft : upperRight;

		return { areaLL, areaUR };
	}


	/************************************************************************************************/


	void QuadTree::clear()
	{
		root.Clear();
	}


	/************************************************************************************************/


	void QuadTreeNode::AddEntity(VisibilityHandle visable)
	{
		if (ChildNodes.empty() && !Contents.full())
		{
			Contents.push_back(visable);
		}
		else
		{
			if (ChildNodes.empty()) 
			{
				ExpandNode(allocator);

				for (auto C : Contents)
					AddEntity(visable);

				Contents.clear();
			}

			// Get Nearest Node
			auto visableEntity	= SceneVisibilityComponent::GetComponent()[visable];
			auto node			= visableEntity.node;
			auto position		= GetPositionW(node);

			float2 pos_f2{position.x, position.z};

			float			l = 1000000000.0f;
			QuadTreeNode*	nearestNode = ChildNodes.front();

			for (auto child : ChildNodes)
			{
				float d = (child->centerPoint - pos_f2).Magnitude();
				if (d < l)
				{
					l = d;
					nearestNode = child;
				}
			}

			nearestNode->AddEntity(visable);
		}
	}


	/************************************************************************************************/


	QuadTreeNode::SphereTestRes QuadTreeNode::IsSphereInside(float r, float3 pos)
	{
		float nodeSpan			= (lowerLeft - upperRight).Magnitude();
		float l					= (float2(pos.x, pos.z) - centerPoint).Magnitude();

		bool OutSideNode		= (l > (nodeSpan / 2));

		return OutSideNode ? Failed : Fully;
	}

	/************************************************************************************************/

	void QuadTreeNode::UpdateBounds()
	{
		for (auto child : ChildNodes) 
		{
			child->UpdateBounds();

			upperRight.x = (child->upperRight.x > upperRight.x) ? child->upperRight.x : upperRight.x;
			upperRight.y = (child->upperRight.y > upperRight.y) ? child->upperRight.y : upperRight.y;

			lowerLeft.x = (child->lowerLeft.x > lowerLeft.x) ? child->lowerLeft.x : lowerLeft.x;
			lowerLeft.y = (child->lowerLeft.y > lowerLeft.y) ? child->lowerLeft.y : lowerLeft.y;
		}

		auto& visibility = SceneVisibilityComponent::GetComponent();

		for(auto visable : Contents)
		{
			auto& visableInfo	= visibility[visable];
			auto node			= visableInfo.node;
			auto boundingSphere	= visableInfo.boundingSphere;
			auto r				= boundingSphere.w;
			auto offset			= boundingSphere.xyz();
			auto position		= GetPositionW(node);

			float2 vectorToNode				= centerPoint - float2{position.x + offset.x, position.z + offset.x };
			
			float2 vectorToNodeNormalized	= vectorToNode / vectorToNode.Magnitude();
			
			if ((vectorToNode * vectorToNode).Sum() < 0.00001f)
				vectorToNodeNormalized = float2{ 0, 0 };

			float2 newUR					= centerPoint + vectorToNodeNormalized * -r;

			upperRight.x = (newUR.y > upperRight.y) ? newUR.x : upperRight.x;
			upperRight.y = (newUR.y > upperRight.y) ? newUR.y : upperRight.y;

			lowerLeft.x = (newUR.y < lowerLeft.y) ? newUR.x : lowerLeft.x;
			lowerLeft.y = (newUR.y < lowerLeft.y) ? newUR.y : lowerLeft.y;
		}
	}

	/************************************************************************************************/


	void QuadTree::AddEntity(VisibilityHandle entityHandle)
	{
		root.AddEntity(entityHandle);
		//UpdateBounds();
	}


	/************************************************************************************************/


	void QuadTree::RemoveEntity(VisibilityHandle handle)
	{

	}


	/************************************************************************************************/


	void QuadTree::Rebuild(GraphicScene& parentScene) 
	{
		RebuildCounter = 0;
		root.Clear();

		float2 lowerLeft	{ 0, 0 };
		float2 upperRight	{ 0, 0 };
			
		//for (auto item : allEntities)
		{
			//auto boundingSphere = parentScene.GetBoundingSphere(item);
			//float r = boundingSphere.w;

			//upperRight.x = boundingSphere.x + r > upperRight.x ? boundingSphere.x + r : upperRight.x;
			//upperRight.y = boundingSphere.z + r > upperRight.y ? boundingSphere.z + r : upperRight.y;

			//lowerLeft.x = boundingSphere.x - r < lowerLeft.x ? boundingSphere.x - r : lowerLeft.x;
			//lowerLeft.y = boundingSphere.z - r < lowerLeft.y ? boundingSphere.z - r : lowerLeft.y;
		}

		auto centerPoint	= (upperRight - lowerLeft) / 2;
		root.centerPoint	= centerPoint;
		root.lowerLeft		= lowerLeft;
		root.upperRight		= upperRight;

		//for (auto item : allEntities)
		//	root.AddChild(item, allocator);
	}


	/************************************************************************************************/


	float4	QuadTree::GetArea()
	{
		return root.GetArea();
	}


	/************************************************************************************************/


	UpdateTask& QuadTree::Update(FlexKit::UpdateDispatcher& dispatcher, GraphicScene* parentScene, UpdateTask& transformDependency)
	{
		struct QuadTreeUpdate
		{
			QuadTree*		QTree;
			GraphicScene*	parentScene;
		};

		auto& task = dispatcher.Add<QuadTreeUpdate>(
			[&](DependencyBuilder& builder, auto& QuadTreeUpdate)
			{
				builder.AddInput(transformDependency);
				builder.SetDebugString("QuadTree Update!");

				QuadTreeUpdate.QTree		= this;
				QuadTreeUpdate.parentScene	= parentScene;

			},
			[](auto& QuadTreeUpdate)
			{
				FK_LOG_INFO("QuadTree::Update");

				const auto period  = QuadTreeUpdate.QTree->RebuildPeriod;
				const auto counter = QuadTreeUpdate.QTree->RebuildCounter;

				//if (period <= counter)
				//	QuadTreeUpdate.QTree->Rebuild(*QuadTreeUpdate.parentScene);
				//else
				//	QuadTreeUpdate.QTree->root.UpdateBounds(*QuadTreeUpdate.parentScene);

				//QuadTreeUpdate.QTree->RebuildCounter++;
			});

		return task;
	}


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


	void UpdateShadowCasters(GraphicScene* GS)
	{
		//for (auto& Caster : GS->SpotLightCasters)
		//{
		//	UpdateCamera(GS->RS, *GS->SN, &Caster.C, 0.00f);
		//}
	}


	/************************************************************************************************/


	void GatherScene(GraphicScene* SM, CameraHandle Camera, PVS* __restrict out, PVS* __restrict T_out)
	{
		FK_ASSERT(out		!= T_out);
		FK_ASSERT(Camera	!= CameraHandle{(unsigned int)INVALIDHANDLE});
		FK_ASSERT(SM		!= nullptr);
		FK_ASSERT(out		!= nullptr);
		FK_ASSERT(T_out		!= nullptr);


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
					[&](DrawableView& drawable)
					{
						if (CompareBSAgainstFrustum(&F, BS))
						{
							float distance = 0;
							if (potentialVisible.transparent)
								PushPV(drawable, T_out);
							else
								PushPV(drawable, out);
						}
					});
			}
		}
	}


	/************************************************************************************************/


	auto& GatherScene(UpdateDispatcher& dispatcher, GraphicScene* scene, CameraHandle C, iAllocator* allocator)
	{
		auto& task = dispatcher.Add<GetPVSTaskData>(
			[&](auto& builder, auto& data)
			{
				size_t taskMemorySize = KILOBYTE * 2048;
				data.taskMemory.Init((byte*)allocator->malloc(taskMemorySize), taskMemorySize);
				data.scene			= scene;
				data.solid			= PVS{ data.taskMemory };
				data.transparent	= PVS{ data.taskMemory };
				data.camera			= C;
			},
			[](auto& data)
			{
				FK_LOG_9("Start PVS gather\n");

                GatherScene(data.scene, data.camera, &data.solid, &data.transparent);
				SortPVS(&data.solid, &CameraComponent::GetComponent().GetCamera(data.camera));

				FK_LOG_9("End PVS gather\n");
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
		bool Available = isResourceAvailable(Guid);
		if (Available)
		{
			auto RHandle = LoadGameResource(Guid);
			auto R		 = GetResource(RHandle);

			EXITSCOPE(FreeResource(RHandle));

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

								if (sceneNode->parent == INVALIDHANDLE) // remove root scaling
									scale = scale / 100;

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
						}
						case SceneBlockType::EntityComponent:
							break;
					default:
						break;
					}
					currentBlock++;
					offset += block->blockSize;
				}


				return true;
			}
		}


		return false;
	}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, const char* LevelName, GraphicScene& GS_out, iAllocator* allocator, iAllocator* Temp)
	{
		if (isResourceAvailable(LevelName))
		{
			auto RHandle = LoadGameResource(LevelName);
			auto R = GetResource(RHandle);

			FINALLY
			{
				FreeResource(RHandle);
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
					auto position	= sceneNode.GetPosition();
					auto scale		= sceneNode.GetScale();
					auto radius		= pointLight.GetRadius();

					if (CompareBSAgainstFrustum(&f, { position, radius * scale }))
						lights.emplace_back(pointLight);

					//visibility->GetBoundingVolume();
					//BoundingSphere BoundingVolume = float4(Pw, light.R * Ps);
				});

		return lights;
	}


	/************************************************************************************************/


	auto& GraphicScene::GetPointLights(UpdateDispatcher& dispatcher, iAllocator* tempMemory)
	{
		return dispatcher.Add<PointLightGather>(
			[&](auto& builder, PointLightGather& data)
			{
				data.temp			= StackAllocator(tempMemory, KILOBYTE * 16);
				data.pointLights	= Vector<PointLightHandle>{ data.temp, 1024 };
				data.scene			= this;
			},
			[this](PointLightGather& data)
			{
				for (auto entity : sceneEntities)
				{
					auto& visables = SceneVisibilityComponent::GetComponent();

					Apply(*visables[entity].entity,
						[&](PointLightView& pointLight,
							SceneVisibilityView& visibility,
							SceneNodeView<>& sceneNode)
						{
							data.pointLights.emplace_back(pointLight);
						});
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


	Drawable::VConsantsLayout Drawable::GetConstants()
	{
		DirectX::XMMATRIX WT;
		FlexKit::GetTransform(Node, &WT);

		Drawable::VConsantsLayout	Constants;

		Constants.MP.Albedo = MatProperties.Albedo;
		Constants.MP.Spec	= MatProperties.Spec;
		Constants.Transform = DirectX::XMMatrixTranspose(WT);

		return Constants;
	}


	/************************************************************************************************/


	void Release(DrawablePoseState* EPS, iAllocator* allocator)
	{
		if (EPS->Joints)			allocator->free(EPS->Joints);
		if (EPS->CurrentPose)	allocator->free(EPS->CurrentPose);

		EPS->Joints = nullptr;
		EPS->CurrentPose = nullptr;
	}

}	/************************************************************************************************/
