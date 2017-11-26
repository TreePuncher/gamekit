/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#ifndef RENDERGRAPH_H
#define RENDERGRAPH_H

#include "..\graphicsutilities\graphics.h"
#include "..\coreutilities\containers.h"


namespace FlexKit
{
	enum RenderObjectState
	{
		RO_Undefined,
		RO_Read,
		RO_Write,
		RO_Present
	};


	enum RenderTargetFormat
	{
		TRF_INT4,
		TRF_SINT4_RGBA,
		TRF_Int4,
		TRF_UInt4,
		TRF_Float4,
		TRF_Auto,
	};


	enum FrameObjectResourceType
	{
		OT_BackBuffer,
		OT_RenderTarget,
		OT_ConstantBuffer,
		OT_PVS,
		OT_Texture,
		OT_VertexBuffer,
		OT_IndirectArguments,
		OT_UnorderedAccessView
	};

	typedef Handle_t<16> FrameResourceHandle;


	struct FrameObject
	{
		FrameObject() :
			Handle{ (uint32_t)INVALIDHANDLE }
		{}

		FrameObject(const FrameObject& rhs) :
			Type	(rhs.Type),
			State	(rhs.State),
			Tag		(rhs.Tag)
		{
			Buffer = rhs.Buffer;
		}

		FrameResourceHandle		Handle; // For Fast Search
		FrameObjectResourceType Type;
		DeviceResourceState		State;
		uint32_t				Tag;

		union
		{
			struct {
				TextureHandle	Texture;
				DescHeapPOS		HeapPOS;
			}RenderTarget;

			struct {
				char	buff[256];
			}Buffer;
		};

		static FrameObject ReadRenderTargetObject(uint32_t Tag, TextureHandle Handle)
		{
			FrameObject RenderTarget;
			RenderTarget.State					= DeviceResourceState::DRS_ShaderResource;
			RenderTarget.Type					= OT_RenderTarget;
			RenderTarget.Tag					= Tag;
			RenderTarget.RenderTarget.Texture	= Handle;

			return RenderTarget;
		}


		static FrameObject WriteRenderTargetObject(uint32_t Tag, TextureHandle Handle)
		{
			FrameObject RenderTarget;
			RenderTarget.State		            = DeviceResourceState::DRS_RenderTarget;
			RenderTarget.Type		            = OT_RenderTarget;
			RenderTarget.Tag		            = Tag;
			RenderTarget.RenderTarget.Texture   = Handle;

			return RenderTarget;
		}

		static FrameObject BackBufferObject(uint32_t Tag, TextureHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_RenderTarget)
		{
			FrameObject RenderTarget;
			RenderTarget.State					= InitialState;
			RenderTarget.Type					= OT_BackBuffer;
			RenderTarget.Tag					= Tag;
			RenderTarget.RenderTarget.Texture	= Handle;

			return RenderTarget;
		}
	};


	/************************************************************************************************/


	typedef Vector<FrameObject>		PassObjectList;
	class FrameGraph;

	class FrameResources
	{
	public:
		FrameResources(RenderSystem* rendersystem, iAllocator* Memory) : 
			Resources(Memory), 
			RenderSystem(rendersystem)
		{}

