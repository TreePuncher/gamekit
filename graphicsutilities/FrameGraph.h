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
		OT_DepthBuffer,
		OT_RenderTarget,
		OT_ConstantBuffer,
		OT_PVS,
		OT_Texture,
		OT_VertexBuffer,
		OT_IndirectArguments,
		OT_UnorderedAccessView
	};

	typedef Handle_t<16> FrameResourceHandle;
	typedef Handle_t<16> StaticFrameResourceHandle;


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

			TextureHandle Texture;

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

		static FrameObject DepthBufferObject(uint32_t Tag, TextureHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_DEPTHBUFFER)
		{
			FrameObject RenderTarget;
			RenderTarget.State                = InitialState;
			RenderTarget.Type                 = OT_DepthBuffer;
			RenderTarget.Tag                  = Tag;
			RenderTarget.RenderTarget.Texture = Handle;

			return RenderTarget;
		}

		static FrameObject TextureObject(uint32_t Tag, TextureHandle Handle )
		{
			FrameObject RenderTarget;
			RenderTarget.State                = DeviceResourceState::DRS_ShaderResource;
			RenderTarget.Type                 = OT_Texture;
			RenderTarget.Tag                  = Tag;
			RenderTarget.RenderTarget.Texture = Handle;

			return RenderTarget;
		}
	};


	/************************************************************************************************/


	typedef Vector<FrameObject>		PassObjectList;
	class FrameGraph;

	FLEXKITAPI class FrameResources
	{
	public:
		FrameResources(RenderSystem* rendersystem, iAllocator* Memory) : 
			Resources		(Memory), 
			Textures		(Memory),
			RenderSystem	(rendersystem)
		{}

		PassObjectList		Resources;
		PassObjectList		Textures;	// State should be mostly Static across frame
		RenderSystem*		RenderSystem;

		//  TODO: Move these to a Scene Structure
		//	Why are these here again?
		PointLightList*	PointLights;
		SpotLightList*	SpotLights;
		//


		void AddRenderTarget(TextureHandle Handle)
		{
			AddRenderTarget(
				Handle,
				RenderSystem->GetTag(Handle),
				RenderSystem->RenderTargets.GetState(Handle));
		}

		void AddRenderTarget(TextureHandle Handle, uint32_t Tag, DeviceResourceState InitialState = DeviceResourceState::DRS_RenderTarget)
		{
			Resources.push_back(
				FrameObject::BackBufferObject(Tag, Handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}

		void AddDepthBuffer(TextureHandle Handle)
		{
			AddDepthBuffer(
				Handle,
				RenderSystem->GetTag(Handle),
				RenderSystem->RenderTargets.GetState(Handle));
		}

		void AddDepthBuffer(TextureHandle Handle, uint32_t Tag, DeviceResourceState InitialState = DeviceResourceState::DRS_DEPTHBUFFER)
		{
			Resources.push_back(
				FrameObject::DepthBufferObject(Tag, Handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}

		TextureObject			GetRenderTargetObject(FrameResourceHandle Handle) const
		{
			return { Resources[Handle].RenderTarget.HeapPOS, Resources[Handle].RenderTarget.Texture };
		}


		ID3D12PipelineState*	GetPipelineState(EPIPELINESTATES State)	const
		{
			return RenderSystem->GetPSO(State);
		}


		size_t					GetVertexBufferOffset(VertexBufferHandle Handle, size_t VertexSize)
		{
			return RenderSystem->VertexBuffers.GetCurrentVertexBufferOffset(Handle) / VertexSize;
		}


		DeviceResourceState		GetResourceObjectState(FrameResourceHandle Handle)
		{
			return Resources[Handle].State;
		}


		FrameObject*			GetResourceObject(FrameResourceHandle Handle)
		{
			return &Resources[Handle];
		}


		TextureHandle			GetRenderTarget(FrameResourceHandle Handle) const
		{
			return Resources[Handle].RenderTarget.Texture;
		}


		uint2					GetRenderTargetWH(FrameResourceHandle Handle) const
		{
			return RenderSystem->GetRenderTargetWH(GetRenderTarget(Handle));
		}

		DescHeapPOS				GetRenderTargetDescHeapEntry(FrameResourceHandle Handle)
		{
			return Resources[Handle].RenderTarget.HeapPOS;
		}


		TextureHandle GetTexture(StaticFrameResourceHandle Handle) const
		{
			return Textures[Handle].Texture;
		}


		StaticFrameResourceHandle AddTexture(TextureHandle Handle, uint32_t Tag)
		{
			FK_ASSERT(0);
		}


		FrameResourceHandle	FindRenderTargetResource(uint32_t Tag)
		{
			auto res = find(Resources, 
				[&](const auto& LHS)
				{
					return (
						LHS.Tag == Tag && 
						(	LHS.Type == OT_RenderTarget || 
							LHS.Type == OT_BackBuffer	|| 
							LHS.Type == OT_DepthBuffer ));	
				});

			if (res != Resources.end())
				return res->Handle;

			// Create New Resource
			FrameResourceHandle NewResource;

			FK_ASSERT(0);

			return NewResource;
		}

		FrameResourceHandle	FindRenderTargetResource(TextureHandle Handle)
		{
			auto res = find(Resources, 
				[&](const auto& LHS)
				{
					auto CorrectType = (
						LHS.Type == OT_RenderTarget ||
						LHS.Type == OT_BackBuffer	||
						LHS.Type == OT_DepthBuffer);

					return (CorrectType && LHS.Texture == Handle);
				});

			if (res != Resources.end())
				return res->Handle;

			// Create New Resource
			FrameResourceHandle NewResource;

			FK_ASSERT(0);

			return NewResource;
		}
	};


	/************************************************************************************************/


	template<typename TY_V>
	bool PushVertex(const TY_V& Vertex, VertexBufferHandle Buffer, FrameResources& Resources)
	{
		bool res = Resources.RenderSystem->VertexBuffers.PushVertex(Buffer, (void*)&Vertex, sizeof(TY_V));
		FK_ASSERT(res, "Failed to Push Vertex!");
		return res;
	}


	/************************************************************************************************/


	template<typename TY_V>
	bool PushVertex(const TY_V& Vertex, VertexBufferHandle Buffer, FrameResources& Resources, size_t PushSize)
	{
		bool res = Resources.RenderSystem->VertexBuffers.PushVertex(Buffer, (void*)&Vertex, PushSize);
		FK_ASSERT(res, "Failed to Push Vertex!");
		return res;
	}


	/************************************************************************************************/


	template<typename TY_V>
	inline size_t GetCurrentVBufferOffset(VertexBufferHandle Buffer, FrameResources& Resources)
	{
		return Resources.RenderSystem->VertexBuffers.GetCurrentVertexBufferOffset(Buffer) / sizeof(TY_V);
	}

	inline size_t BeginNewConstantBuffer(ConstantBufferHandle CB, FrameResources& Resources)
	{
		return Resources.RenderSystem->ConstantBuffers.BeginNewBuffer(CB);
	}


	//inline size_t GetCurrentConstantBufferOffset(ConstantBufferHandle Buffer, FrameResources& Resources)
	//{

//	}

	template<typename TY_CB>
	bool PushConstantBufferData(const TY_CB& Constants, ConstantBufferHandle Buffer, FrameResources& Resources)
	{
		bool res = Resources.RenderSystem->ConstantBuffers.Push(Buffer, (void*)&Constants, sizeof(TY_CB));
		FK_ASSERT(res, "Failed to Push Constants!");
		return res;
	}


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
		uint32_t			ID;	// Extra ID
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


		void HandleBarriers	(FrameResources& Resouces, Context* Ctx);
		void AddTransition	(FrameObjectDependency& Dep);
		bool DependsOn		(uint32_t Tag);
		bool Outputs		(uint32_t Tag);


		Vector<FrameGraphNode*>		GetNodeDependencies()	{ return (nullptr); } 


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


		FrameObjectDependency& GetReadable(uint32_t Tag)
		{
			auto Pred = [&](auto& lhs) { return (lhs.Tag == Tag);	};
			auto Res = find(Readables, Pred);

			return *Res;
		}


		FrameObjectDependency& GetWriteable(uint32_t Tag)
		{
			auto Pred = [&](auto& lhs) { return (lhs.Tag == Tag);	};
			auto Res = find(Writables, Pred);

			return *Res;
		}

		Vector<FrameObjectDependency>	GetFinalStates()
		{
			Vector<FrameObjectDependency> Objects(Writables.Allocator);
			Objects.reserve(Writables.size() + Readables.size() + Retirees.size());

			Objects += Writables;
			Objects += Readables;
			Objects += Retirees;

			return std::move(Objects);
		}

		Pair<bool, FrameObjectDependency&> IsTrackedWriteable	(uint32_t Tag);
		Pair<bool, FrameObjectDependency&> IsTrackedReadable	(uint32_t Tag);
		Pair<bool, FrameObjectDependency&> IsTrackedReadable	(TextureHandle Handle, FrameObjectResourceType Type);
		Pair<bool, FrameObjectDependency&> IsTrackedWriteable	(TextureHandle Handle, FrameObjectResourceType Type);

		Vector<FrameObjectDependency>	Writables;
		Vector<FrameObjectDependency>	Readables;
		Vector<FrameObjectDependency>	Retirees;
	};


	/************************************************************************************************/


	FLEXKITAPI class FrameGraphNodeBuilder
	{
	public:
		FrameGraphNodeBuilder(
			FrameResources*				Resources_IN, 
			FrameGraphNode&				Node_IN,
			FrameGraphResourceContext&	Context_IN,
			iAllocator*					Temp) :
				Context			{Context_IN},
				LocalInputs		{Temp},
				LocalOutputs	{Temp},
				Node			{Node_IN},
				Resources		{Resources_IN},
				Transitions		{Temp}
		{}


		// No Copying
		FrameGraphNodeBuilder				(const FrameGraphNodeBuilder& RHS) = delete;
		FrameGraphNodeBuilder&	operator =	(const FrameGraphNodeBuilder& RHS) = delete;

		void BuildNode(FrameGraph* FrameGraph);

		StaticFrameResourceHandle ReadTexture	(uint32_t Tag, TextureHandle Texture);

		FrameResourceHandle ReadRenderTarget	(uint32_t Tag, RenderTargetFormat Formt = TRF_Auto);
		FrameResourceHandle WriteRenderTarget	(uint32_t Tag, RenderTargetFormat Formt = TRF_Auto);

		FrameResourceHandle ReadRenderTarget	(TextureHandle Handle);
		FrameResourceHandle WriteRenderTarget	(TextureHandle Handle);

		FrameResourceHandle	PresentBackBuffer	(uint32_t Tag);
		FrameResourceHandle	ReadBackBuffer		(uint32_t Tag);
		FrameResourceHandle	WriteBackBuffer		(uint32_t Tag);

		FrameResourceHandle	ReadDepthBuffer		(uint32_t Tag);
		FrameResourceHandle	WriteDepthBuffer	(uint32_t Tag);

		//FrameResourceHandle	ReadDepthBuffer		(TextureHandle Handle);
		FrameResourceHandle	WriteDepthBuffer	(TextureHandle Handle);

	private:
		FrameResourceHandle AddReadableResource		(uint32_t Tag, DeviceResourceState State);
		FrameResourceHandle AddWriteableResource	(uint32_t Tag, DeviceResourceState State);

		FrameResourceHandle AddWriteableResource	(TextureHandle Handle, DeviceResourceState State, FrameObjectResourceType Type);



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

		FrameGraphResourceContext&		Context;
		FrameGraphNode&					Node;
		FrameResources*					Resources;
	};


	/************************************************************************************************/


	FLEXKITAPI class FrameGraph
	{
	public:
		FrameGraph(RenderSystem* RS, iAllocator* Temp) :
			Nodes			{ Temp },
			Memory			{ Temp },
			Resources		{ RS,	Temp },
			ResourceContext	{ Temp } {}

		FrameGraph				(const FrameGraph& RHS) = delete;
		FrameGraph& operator =	(const FrameGraph& RHS) = delete;

		template<typename TY, typename SetupFN, typename DrawFN>
		TY& AddNode(uint32_t Tag, SetupFN Setup, DrawFN Draw)
		{
			Nodes.push_back(FrameGraphNode(Memory));

			TY& Data				= Memory->allocate_aligned<TY>();
			FrameGraphNode& Node	= Nodes.back();
			FrameGraphNodeBuilder Builder(&Resources, Node, ResourceContext, Memory);

			Node.NodeAction = [=, &Data, &Node](
				FrameResources& Resources, 
				Context* Ctx) 
			{
				Node.HandleBarriers(Resources, Ctx);
				Draw(Data, Resources, Ctx);
				Ctx->FlushBarriers();
			};

			Setup(Builder, Data);
			Builder.BuildNode(this);

			return Data;
		}

		void AddRenderTarget	(TextureHandle Texture);

		void ProcessNode		(FrameGraphNode* N, FrameResources& Resources, Context& Context);
		
		void UpdateFrameGraph	(RenderSystem* RS, RenderWindow* Window, iAllocator* Temp);// 
		void SubmitFrameGraph	(RenderSystem* RS, RenderWindow* Window);

		FrameResources				Resources;
		FrameGraphResourceContext	ResourceContext;
		iAllocator*					Memory;
		Vector<FrameGraphNode>		Nodes;

	private:
		void ReadyResources();
		void UpdateResourceFinalState();


	};


	/************************************************************************************************/

	struct Rectangle
	{
		float4 Color	= { 1.0f, 1.0f, 1.0f, 1.0f };
		float2 Position;
		float2 WH;
	};

	typedef Vector<Rectangle> RectangleList;

	void ClearBackBuffer	(FrameGraph& Graph, float4 Color = {0.0f, 0.0f, 0.0f, 0.0f });// Clears BackBuffer to Black
	void ClearDepthBuffer	(FrameGraph& Graph, TextureHandle Handle, float D);
	void PresentBackBuffer	(FrameGraph& Graph, RenderWindow* Window);


	void SetRenderTargets	(Context* Ctx, static_vector<FrameResourceHandle> RenderTargets, FrameResources& FG);
	void ClearVertexBuffer	(FrameGraph& FG, VertexBufferHandle PushBuffer);


	/************************************************************************************************/


	struct ShapeVert {
		float2 POS;
		float2 UV;
		float4 Color;
	};

	struct ShapeDraw
	{
		size_t ConstantBufferOffset;
		size_t VertexBufferOffset;
		size_t VertexCount;

		enum class RenderMode
		{
			Line,
			Triangle,
			Textured,
		}Mode = RenderMode::Triangle;
	};

	typedef Vector<ShapeDraw> DrawList;

	struct Constants
	{
		float4		Albedo;
		float4		Specular;
		float4x4	WT;
	};


	class ShapeProtoType
	{
	public:
		ShapeProtoType() {}
		virtual ~ShapeProtoType() {}
		ShapeProtoType(const ShapeProtoType& rhs) = delete;

		virtual void AddShapeDraw(
			DrawList&				DrawList,
			VertexBufferHandle		PushBuffer,
			ConstantBufferHandle	CB,
			FrameResources&			Resources) = 0;
	};


	class ShapeList final : public ShapeProtoType
	{
	public:
		ShapeList(iAllocator* Memory = SystemAllocator) :
			Shapes{ Memory } {}

		ShapeList(const ShapeList& rhs)
		{
		}


		~ShapeList()
		{
			Shapes.Release();
		}

		void AddShape(ShapeProtoType* Shape)
		{
			Shapes.push_back(Shape);
		}

	protected:
		void AddShapeDraw(
			DrawList&				DrawList,
			VertexBufferHandle		PushBuffer,
			ConstantBufferHandle	CB,
			FrameResources&			Resources) override
		{
			for (auto Shape : Shapes)
				Shape->AddShapeDraw(
					DrawList, 
					PushBuffer, 
					CB, 
					Resources);
		}

		FlexKit::Vector<ShapeProtoType*> Shapes;
	};

	/*
	void AddShape2List(ShapeList&)
	{
	}

	template<typename TY_Shape, typename ... SHAPE_PACK>
	void AddShape2List(ShapeList& List, TY_Shape& Shape, SHAPE_PACK&& ..SHAPE_PACK)
	{
		List.AddShape(Shape);
		AddShape2List(List);
	}

	template<typename ... SHAPE_PACK>
	ShapeList&& CreateShapeList(iAllocator* Memory, SHAPE_PACK&& ... Shape_Pack)
	{
		ShapeList Out;
		AddShape2List(Out, Shape_Pack...);
		return std::move(Out);
	}
	*/

	class CircleShape final : public ShapeProtoType
	{
	public:
		CircleShape(
			float2	POS_IN, 
			float	Radius, 
			float4	Color_IN		= float4(1.0f), 
			float	AspectRatio_IN	= 16.0f/9.0f,
			size_t	Divisions_IN	= 64) : 
				Color		{Color_IN},
				POS			{POS_IN}, 
				R			{Radius},
				Divisions	{Divisions_IN},
				AspectRatio {AspectRatio_IN}{}

		CircleShape(const CircleShape& rhs):
				Color		{rhs.Color},
				POS			{rhs.POS}, 
				R			{rhs.R},
				Divisions	{rhs.Divisions},
				AspectRatio {rhs.AspectRatio}
		{}

		void AddShapeDraw(
			DrawList&				DrawList, 
			VertexBufferHandle		PushBuffer, 
			ConstantBufferHandle	CB,
			FrameResources&			Resources) override
		{
			size_t VBOffset = Resources.GetVertexBufferOffset(PushBuffer, sizeof(ShapeVert));

			float Step = 2 * pi / Divisions;
			for (size_t I = 0; I < Divisions; ++I)
			{
				float2 V1 = { POS.x + R * cos(Step * (I + 1)),	POS.y - AspectRatio * (R * sin(Step * (I + 1)))};
				float2 V2 = { POS.x + R * cos(Step * I),		POS.y - AspectRatio * (R * sin(Step * I)) };

				PushVertex(ShapeVert{ Position2SS(POS),	{ 0.0f, 1.0f }, Color }, PushBuffer, Resources);
				PushVertex(ShapeVert{ Position2SS(V1),	{ 0.0f, 1.0f }, Color }, PushBuffer, Resources);
				PushVertex(ShapeVert{ Position2SS(V2),	{ 1.0f, 0.0f }, Color }, PushBuffer, Resources);
			}

			Constants CB_Data = {
				Color,
				Color,
				float4x4::Identity()
			};

			auto CBOffset = BeginNewConstantBuffer(CB, Resources);
			PushConstantBufferData(CB_Data, CB, Resources);

			DrawList.push_back({ CBOffset, VBOffset, Divisions * 3});
		}

		float2	POS;
		float4	Color;
		float	R;
		float	AspectRatio;
		size_t	Divisions;
	};


	class LineShape final : public ShapeProtoType
	{
	public:
		LineShape(
			LineSegments& lines
		) : Lines{lines} {}

		LineShape(const LineShape& rhs)
			: Lines{ rhs.Lines }{}

		LineShape(const LineShape&& rhs)
			: Lines{ rhs.Lines }{}

		void AddShapeDraw(
			DrawList&				DrawList,
			VertexBufferHandle		PushBuffer,
			ConstantBufferHandle	CB,
			FrameResources&			Resources) override
		{
			size_t VBOffset = Resources.GetVertexBufferOffset(PushBuffer, sizeof(ShapeVert));

			for (auto Segment : Lines)
			{
				PushVertex(ShapeVert{ Position2SS(Segment.A),{ 0.0f, 0.0f }, Segment.AColour }, PushBuffer, Resources);
				PushVertex(ShapeVert{ Position2SS(Segment.B),{ 0.0f, 0.0f }, Segment.BColour }, PushBuffer, Resources);
			}

			Constants CB_Data = {
				float4{ 0 },
				float4{ 0 },
				float4x4::Identity()
			};

			auto CBOffset = BeginNewConstantBuffer(CB, Resources);
			PushConstantBufferData(CB_Data, CB, Resources);

			DrawList.push_back({ CBOffset, VBOffset, Lines.size() * 2, ShapeDraw::RenderMode::Line });
		}


	private:
		LineSegments& Lines;
	};


	class RectangleShape final : public ShapeProtoType
	{
	public:
		RectangleShape(float2 POS_IN, float2 WH_IN, float4 Color_IN = float4(1.0f)) :
			POS(POS_IN),
			WH(WH_IN),
			Color(Color_IN){}

		RectangleShape(const RectangleShape& rhs) :
			POS		{ rhs.POS		},
			WH		{ rhs.WH		},
			Color	{ rhs.Color		}{}

		void AddShapeDraw(
			DrawList&				DrawList, 
			VertexBufferHandle		PushBuffer, 
			ConstantBufferHandle	CB,
			FrameResources&			Resources) override
		{
			float2 RectUpperLeft	= POS;
			float2 RectBottomRight	= POS + WH;
			float2 RectUpperRight	= { RectBottomRight.x,	RectUpperLeft.y };
			float2 RectBottomLeft	= { RectUpperLeft.x,	RectBottomRight.y };

			size_t VBOffset = Resources.GetVertexBufferOffset(PushBuffer, sizeof(ShapeVert));

			PushVertex(ShapeVert{ Position2SS(RectUpperLeft),	{ 0.0f, 1.0f }, Color }, PushBuffer, Resources);
			PushVertex(ShapeVert{ Position2SS(RectBottomRight),	{ 1.0f, 0.0f }, Color }, PushBuffer, Resources);
			PushVertex(ShapeVert{ Position2SS(RectBottomLeft),	{ 0.0f, 1.0f }, Color }, PushBuffer, Resources);

			PushVertex(ShapeVert{ Position2SS(RectUpperLeft),	{ 0.0f, 1.0f }, Color }, PushBuffer, Resources);
			PushVertex(ShapeVert{ Position2SS(RectUpperRight),	{ 1.0f, 1.0f }, Color }, PushBuffer, Resources);
			PushVertex(ShapeVert{ Position2SS(RectBottomRight),	{ 1.0f, 0.0f }, Color }, PushBuffer, Resources);

			Constants CB_Data = {
				Color,
				Color,
				float4x4::Identity()
			};

			auto CBOffset = BeginNewConstantBuffer(CB, Resources);
			PushConstantBufferData(CB_Data, CB, Resources);

			DrawList.push_back({ CBOffset, VBOffset, 6 });
		}

		float2 POS;
		float2 WH;
		float4 Color;
	};

	inline void AddShapes(
		DrawList&				List, 
		VertexBufferHandle		VertexBuffer,
		ConstantBufferHandle	CB,
		FrameResources&			Resources) {}


	template<typename TY_1, typename ... TY_OTHER_SHAPES>
	void AddShapes(
		DrawList&				List, 
		VertexBufferHandle		VertexBuffer, 
		ConstantBufferHandle	CB,
		FrameResources&			Resources,
		TY_1					Shape, 
		TY_OTHER_SHAPES&& ...	ShapePack)
	{
		Shape.AddShapeDraw(List, VertexBuffer, CB, Resources);
		AddShapes(List, VertexBuffer, CB, Resources, ShapePack...);
	}


	template<typename ... TY_OTHER>
	void DrawShapes(EPIPELINESTATES State, FrameGraph& Graph, VertexBufferHandle PushBuffer, ConstantBufferHandle CB, TextureHandle RenderTarget, iAllocator* Memory, TY_OTHER&& ... Args)
	{
		struct DrawRect
		{
			FrameResourceHandle		RenderTarget;
			VertexBufferHandle		VertexBuffer;
			ConstantBufferHandle	ConstantBuffer;
			DrawList				Draws;
		};

		auto& Pass = Graph.AddNode<DrawRect>(GetCRCGUID(PRESENT),
			[&](FrameGraphNodeBuilder& Builder, DrawRect& Data)
			{
				// Single Thread Section
				// All Rendering Data Must be pushed into buffers here in advance, or allocated in advance
				// for thread safety

				Data.RenderTarget	= Builder.WriteRenderTarget(RenderTarget);
				Data.VertexBuffer	= PushBuffer;
				Data.ConstantBuffer = CB;
				Data.Draws			= DrawList(Memory);

				AddShapes(Data.Draws, PushBuffer, CB, Graph.Resources, Args...);
			},
			[=](const DrawRect& Data, const FrameResources& Resources, Context* Ctx)
			{
				// Multi-threadable Section

				auto WH = Resources.GetRenderTargetWH(Data.RenderTarget);

				Ctx->SetScissorAndViewports({Resources.GetRenderTarget(Data.RenderTarget)});
				Ctx->SetRenderTargets({ (DescHeapPOS)Resources.GetRenderTargetObject(Data.RenderTarget) }, false);

				Ctx->SetRootSignature		(Resources.RenderSystem->Library.RS4CBVs4SRVs);
				Ctx->SetPipelineState		(Resources.GetPipelineState(State));
				Ctx->SetPrimitiveTopology	(EInputTopology::EIT_TRIANGLE);
				Ctx->SetVertexBuffers		(VertexBufferList{ { Data.VertexBuffer, sizeof(ShapeVert)} });

				ShapeDraw::RenderMode PreviousMode = ShapeDraw::RenderMode::Triangle;
				for (auto D : Data.Draws)
				{
					switch (D.Mode) {
						case ShapeDraw::RenderMode::Line:
						{
							Ctx->SetPrimitiveTopology(EInputTopology::EIT_LINE);
						}	break;
						case ShapeDraw::RenderMode::Triangle:
						{
							Ctx->SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
						}	break;
						case ShapeDraw::RenderMode::Textured:
						{
							FK_ASSERT(0, "UNHANDLED CASE!");
						}	break;
					}

					Ctx->SetGraphicsConstantBufferView(2, Data.ConstantBuffer, D.ConstantBufferOffset);
					Ctx->Draw(D.VertexCount, D.VertexBufferOffset);

					PreviousMode = D.Mode;
				}
			});
	} 


}	/************************************************************************************************/
#endif