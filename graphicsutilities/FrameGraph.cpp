#include "graphics.h"
#include "FrameGraph.h"
#include "..\coreutilities\Logging.h"

namespace FlexKit
{	/************************************************************************************************/


	ResourceTransition::ResourceTransition(FrameObjectDependency& Dep) :
		Object		{ Dep.FO },
		BeforeState	{ Dep.ExpectedState },
		AfterState	{ Dep.State}{}


	/************************************************************************************************/


	void ResourceTransition::ProcessTransition(FrameResources& Resources, Context* ctx) const
	{
		switch (Object->Type)
		{
		case OT_StreamOut:
		{
			switch (AfterState)
			{
			case DRS_VERTEXBUFFER:
			case DRS_STREAMOUT: {
				ctx->AddStreamOutBarrier(
					Object->SOBuffer,
					BeforeState,
					AfterState);
			}	break;
			}
		}	break;
		default:
			switch (AfterState)
			{
			case DRS_Present:
			{
				ctx->AddPresentBarrier(
					Resources.GetRenderTarget(Object->Handle),
					BeforeState);
			}	break;
			case DRS_DEPTHBUFFERWRITE:
			case DRS_RenderTarget:
				ctx->AddRenderTargetBarrier(
					Resources.GetRenderTarget(Object->Handle),
					BeforeState,
					AfterState);
				break;
			case DRS_UAV:
				ctx->AddUAVBarrier(
					Object->UAVBuffer,
					BeforeState,
					AfterState);
				break;
			case DRS_STREAMOUT:
			case DRS_ShaderResource:
			case DRS_CONSTANTBUFFER:
			case DRS_PREDICATE:
			case DRS_INDIRECTARGS:
			case DRS_UNKNOWN:
			default:
				FK_ASSERT(0); // Unimplemented
			}

		}


	}


	/************************************************************************************************/


	void FrameGraphNode::HandleBarriers(FrameResources& Resources, Context* Ctx)
	{
		for (const auto& T : Transitions) {
			T.ProcessTransition(Resources, Ctx);

			auto stateObject	= *T.Object;
			stateObject.State = T.AfterState;
			auto idx			= Resources.SubNodeTracking.push_back(stateObject);
		}
	}


	/************************************************************************************************/


	void FrameGraphNode::AddTransition(FrameObjectDependency& Dep)
	{
		Transitions.push_back(Dep);
	}


	/************************************************************************************************/


	bool FrameGraphNode::DependsOn(uint32_t Tag)
	{
		auto Pred = [&](const auto& RHS) -> bool {	return RHS.FO->Tag == Tag;	};

		return (find(InputObjects, Pred) != InputObjects.end());
	}


	/************************************************************************************************/


	bool FrameGraphNode::Outputs(uint32_t Tag)
	{
		auto Pred = [&](const auto& RHS) -> bool {	return RHS.FO->Tag == Tag;	};

		return (find(OutputObjects, Pred) != OutputObjects.end());
	}


	/************************************************************************************************/


	void FrameGraphNode::RestoreResourceStates(Context* ctx, PassObjectList& locallyTrackedObjects)
	{
		for (auto& resNode : locallyTrackedObjects)
		{
			auto res = find(OutputObjects,
				[&](FrameObjectDependency& rhs) -> bool
				{
					return rhs.FO->Handle == resNode.Handle;
				});

			if (res!= OutputObjects.end() && res->State != resNode.State)
			{
				switch (resNode.Type)
				{
				case FrameObjectResourceType::OT_StreamOut:
					ctx->AddStreamOutBarrier(resNode.SOBuffer, resNode.State, res->State);
					break;
				case FrameObjectResourceType::OT_UAVBuffer:
				case FrameObjectResourceType::OT_UnorderedAccessView:
					ctx->AddUAVBarrier(resNode.UAVBuffer, resNode.State, res->State);
					break;
				case FrameObjectResourceType::OT_BackBuffer:
				case FrameObjectResourceType::OT_ConstantBuffer:
				case FrameObjectResourceType::OT_DepthBuffer:
				case FrameObjectResourceType::OT_IndirectArguments:
				case FrameObjectResourceType::OT_PVS:
				case FrameObjectResourceType::OT_RenderTarget:
				case FrameObjectResourceType::OT_Texture:
				case FrameObjectResourceType::OT_UAVTexture:
				case FrameObjectResourceType::OT_VertexBuffer:
					FK_ASSERT(0, "UN-IMPLEMENTED BLOCK!");
				}
			}
		}

		locallyTrackedObjects.clear();

		return;
	}


