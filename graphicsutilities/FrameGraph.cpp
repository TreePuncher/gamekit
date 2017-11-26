#include "graphics.h"
#include "FrameGraph.h"

namespace FlexKit
{	/************************************************************************************************/


	ResourceTransition::ResourceTransition(FrameObjectDependency& Dep) :
		Object		{ Dep.FO },
		BeforeState	{ Dep.ExpectedState },
		AfterState	{ Dep.State}{}


	/************************************************************************************************/


	void ResourceTransition::ProcessTransition(FrameResources& Resources, Context* Ctx) const
	{
		switch (BeforeState)
		{
		case DRS_Present:
			Ctx->AddPresentBarrier(
				Resources.GetRenderTarget(Object->Handle),
				BeforeState);
			break;
		case DRS_RenderTarget:
			Ctx->AddRenderTargetBarrier(
				Resources.GetRenderTarget(Object->Handle),
				BeforeState,
				AfterState);
			break;
		case DRS_ShaderResource:
			break;
		case DRS_UAV:
			break;
		case DRS_CONSTANTBUFFER:
			break;
		case DRS_PREDICATE:
			break;
		case DRS_INDIRECTARGS:
			break;
		case DRS_UNKNOWN:
			break;
		default:
			FK_ASSERT(0);
		}
	}


	/************************************************************************************************/


