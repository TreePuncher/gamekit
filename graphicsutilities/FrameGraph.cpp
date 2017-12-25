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
		switch (AfterState)
		{
		case DRS_Present:
		{
			Ctx->AddPresentBarrier(
				Resources.GetRenderTarget(Object->Handle),
				BeforeState);
		}	break;
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


	Pair<bool, FrameObjectDependency&>	FrameGraphResourceContext::IsTrackedWriteable(uint32_t Tag)
	{
		auto Pred = [&](auto& lhs)
		{	return (lhs.Tag == Tag); };

		auto Res = find(Writables, Pred);
		return {Res != Writables.end(), *Res};
	}


	Pair<bool, FrameObjectDependency&>	FrameGraphResourceContext::IsTrackedReadable(uint32_t Tag)
	{
		auto Pred = [&](auto& lhs)
		{	return (lhs.Tag == Tag); };

		auto Res = find(Readables, Pred);
		return { Res != Readables.end(), *Res };
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AddReadableResource(uint32_t Tag, DeviceResourceState State)
	{
		bool TrackedReadable = Context.IsTrackedReadable(Tag);
		bool TrackedWritable = Context.IsTrackedWriteable(Tag);

		if (!TrackedReadable && !TrackedWritable)
		{
			FrameResourceHandle Resource = Resources->FindRenderTargetResource(Tag);

			LocalInputs.push_back(
				FrameObjectDependency{
					Resources->GetResourceObject(Resource),
					nullptr,
					Resources->GetResourceObjectState(Resource),
					State });

			Context.AddReadable(LocalOutputs.back());
			Transitions.push_back(LocalOutputs.back());

			return Resource;
		}
		else
		{
			auto Object				= TrackedReadable ? 
				Context.GetReadable(Tag) : Context.GetWriteable(Tag);

			Object.ExpectedState	= Object.State;
			Object.State			= State;
			LocalOutputs.push_back(Object);

			if (TrackedWritable) {
				Context.RemoveWriteable(Tag);
				Context.AddReadable(Object);
				Transitions.push_back(Object);
			}

			return Object.FO->Handle;
		}
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AddWriteableResource(uint32_t Tag, DeviceResourceState State)
	{
		bool TrackedReadable = Context.IsTrackedReadable(Tag);
		bool TrackedWritable = Context.IsTrackedWriteable(Tag);

		if (!TrackedReadable && !TrackedWritable)
		{
			FrameResourceHandle Resource = Resources->FindRenderTargetResource(Tag);

			LocalOutputs.push_back(
				FrameObjectDependency{
					Resources->GetResourceObject(Resource),
					nullptr,
					Resources->GetResourceObjectState(Resource),
					State });

			Context.AddWriteable(LocalOutputs.back());
			Transitions.push_back(LocalOutputs.back());

			return Resource;
		}
		else
		{
			auto Object				= TrackedReadable ? 
				Context.GetReadable(Tag) : Context.GetWriteable(Tag);

			Object.ExpectedState	= Object.State;
			Object.State			= State;
			LocalOutputs.push_back(Object);

			if (TrackedReadable) {
				Context.RemoveReadable(Tag);
				Context.AddWriteable(Object);
				Transitions.push_back(Object);
			}

			return Object.FO->Handle;
		}
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

		Contexts.push_back(Context(CL, RS, Memory));
		ReadyResources();

		for (auto& N : Nodes)
			ProcessNode(&N, Resources, Contexts.back());

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

		for (auto& Resource : Resources.Resources)
		{
			if (Resource.Type == OT_RenderTarget ||
				Resource.Type == OT_BackBuffer)
			{
				RenderTargets.push_back(&Resource);
			}
		}

		auto RS			= Resources.RenderSystem;
		auto Table		= RS->_ReserveRTVHeap(RenderTargets.size());
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


	void FrameGraph::UpdateResourceFinalState()
	{
		auto Objects = ResourceContext.GetFinalStates();

		for (auto& I : Objects)
		{
			switch (I.State)
			{
			case DRS_RenderTarget:
			case DRS_Present:
			{
				if (I.FO->Type == FrameObjectResourceType::OT_BackBuffer ||
					I.FO->Type == FrameObjectResourceType::OT_RenderTarget)
				{
					Resources.RenderSystem->RenderTargets.SetState(
						I.FO->RenderTarget.Texture, 
						I.State);
				}
			}	break;
			case DRS_ShaderResource:
			{	FK_ASSERT(0, "Not Implemented!");
			}	break;
			case DRS_UAV:
			{
				FK_ASSERT(0, "Not Implemented!");
			}	break;
			case DRS_VERTEXBUFFER: // & DRS_CONSTANTBUFFER
			{	FK_ASSERT(0, "Not Implemented!");
			}	break;
			case DRS_PREDICATE:
			{	FK_ASSERT(0, "Not Implemented!");
			}	break;
			case DRS_INDIRECTARGS:
			{	FK_ASSERT(0, "Not Implemented!");
			}	break;
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
#if _DEBUG
				std::cout << "Present Barrier\n";
#endif
			});
	}

	
	/************************************************************************************************/


	void DrawRectangle(
		FrameGraph& Graph, 
		VertexBufferHandle PushBuffer,
		ConstantBufferHandle CB,
		float2 UpperLeft, 
		float2 BottomRight, 
		float4 Color)
	{
		struct DrawRect
		{
			FrameResourceHandle		BackBuffer;
			VertexBufferHandle		VertexBuffer;
			ConstantBufferHandle	ConstantBuffer;
			size_t					CB_Offset;
			size_t					VB_Offset;
		};


		struct Vert {
			float2 POS;
			float2 UV;
			float4 Color;
		};


		auto& Pass = Graph.AddNode<DrawRect>(GetCRCGUID(PRESENT),
			[&](FrameGraphNodeBuilder& Builder, DrawRect& Data)
		{
			// Single Thread Section
			// All Rendering Data Must be pushed into buffers here in advance, or allocated in advance
			// for thread safety

			Data.BackBuffer		= Builder.WriteBackBuffer(GetCRCGUID(BACKBUFFER));
			Data.VB_Offset		= Graph.Resources.GetVertexBufferOffset(PushBuffer, sizeof(float2));
			Data.VertexBuffer	= PushBuffer;
			Data.ConstantBuffer = CB;

			float2 RectUpperLeft	= UpperLeft;
			float2 RectUpperRight	= { BottomRight.x, UpperLeft.y};
			float2 RectBottomRight	= BottomRight;
			float2 RectBottomLeft	= { UpperLeft.x, BottomRight.y};
			float2 UV				= { 0, 0 };

			float2 TestOffset = { -1, -1 };
			PushVertex(Vert{ RectUpperLeft, UV,		Color }, Data.VertexBuffer, Graph.Resources);
			PushVertex(Vert{ RectBottomRight, UV,	Color }, Data.VertexBuffer, Graph.Resources);
			PushVertex(Vert{ RectBottomLeft, UV,	Color }, Data.VertexBuffer, Graph.Resources);

			PushVertex(Vert{ RectUpperLeft, UV,		Color }, Data.VertexBuffer, Graph.Resources);
			PushVertex(Vert{ RectUpperRight, UV,	Color }, Data.VertexBuffer, Graph.Resources);
			PushVertex(Vert{ RectBottomRight, UV,	Color }, Data.VertexBuffer, Graph.Resources);

			/*
			cbuffer LocalConstants : register( b1 )
			{
			float4	 Albedo; // + roughness
			float4	 Specular;
			float4x4 WT;
			}
			*/

			struct Constants
			{
				float4		Albedo;
				float4		Specular;
				float4x4	WT;
			}CB_Data_1, CB_Data_2;

			CB_Data_1.Albedo	= float4(1, 1, 0, 1);
			CB_Data_1.Specular	= float4(1, 1, 0, 1);
			CB_Data_1.WT		= float4x4::Identity();

			CB_Data_2.Albedo	= float4(1, 0, 1, 1);
			CB_Data_2.Specular	= float4(1, 0, 1, 1);
			CB_Data_2.WT		= float4x4::Identity();

			Data.CB_Offset = BeginNewConstantBuffer(CB,	Graph.Resources);
			PushConstantBufferData(CB_Data_1,	CB,	Graph.Resources);

			BeginNewConstantBuffer(CB, Graph.Resources);
			PushConstantBufferData(CB_Data_2,	CB,	Graph.Resources);
		},
			[=](const DrawRect& Data, const FrameResources& Resources, Context* Ctx)
		{
			// Multithreaded Section

			D3D12_RECT Rects{
					(LONG)(0),
					(LONG)(0),
					(LONG)(1920),
					(LONG)(1080),
			};

			Ctx->SetViewports({ { 0, 0, 1920, 1080, 0, 1 } });
			Ctx->SetScissorRects({ Rects });
			Ctx->SetRenderTargets({ (DescHeapPOS)Resources.GetRenderTargetObject(Data.BackBuffer) }, false);

			Ctx->SetRootSignature(Resources.RenderSystem->Library.RS4CBVs4SRVs);
			Ctx->SetPipelineState(Resources.GetPipelineState(EPIPELINESTATES::Draw_PSO));
			Ctx->SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

			Ctx->SetGraphicsConstantBufferView(2, Data.ConstantBuffer, Data.CB_Offset);
			Ctx->SetVertexBuffers(VertexBufferList{ { Data.VertexBuffer, sizeof(Vert)} });
			Ctx->Draw(6, Data.VB_Offset);

			Ctx->SetGraphicsConstantBufferView(2, Data.ConstantBuffer, Data.CB_Offset + 256);
			Ctx->Draw(6, Data.VB_Offset + 6);
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
		FG.Resources.RenderSystem->VertexBuffers.Reset(PushBuffer);
	}


}	/************************************************************************************************/