	/************************************************************************************************/


	void FrameGraph::AddRenderTarget(TextureHandle Texture)
	{

	}


	/************************************************************************************************/

	
	void FrameGraph::UpdateFrameGraph(RenderSystem* RS, RenderWindow* Window, iAllocator* Temp)
	{
		RS->SubmitUploadQueues();
		RS->BeginSubmission(Window);
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::BuildNode(FrameGraph* FrameGraph)
	{	// Builds Nodes Linkages, Transitions
		auto CheckNodeSource = [](FrameGraphNode& Node, FrameObjectDependency& Resource)
		{
			if (//Input.SourceObject != &Node &&
				Resource.Source&&
				!IsXInSet(Resource.Source, Node.Sources))
			{
				Node.Sources.push_back(Resource.Source);
			}
		};

		/*
		for (auto& Object : LocalInputs)
		{
			if (!Object.Source)
				Object.Source= &Node;

			auto Res = CheckResourceSituation(
						Context.Writables, 
						Context.Readables, 
						Object);

			switch (Res)
			{
			case CheckStateRes::TransitionNeeded:
				Transitions.push_back(Object);
				break;
			case CheckStateRes::CorrectState:
				CheckNodeSource(Node, Object);
				break;
			case CheckStateRes::ResourceNotTracked:
			{	// Begin Tracking Resource
				auto ResState = Object.FO->State;	

				Transitions.push_back(
					FrameObjectDependency(
						Object.FO,
						&Node,
						ResState,
						Object.State));
			}	break;
			case CheckStateRes::Error:
				FK_ASSERT(0);
				break;
			}
		}

		for (auto& Object : LocalOutputs)
		{
			Object.Source = &Node;
			auto Res = CheckResourceSituation(
						Context.Readables, 
						Context.Writables, 
						Object);

			switch (Res)
			{
			case CheckStateRes::TransitionNeeded:
				Transitions.push_back(Object);
				break;
			case CheckStateRes::CorrectState:
				CheckNodeSource(Node, Object);
				break;
			case CheckStateRes::ResourceNotTracked:
			{	// Begin Tracking Resource
				auto ResState = Object.FO->State;	
				Object.Source = &Node;

				Transitions.push_back(
					FrameObjectDependency(
						Object.FO,
						&Node,
						ResState,
						Object.State));
			}	break;
			case CheckStateRes::Error:
				FK_ASSERT(0);
				break;
			}
		}

		*/

		// Process Transitions
		for (auto& Object : Transitions)
		{
			CheckNodeSource(Node, Object);
			Object.Source = &Node;

			if(Object.State == DRS_Free)
				Context.Retire(Object);
			else
				if(Object.ExpectedState != Object.State)
					Node.AddTransition(Object);
		}

		for (auto& SourceNode : LocalInputs)
			CheckNodeSource(Node, SourceNode);

		Node.InputObjects	= std::move(LocalInputs);
		Node.OutputObjects	= std::move(LocalOutputs);
	}


	/************************************************************************************************/
	
	
	StaticFrameResourceHandle FrameGraphNodeBuilder::ReadTexture(uint32_t Tag, TextureHandle Texture)
	{
		FrameResourceHandle Handle_Out{ 0xffff };
		auto State = Resources->renderSystem->Textures.GetState(Texture);

		if (!(State | DRS_Read))
		{	// Auto Barrier Insertion
			FK_ASSERT(0);
			//Context.AddReadable({});
		}

		auto Index = Resources->renderSystem->GetTextureFrameGraphIndex(Texture);
		if (Index != INVALIDHANDLE)
		{
			size_t Index = 0;
			Resources->renderSystem->SetTextureFrameGraphIndex(Texture, Index);
			Handle_Out = FrameResourceHandle{ static_cast<unsigned int>(Index) };
		}
		else
		{
			Handle_Out = FrameResourceHandle{ static_cast<unsigned int>(Resources->Textures.size()) };
			Resources->Textures.push_back(FrameObject::TextureObject(Tag, Texture));
		}

		return Handle_Out;
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::ReadRenderTarget(
			uint32_t Tag, 
			RenderTargetFormat Format)
	{
		return AddReadableResource(Tag, DeviceResourceState::DRS_ShaderResource);
	}


	FrameResourceHandle FrameGraphNodeBuilder::ReadRenderTarget(TextureHandle RenderTarget)
	{
		return ReadRenderTarget(Resources->renderSystem->GetTag(RenderTarget));
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteRenderTarget(
		uint32_t Tag, 
		RenderTargetFormat Format)
	{
		return AddWriteableResource(Tag, DeviceResourceState::DRS_RenderTarget);
	}

	FrameResourceHandle FrameGraphNodeBuilder::WriteRenderTarget(TextureHandle RenderTarget)
	{
		return WriteRenderTarget(Resources->renderSystem->GetTag(RenderTarget));
	}



	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::PresentBackBuffer(uint32_t Tag)
	{
		return AddReadableResource(Tag, DeviceResourceState::DRS_Present);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteBackBuffer(uint32_t Tag)
	{
		return AddWriteableResource(Tag, DeviceResourceState::DRS_RenderTarget);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::ReadBackBuffer(uint32_t Tag)
	{
		FrameResourceHandle Resource = Resources->FindFrameResource(Tag);
		LocalInputs.push_back(FrameObjectDependency{
			Resources->GetResourceObject(Resource),
			nullptr,
			Resources->GetResourceObjectState(Resource),
			DeviceResourceState::DRS_ShaderResource });

		return Resource;
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::ReadDepthBuffer(uint32_t Tag)
	{
		FrameResourceHandle Resource = Resources->FindFrameResource(Tag);
		LocalInputs.push_back(FrameObjectDependency{
			Resources->GetResourceObject(Resource),
			nullptr,
			Resources->GetResourceObjectState(Resource),
			DeviceResourceState::DRS_DEPTHBUFFERWRITE });

		return Resource;
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::WriteDepthBuffer(uint32_t Tag)
	{
		return AddWriteableResource(Tag, DeviceResourceState::DRS_DEPTHBUFFERWRITE);
	}


	/************************************************************************************************/

	/*
	FrameResourceHandle	FrameGraphNodeBuilder::ReadDepthBuffer(TextureHandle Handle)
	{
		auto Resource = Resources->FindRenderTargetResource(Handle);
		LocalInputs.push_back(FrameObjectDependency{
			Resources->GetResourceObject(Resource),
			nullptr,
			Resources->GetResourceObjectState(Resource),
			DeviceResourceState::DRS_DEPTHBUFFERWRITE });

		return FrameResourceHandle(INVALIDHANDLE);
	}
	*/

	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::WriteDepthBuffer(TextureHandle handle)
	{
		return AddWriteableResource(handle, DeviceResourceState::DRS_DEPTHBUFFERWRITE, FrameObjectResourceType::OT_DepthBuffer);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::ReadWriteUAVBuffer(UAVResourceHandle handle)
	{
		return AddWriteableResource(handle, DeviceResourceState::DRS_Write);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::ReadSOBuffer(SOResourceHandle handle)
	{
		return AddReadableResource(handle, DeviceResourceState::DRS_VERTEXBUFFER);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::WriteSOBuffer(SOResourceHandle handle)
	{
		return AddWriteableResource(handle, DeviceResourceState::DRS_STREAMOUT);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AddWriteableResource(TextureHandle handle, DeviceResourceState state, FrameObjectResourceType type)
	{
		bool TrackedReadable = Context.IsTrackedReadable	(handle, type);
		bool TrackedWritable = Context.IsTrackedWriteable	(handle, type);

		if (!TrackedReadable && !TrackedWritable)
		{
			FrameResourceHandle Resource = Resources->FindFrameResource(handle);

			LocalOutputs.push_back(
				FrameObjectDependency{
					Resources->GetResourceObject(Resource),
					nullptr,
					Resources->GetResourceObjectState(Resource),
					state });

			Context.AddWriteable(LocalOutputs.back());
			Transitions.push_back(LocalOutputs.back());

			return Resource;
		}
		else
		{
			FK_ASSERT(0);
			/*
			auto Object = TrackedReadable ?
				Context.GetReadable(Tag) : Context.GetWriteable(Tag);
			Object.ExpectedState = Object.State;
			Object.State = State;
			LocalOutputs.push_back(Object);
			if (TrackedReadable) {
				Context.RemoveReadable(Tag);
				Context.AddWriteable(Object);
				Transitions.push_back(Object);
			}
			return Object.FO->Handle;
			*/
		}

		return FrameResourceHandle(0);
	}


	/************************************************************************************************/


	size_t FrameGraphNodeBuilder::GetDescriptorTableSize(PSOHandle State, size_t idx) const
	{
		auto rootSig	= Resources->renderSystem->GetPSORootSignature(State);
		auto tableSize	= rootSig->GetDesciptorTableSize(idx);

		return tableSize;
	}


	const DesciptorHeapLayout<16>&	FrameGraphNodeBuilder::GetDescriptorTableLayout(PSOHandle State, size_t idx) const
	{
		auto rootSig = Resources->renderSystem->GetPSORootSignature(State);
		return rootSig->GetDescHeap(idx);
	}


	/************************************************************************************************/


	DescriptorHeap	FrameGraphNodeBuilder::ReserveDescriptorTableSpaces(const DesciptorHeapLayout<16>& IN_layout, size_t requiredTables, iAllocator* tempMemory)
	{
		DescriptorHeap newHeap;
		newHeap.Init(
			Resources->renderSystem,
			IN_layout,
			requiredTables,
			tempMemory);


		return newHeap;
	}


	/************************************************************************************************/


	FrameGraphNodeBuilder::CheckStateRes FrameGraphNodeBuilder::CheckResourceSituation(
		Vector<FrameObjectDependency>&  Set1,
		Vector<FrameObjectDependency>&  Set2,
		FrameObjectDependency&			Object)
	{
		auto Pred = [&](auto& lhs)
		{	return (lhs.Tag == Object.Tag);	};

		{
			auto Res = find(Set1, Pred);
			if (Res != Set1.end())
			{
				return CheckStateRes::TransitionNeeded;
			}
		}
		{
			auto Res = find(Set2, Pred);
			if (Res == Set2.end())
			{
				return CheckStateRes::ResourceNotTracked;
			}
			return CheckStateRes::CorrectState;
		}

		return CheckStateRes::Error;
	}


	/************************************************************************************************/


	void FrameGraph::ProcessNode(FrameGraphNode* N, FrameResources& Resources, Context& Context)
	{
		if (!N->Executed)
		{
			N->Executed = true;

			for (auto Source : N->Sources)
				ProcessNode(Source, Resources, Context);

			N->NodeAction(Resources, &Context);
		}
	}


	/************************************************************************************************/


	void FrameGraph::SubmitFrameGraph(RenderSystem* RS, RenderWindow* RenderWindow)
	{
		Vector<Context>	Contexts(Memory);
		auto CL = RS->_GetCurrentCommandList();

		Contexts.emplace_back(Context(CL, RS, Memory));
		ReadyResources();

		for (auto& N : Nodes) 
		{
			ProcessNode(&N, Resources, Contexts.back());
		}

		Contexts.back().FlushBarriers();

		static_vector<ID3D12CommandList*> CLs = { Contexts.back().GetCommandList() };

		Close({ Contexts.back().GetCommandList() });
		RS->Submit(CLs);

		UpdateResourceFinalState();
	}


	/************************************************************************************************/


	void FrameGraph::ReadyResources()
	{
		Vector<FrameObject*> RenderTargets(Memory);
		RenderTargets.reserve(64);

		Vector<FrameObject*> DepthBuffers(Memory);
		DepthBuffers.reserve(64);


		for (auto& Resource : Resources.Resources)
		{
			if (Resource.Type == OT_RenderTarget ||
				Resource.Type == OT_BackBuffer)
			{
				RenderTargets.push_back(&Resource);
			}
			else if(Resource.Type == OT_DepthBuffer)
				DepthBuffers.push_back(&Resource);
		}

		auto RS	= Resources.renderSystem;

		{	// Push RenderTargets
			auto Table		= RS->_ReserveRTVHeap(RenderTargets.size());
			auto TablePOS	= Table;

			for (auto RT : RenderTargets)
			{
				auto Handle			= RT->RenderTarget.Texture;
				auto Texture		= Resources.renderSystem->RenderTargets[Handle];

				RT->RenderTarget.HeapPOS	= TablePOS;
				TablePOS					= PushRenderTarget(RS, &Texture, TablePOS);
			}
		}
		{	// Push Depth Stencils
			auto TableDepth = RS->_ReserveDSVHeap(DepthBuffers.size());
			auto TableDepthPOS = TableDepth;

			for (auto RT : DepthBuffers)
			{
				auto Handle		= RT->RenderTarget.Texture;
				auto Texture	= Resources.renderSystem->RenderTargets[Handle];

				RT->RenderTarget.HeapPOS	= TableDepthPOS;
				TableDepthPOS				= PushDepthStencil(RS, &Texture, TableDepthPOS);
			}
		}
	}


	/************************************************************************************************/


	void FrameGraph::UpdateResourceFinalState()
	{
		auto Objects = ResourceContext.GetFinalStates();

		for (auto& I : Objects)
		{
			switch (I.FO->Type)
			{
			case OT_UAVBuffer:
			{
				auto UAV	= I.FO->UAVBuffer;
				auto state	= I.State;
				Resources.renderSystem->SetObjectState(UAV, state);
			}	break;
			case OT_BackBuffer:
			case OT_DepthBuffer:
			case OT_RenderTarget:
			{
				Resources.renderSystem->RenderTargets.SetState(
					I.FO->RenderTarget.Texture, 
					I.State);
			}	break;
			case OT_StreamOut:
			{
				auto SOBuffer	= I.FO->SOBuffer;
				auto state		= I.State;
				Resources.renderSystem->SetObjectState(SOBuffer, state);
			}	break;
			default:
				FK_ASSERT(false, "UN-IMPLEMENTED BLOCK!");
			}
		}
	}


	/************************************************************************************************/


	void ClearBackBuffer(FrameGraph& Graph, float4 Color)
	{
		struct PassData
		{
			FrameResourceHandle BackBuffer;
			float4				ClearColor;
		};

		auto& Pass = Graph.AddNode<PassData>(GetCRCGUID(PRESENT),
			[=](FrameGraphNodeBuilder& Builder, PassData& Data)
			{
				Data.BackBuffer = Builder.WriteBackBuffer(GetCRCGUID(BACKBUFFER));
				Data.ClearColor = Color;
			},
			[=](const PassData& Data, const FrameResources& Resources, Context* Ctx)
			{	// do clear here
				Ctx->ClearRenderTarget(
					Resources.GetRenderTargetObject(Data.BackBuffer), 
					Data.ClearColor);
			});
	}


	/************************************************************************************************/


	void ClearDepthBuffer(FrameGraph& Graph, TextureHandle Handle, float D)
	{
		FK_VLOG(Verbosity_9, "Clearing depth buffer.");

		struct ClearDepthBuffer
		{
			FrameResourceHandle DepthBuffer;
			float				ClearDepth;
		};

		auto& Pass = Graph.AddNode<ClearDepthBuffer>(GetCRCGUID(PRESENT),
			[=](FrameGraphNodeBuilder& Builder, ClearDepthBuffer& Data)
			{
				Data.DepthBuffer = Builder.WriteDepthBuffer(Handle);
				Data.ClearDepth = D;
			},
			[=](const ClearDepthBuffer& Data, const FrameResources& Resources, Context* Ctx)
			{	// do clear here
				Ctx->ClearDepthBuffer(
					Resources.GetRenderTargetObject(Data.DepthBuffer),
					Data.ClearDepth);
			});
	}


	/************************************************************************************************/


	void PresentBackBuffer(FrameGraph& Graph, RenderWindow* Window)
	{
		struct PassData
		{
			FrameResourceHandle BackBuffer;
		};
		auto& Pass = Graph.AddNode<PassData>(GetCRCGUID(PRESENT),
			[&](FrameGraphNodeBuilder& Builder, PassData& Data)
			{
				Data.BackBuffer = Builder.PresentBackBuffer(GetCRCGUID(BACKBUFFER));
			},
			[=](const PassData& Data, const FrameResources& Resources, Context* Ctx)
			{	
			});
	}

	
	/************************************************************************************************/


	void SetRenderTargets(
		Context* Ctx, 
		static_vector<FrameResourceHandle> RenderTargets, 
		FrameResources& Resources)
	{
		static_vector<DescHeapPOS> Targets;
		for (auto RenderTarget : RenderTargets)
			auto HeapPOS = Resources.GetRenderTargetDescHeapEntry(RenderTarget);

		Ctx->SetRenderTargets(Targets, false, DescHeapPOS());
	}


	/************************************************************************************************/


	void ClearVertexBuffer(FrameGraph& FG, VertexBufferHandle PushBuffer)
	{
		FG.Resources.renderSystem->VertexBuffers.Reset(PushBuffer);
	}


}	/************************************************************************************************/