	void FrameGraphNode::HandleBarriers(FrameResources& Resources, Context* Ctx)
	{
		for (const auto& T : Transitions)
			T.ProcessTransition(Resources, Ctx);
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


	void FrameGraph::AddRenderTarget(TextureHandle Texture)
	{

	}


	/************************************************************************************************/

	
	void FrameGraph::UpdateFrameGraph(RenderSystem* RS, RenderWindow* Window, iAllocator* Temp)
	{
		SubmitUploadQueues(RS);
		BeginSubmission(RS, Window);
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::BuildNode(FrameGraphResourceContext& Context, FrameGraph* FrameGraph)
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

				if (ResState & DRS_Read && Object.TransitionNeeded())
					Transitions.push_back(Object);

				Context.AddReadable(
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

				if (ResState & DRS_Read && Object.TransitionNeeded())
					Transitions.push_back(Object);

				Context.AddWriteable(
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

		//auto Pred = [&](auto& lhs) { return (lhs.Tag == Object.Tag);	};
		//Writables.remove_unstable(find(Object.SourceObject, Pred));

		// Process Detected Transitions
		for (auto& Object : Transitions)
		{
			CheckNodeSource(Node, Object);

			if (Object.ExpectedState & DRS_Read)
				Context.RemoveReadable(Object.Tag);
			else if (Object.ExpectedState & DRS_Write)
				Context.RemoveWriteable(Object.Tag);

			if (Object.State & DRS_Read)
			{
				Context.AddReadable(Object);
				Node.AddTransition(Object);
			}
			else if (Object.State & DRS_Write)
			{
				Object.Source = &Node;
				Context.AddWriteable(Object);
				Node.AddTransition(Object);
			}
			else if(Object.State == DRS_Free)
			{
				Context.Retire(Object);
			}
		}

		for (auto& SourceNode : LocalInputs)
			CheckNodeSource(Node, SourceNode);

		Node.InputObjects	= std::move(LocalInputs);
		Node.OutputObjects	= std::move(LocalOutputs);

		ClearContext();
	}


	/************************************************************************************************/


	Pair<bool, FrameObjectDependency&>	FrameGraphResourceContext::IsTrackedWriteable(uint32_t Tag)
	{
		auto Pred = [&](auto& lhs)
		{	return (lhs.Tag == Tag); };

		auto Res = find(Writables, Pred);
		return {Res == Writables.end(), *Res};
	}


	Pair<bool, FrameObjectDependency&>	FrameGraphResourceContext::IsTrackedReadable(uint32_t Tag)
	{
		auto Pred = [&](auto& lhs)
		{	return (lhs.Tag == Tag); };

		auto Res = find(Readables, Pred);
		return { Res == Readables.end(), *Res };
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AddReadableResource(uint32_t Tag, DeviceResourceState State)
	{
		FrameResourceHandle Resource = Resources->FindRenderTargetResource(Tag);
		LocalInputs.push_back(
			FrameObjectDependency{
				Resources->GetResourceObject(Resource),
				nullptr,
				Resources->GetResourceObjectState(Resource),
				State });

		return Resource;
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AddWriteableResource(uint32_t Tag, DeviceResourceState State)
	{

		FrameResourceHandle Resource = Resources->FindRenderTargetResource(Tag);
		LocalOutputs.push_back(
			FrameObjectDependency{
				Resources->GetResourceObject(Resource),
				nullptr,
				Resources->GetResourceObjectState(Resource),
				State });

		return Resource;
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::ReadRenderTarget(
			uint32_t Tag, 
			RenderTargetFormat Format)
	{
		return AddReadableResource(Tag, DeviceResourceState::DRS_ShaderResource);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteRenderTarget(
		uint32_t Tag, 
		RenderTargetFormat Format)
	{
		return AddWriteableResource(Tag, DeviceResourceState::DRS_RenderTarget);
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
		FrameResourceHandle Resource = Resources->FindRenderTargetResource(Tag);
		LocalInputs.push_back(FrameObjectDependency{
			Resources->GetResourceObject(Resource),
			nullptr,
			Resources->GetResourceObjectState(Resource),
			DeviceResourceState::DRS_ShaderResource });

		return Resource;
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::ClearContext()
	{
		LocalInputs.clear();
		LocalOutputs.clear();
		Transitions.clear();
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
		auto CL = GetCurrentCommandList(RS);

		Contexts.push_back(Context(CL, RS, Memory));
		ReadyResources();

		for (auto& N : Nodes)
			ProcessNode(&N, Resources, Contexts.back());

		static_vector<ID3D12CommandList*> CLs = { Contexts.back().GetCommandList() };

		Close	({ Contexts.back().GetCommandList() });
		Submit	(CLs, RS);
	}


	/************************************************************************************************/


	void FrameGraph::ReadyResources()
	{
		Vector<FrameObject*> RenderTargets(Memory);
		RenderTargets.reserve(64);

		for (auto& Resource : Resources.Resources)
		{
			if (Resource.Type == OT_RenderTarget ||
				Resource.Type == OT_BackBuffer)
			{
				RenderTargets.push_back(&Resource);
			}
		}

		auto RS			= Resources.RenderSystem;
		auto Table		= ReserveRTVHeap(RS, RenderTargets.size());
		auto TablePOS	= Table;

		for (auto RT : RenderTargets)
		{
			auto Handle			= RT->RenderTarget.Texture;
			auto Texture		= Resources.RenderSystem->RenderTargets[Handle];

			RT->RenderTarget.HeapPOS = TablePOS;
			TablePOS = PushRenderTarget(RS, &Texture, TablePOS);
		}
	}


	/************************************************************************************************/


	void ClearBackBuffer(FrameGraph* Graph, float4 Color)
	{
		struct PassData
		{
			FrameResourceHandle BackBuffer;
		};

		auto& Pass = Graph->AddNode<PassData>(GetCRCGUID(PRESENT),
			[&](FrameGraphNodeBuilder& Builder, PassData& Data)
			{
				Data.BackBuffer = Builder.WriteBackBuffer(GetCRCGUID(BACKBUFFER));
			},
			[=](const PassData& Data, const FrameResources& Resources, Context* Ctx)
			{	// do clear here
				Ctx->ClearRenderTarget(Resources.GetRenderTargetObject(Data.BackBuffer));
			});
	}


	/************************************************************************************************/


	void PresentBackBuffer(FrameGraph* Graph, RenderWindow* Window)
	{
		struct PassData
		{
			FrameResourceHandle BackBuffer;
		};
		auto& Pass = Graph->AddNode<PassData>(GetCRCGUID(PRESENT),
			[&](FrameGraphNodeBuilder& Builder, PassData& Data)
			{
				Data.BackBuffer = Builder.PresentBackBuffer(GetCRCGUID(BACKBUFFER));
			},
			[=](const PassData& Data, const FrameResources& Resources, Context* Ctx)
			{	// Do nothing, Barriers already handled
				// This makes sure all writes to the back-buffer complete
#if _DEBUG
				std::cout << "Present Barrier\n";
#endif
			});
	}


	/************************************************************************************************/


	void SetRenderTargets(
		Context* Ctx, 
		static_vector<FrameResourceHandle> RenderTargets, 
		FrameResources& Resources)
	{
		static_vector<DescHeapPOS> Targets;
		for (auto RenderTarget : RenderTargets) {
			auto HeapPOS = Resources.GetRenderTargetDescHeapEntry(RenderTarget);
		}

		Ctx->SetRenderTargets(Targets, false, DescHeapPOS());
	}


}	/************************************************************************************************/