		void AddBackBuffer(TextureHandle Handle, uint32_t Tag, DeviceResourceState InitialState = DeviceResourceState::DRS_RenderTarget)
		{
			Resources.push_back(
				FrameObject::BackBufferObject(Tag, Handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}

		PassObjectList		Resources;
		RenderSystem*		RenderSystem;

		//  || Should These Be in a scene structure instead?
		//  \/
		GeometryTable*		GT;
		PointLightBuffer*	PointLights;
		SpotLightBuffer*	SpotLights;
		//
		//

		TextureObject		GetRenderTargetObject(FrameResourceHandle Handle) const
		{
			return { Resources[Handle].RenderTarget.HeapPOS, Resources[Handle].RenderTarget.Texture };
		}

		DeviceResourceState	GetResourceObjectState(FrameResourceHandle Handle)
		{
			return Resources[Handle].State;
		}

		FrameObject*		GetResourceObject(FrameResourceHandle Handle)
		{
			return &Resources[Handle];
		}


		FrameResourceHandle FindRenderTargetResource(uint32_t Tag)
		{
			auto res = find(Resources, 
				[&](const auto& LHS)
				{
					return (
						LHS.Tag == Tag && 
						(	LHS.Type == OT_RenderTarget || 
							Tag && LHS.Type == OT_BackBuffer));	
				});

			if (res != Resources.end())
				return res->Handle;

			// Create New Resource
			FrameResourceHandle NewResource;

			FK_ASSERT(0);

			return NewResource;
		}


		TextureHandle	GetRenderTarget(FrameResourceHandle Handle) const
		{
			return Resources[Handle].RenderTarget.Texture;
		}


		DescHeapPOS		GetRenderTargetDescHeapEntry(FrameResourceHandle Handle)
		{
			return Resources[Handle].RenderTarget.HeapPOS;
		}
	};


	/************************************************************************************************/

	class FrameGraphNode;

	struct FrameObjectDependency
	{
		FrameObjectDependency(
			FrameObject*		FO_IN				= nullptr,
			FrameGraphNode*		SourceObject_IN		= nullptr,
			DeviceResourceState	ExpectedState_IN	= DRS_UNKNOWN,
			DeviceResourceState	State_IN			= DRS_UNKNOWN)
			:
				FO				(FO_IN),
				Source			(SourceObject_IN),
				ExpectedState	(ExpectedState_IN),
				State			(State_IN),
				Tag				(FO_IN ? FO_IN->Tag : -1)
		{}

		bool TransitionNeeded()
		{
			return ExpectedState != State;
		}

		FrameObject*		FO;
		FrameGraphNode*		Source;
		uint32_t			Tag;
		DeviceResourceState	ExpectedState;
		DeviceResourceState	State;
	};


	/************************************************************************************************/


	class ResourceTransition
	{
	public:
		ResourceTransition() :
			Object		{ nullptr },
			BeforeState	{ DRS_UNKNOWN },
			AfterState	{ DRS_UNKNOWN }{}

		ResourceTransition		(FrameObjectDependency& Dep);
		void ProcessTransition	(FrameResources& Resources, Context* Ctx) const;

	private:
		FrameObject*		Object;
		DeviceResourceState	BeforeState;
		DeviceResourceState	AfterState;
	};


	/************************************************************************************************/


	FLEXKITAPI class FrameGraphNode
	{
	public:
		typedef std::function<void (FrameResources& Resources, Context* Ctx)> FN_NodeAction;

		FrameGraphNode(iAllocator* TempMemory = nullptr) :
			InputObjects	(TempMemory),
			OutputObjects	(TempMemory),
			Sources			(TempMemory),
			Transitions		(TempMemory),
			NodeAction		([](auto, auto) {}),
			Executed		(false)
		{}


		FrameGraphNode(const FrameGraphNode& RHS) : 
			Sources			(RHS.Sources),
			InputObjects	(RHS.InputObjects),
			OutputObjects	(RHS.OutputObjects),
			Transitions		(RHS.Transitions),
			NodeAction		(RHS.NodeAction),
			Executed		(false)
		{}


		~FrameGraphNode()
		{}


		void HandleBarriers(FrameResources& Resouces, Context* Ctx);

		void AddTransition	(FrameObjectDependency& Dep);
		bool DependsOn		(uint32_t Tag);
		bool Outputs		(uint32_t Tag);

		Vector<FrameGraphNode*>		GetNodeDependencies()	{ return (nullptr); } 
		//PassObjectList				GetInputs()				{ return InputObjects; }
		//PassObjectList				GetOutputs()			{ return OutputObjects; }

		bool							Executed;
		FN_NodeAction					NodeAction;
		Vector<FrameGraphNode*>			Sources;// Nodes that this node reads from
		Vector<FrameObjectDependency>	InputObjects;
		Vector<FrameObjectDependency>	OutputObjects;
		Vector<ResourceTransition>		Transitions;
	};


	/************************************************************************************************/


	FLEXKITAPI class FrameGraphResourceContext
	{
	public:
		FrameGraphResourceContext(iAllocator* Temp) :
			Writables	{Temp},
			Readables	{Temp},
			Retirees	{Temp}
		{}

		void AddWriteable(const FrameObjectDependency& NewObject)
		{
			Writables.push_back(NewObject);
		}

		void AddReadable(const FrameObjectDependency& NewObject)
		{
			Readables.push_back(NewObject);
		}

		void RemoveReadable(uint32_t Tag)
		{
			auto Pred = [&](auto& lhs) { return (lhs.Tag == Tag);	};
			Readables.remove_unstable(find(Readables, Pred));
		}

		void RemoveWriteable(uint32_t Tag)
		{
			auto Pred = [&](auto& lhs) { return (lhs.Tag == Tag);	};
			Writables.remove_unstable(find(Writables, Pred));
		}

		void Retire(FrameObjectDependency& Object)
		{
			RemoveReadable	(Object.Tag);
			RemoveWriteable	(Object.Tag);

			Object.ExpectedState = Object.State;
			Retirees.push_back(Object);
		}

		Pair<bool, FrameObjectDependency&> IsTrackedWriteable	(uint32_t Tag);
		Pair<bool, FrameObjectDependency&> IsTrackedReadable	(uint32_t Tag);

		Vector<FrameObjectDependency>	Writables;
		Vector<FrameObjectDependency>	Readables;
		Vector<FrameObjectDependency>	Retirees;
	};


	FLEXKITAPI class FrameGraphNodeBuilder
	{
	public:
		FrameGraphNodeBuilder(
			FrameResources*				Resources_IN, 
			FrameGraphNode&				Node_IN,
			iAllocator*					Temp) :
				LocalInputs		{Temp},
				LocalOutputs	{Temp},
				Node			{Node_IN},
				Transitions		{Temp},
				Resources		{Resources_IN}
		{}

		// No Copying
		FrameGraphNodeBuilder				(const FrameGraphNodeBuilder& RHS) = delete;
		FrameGraphNodeBuilder&	operator =	(const FrameGraphNodeBuilder& RHS) = delete;


		void BuildNode(FrameGraphResourceContext& Context, FrameGraph* FrameGraph);

		FrameResourceHandle ReadRenderTarget	(uint32_t Tag, RenderTargetFormat Formt = TRF_Auto);
		FrameResourceHandle WriteRenderTarget	(uint32_t Tag, RenderTargetFormat Formt = TRF_Auto);

		FrameResourceHandle	PresentBackBuffer	(uint32_t Tag);
		FrameResourceHandle	ReadBackBuffer		(uint32_t Tag);
		FrameResourceHandle	WriteBackBuffer		(uint32_t Tag);

		void ClearContext();

	private:
		FrameResourceHandle AddReadableResource		(uint32_t Tag, DeviceResourceState State);
		FrameResourceHandle AddWriteableResource	(uint32_t Tag, DeviceResourceState State);


		template<typename FN_Create>
		FrameResourceHandle AddWriteableResource(uint32_t Tag, DeviceResourceState State, FN_Create CreateResource)
		{
			auto Res = AddWriteableResource(Tag, State);
			if (Res == -1)
				Res = CreateResource();

			return Res;
		}


		enum class CheckStateRes
		{
			TransitionNeeded,
			CorrectState,
			ResourceNotTracked,
			Error,
		};


		static CheckStateRes CheckResourceSituation(
			Vector<FrameObjectDependency>&  Set1,
			Vector<FrameObjectDependency>&  Set2,
			FrameObjectDependency&			Object);

		Vector<FrameObjectDependency>	LocalOutputs;
		Vector<FrameObjectDependency>	LocalInputs;
		Vector<FrameObjectDependency>	Transitions;

		FrameGraphNode&					Node;
		FrameResources*					Resources;
	};


	/************************************************************************************************/


	FLEXKITAPI class FrameGraph
	{
	public:
		FrameGraph(RenderSystem* RS, iAllocator* Temp) :
			Nodes{ Temp },
			Memory{ Temp },
			Resources{ RS,			Temp },
			ResourceContext{ Temp } {}


		FrameGraph				(const FrameGraph& RHS) = delete;
		FrameGraph& operator =	(const FrameGraph& RHS) = delete;

		template<typename TY, typename SetupFN, typename DrawFN>
		TY& AddNode(uint32_t Tag, SetupFN Setup, DrawFN Draw)
		{
			Nodes.push_back(FrameGraphNode(Memory));

			TY& Data				= Memory->allocate_aligned<TY>();
			FrameGraphNode& Node	= Nodes.back();
			FrameGraphNodeBuilder Builder(&Resources, Node, Memory);

			Node.NodeAction = [=, &Data, &Node](
				FrameResources& Resources, 
				Context* Ctx) 
			{
				// TODO(R.M.): Auto Barrier Handling Here
				Node.HandleBarriers(Resources, Ctx);
				Draw(Data, Resources, Ctx);
				Ctx->FlushBarriers();
			};

			Setup(Builder, Data);
			Builder.BuildNode(ResourceContext, this);

			return Data;
		}

		void AddRenderTarget	(TextureHandle Texture);
		void ProcessNode		(FrameGraphNode* N, FrameResources& Resources, Context& Context);
		
		void UpdateFrameGraph	(RenderSystem* RS, RenderWindow* Window, iAllocator* Temp);// 
		void SubmitFrameGraph	(RenderSystem* RS, RenderWindow* Window);

		enum class ObjectState
		{
			OA_Closed,
			OA_Read,
			OA_Write,
			OA_UnCreated
		};


		struct RenderTargetState
		{
			ObjectState		State;
			TextureHandle	Handle;
		};

		FrameResources				Resources;
		FrameGraphResourceContext	ResourceContext;
		iAllocator*					Memory;
		Vector<FrameGraphNode>		Nodes;

	private:
		void ReadyResources();
	};


	/************************************************************************************************/


	void ClearBackBuffer	(FrameGraph* Graph, float4 Color = {0.0f, 0.0f, 0.0f, 0.0f });// Clears BackBuffer to Black
	void PresentBackBuffer	(FrameGraph* Graph, RenderWindow* Window);
	void SetRenderTargets	(Context* Ctx, static_vector<FrameResourceHandle> RenderTargets, FrameResources& FG);
}
#endif