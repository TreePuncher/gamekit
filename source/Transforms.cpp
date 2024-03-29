#include "XMMathConversion.h"
#include "Transforms.h"
#include "TriggerSlotIDs.hpp"

#include <any>
#include <iostream>

namespace FlexKit
{
	/************************************************************************************************/


	size_t CalculateNodeBufferSize(size_t BufferSize)
	{
		size_t PerNodeFootPrint = sizeof(LT_Entry) + sizeof(WT_Entry) + sizeof(Node) + sizeof(uint16_t);
		return (BufferSize - sizeof(SceneNodes)) / PerNodeFootPrint;
	}

	
	/************************************************************************************************/


	inline uint16_t	_SNHandleToIndex(NodeHandle Node) 
	{
		if (Node == InvalidHandle)
			return 0;

		return SceneNodeTable.Indexes[Node]; 
	}


	/************************************************************************************************/


	inline void		_SNSetHandleIndex(NodeHandle Node, uint16_t index)
	{ 
		SceneNodeTable.Indexes[Node] = index; 
	}


	/************************************************************************************************/


	void InitiateSceneNodeBuffer(iAllocator* persistent)
	{
		SceneNodeTable.Nodes    = { persistent, 2048 };
		SceneNodeTable.LT       = { persistent, 2048 };
		SceneNodeTable.WT       = { persistent, 2048 };
		SceneNodeTable.Flags    = { persistent, 2048 };
		SceneNodeTable.Children = { persistent, 2048 };

		SceneNodeTable.Nodes.reserve(1024);
		SceneNodeTable.LT.reserve(1024);
		SceneNodeTable.WT.reserve(1024);
		SceneNodeTable.Flags.reserve(1024);
		SceneNodeTable.Children.reserve(1024);

		SceneNodeTable.Indexes.Initiate( persistent );

		SceneNodeTable.root = GetZeroedNode();
	}


	/************************************************************************************************/


	void PushAllChildren(size_t CurrentNode, Vector<size_t>& Out)
	{
		for (size_t I = 1; I < SceneNodeTable.size(); ++I)
		{
			auto ParentIndex = _SNHandleToIndex(SceneNodeTable.Nodes[I].Parent);
			if(ParentIndex == CurrentNode)
			{
				Out.push_back(I);
				PushAllChildren(I, Out);
			}
		}
	}

	void SwapNodeEntryies(size_t LHS, size_t RHS)
	{
		LT_Entry	Local_Temp = SceneNodeTable.LT[LHS];
		WT_Entry	World_Temp = SceneNodeTable.WT[LHS];
		Node		Node_Temp  = SceneNodeTable.Nodes[LHS];
		char		Flags_Temp = SceneNodeTable.Flags[LHS];

		SceneNodeTable.LT[LHS]		= SceneNodeTable.LT		[RHS];
		SceneNodeTable.WT[LHS]		= SceneNodeTable.WT		[RHS];
		SceneNodeTable.Nodes[LHS]	= SceneNodeTable.Nodes	[RHS];
		SceneNodeTable.Flags[LHS]	= SceneNodeTable.Flags	[RHS];

		SceneNodeTable.LT[RHS]		= Local_Temp;
		SceneNodeTable.WT[RHS]		= World_Temp;
		SceneNodeTable.Nodes[RHS]	= Node_Temp;
		SceneNodeTable.Flags[RHS]	= Flags_Temp;
	}

	size_t FindFreeIndex()
	{
				size_t FreeIndex	= -1;
		const	size_t end			= SceneNodeTable.Flags.size();

		for (size_t I = 0; I < end; ++I)
		{
			if (SceneNodeTable.Flags[I] & SceneNodes::FREE)
				return I;
		}

		return -1;
	}

	void SortNodes(StackAllocator* Temp)
	{
		#ifdef _DEBUG
			std::cout << "Node Usage Before\n";
			for (size_t I = 0; I < SceneNodeTable.size(); ++I)
				if (SceneNodeTable.Flags[I] & SceneNodes::FREE)
					std::cout << "Node: " << I << " Unused\n";
				else
					std::cout << "Node: " << I << " Used\n";
		#endif

		// Find Order
		if (0 > 1)
		{
			Vector<size_t> Out{ *Temp, SceneNodeTable.LT.size() - 1 };

			for (size_t I = 1; I < SceneNodeTable.LT.size(); ++I)// First Node Is Always Root
			{
				if (SceneNodeTable.Flags[I] & SceneNodes::FREE) {
					size_t II = I + 1;
					for (; II < SceneNodeTable.LT.size(); ++II)
						if (!(SceneNodeTable.Flags[II] & SceneNodes::FREE))
							break;
					
					SwapNodeEntryies(I, II);

					SceneNodeTable.Nodes[I];

					SceneNodeTable.Indexes[NodeHandle(I)]   = (uint16_t)I;
					SceneNodeTable.Indexes[NodeHandle(II)]  = (uint16_t)II;
					//NewLength--;
					int x = 0;
				}
				if(SceneNodeTable.Nodes[I].Parent == NodeHandle(-1))
					continue;

				size_t ParentIndex = _SNHandleToIndex(SceneNodeTable.Nodes[I].Parent);
				if (ParentIndex > I)
				{					
					SwapNodeEntryies(ParentIndex, I);
					SceneNodeTable.Indexes[NodeHandle(ParentIndex)]	= (uint16_t)I;
					SceneNodeTable.Indexes[NodeHandle(I)]			= (uint16_t)ParentIndex;
				}
			}

#ifdef _DEBUG
			std::cout << "Node Usage After\n";
			for (size_t I = 0; I < SceneNodeTable.size(); ++I)
				if (SceneNodeTable.Flags[I] & SceneNodes::FREE)
					std::cout << "Node: " << I << " Unused\n";
				else
					std::cout << "Node: " << I << " Used\n";
#endif
		}
	}


	/************************************************************************************************/

	// TODO: Search an optional Free List
	NodeHandle GetNewNode()
	{
		uint nodeIdx = (uint)SceneNodeTable._AddNode();

		SceneNodeTable.Flags[nodeIdx] = SceneNodes::DIRTY;
		const auto handle = SceneNodeTable.Indexes.GetNewHandle();

		SceneNodeTable.Indexes[handle]          = nodeIdx;
		auto& node  = SceneNodeTable.Nodes[nodeIdx];
		node.handle	= handle;
		node.Parent = SceneNodeTable.root;

		return handle;
	}


	/************************************************************************************************/


	void SwapNodes(NodeHandle lhs, NodeHandle rhs)
	{
		auto lhs_Index = _SNHandleToIndex(lhs);
		auto rhs_Index = _SNHandleToIndex(rhs);
		SwapNodeEntryies(lhs_Index, rhs_Index);

		_SNSetHandleIndex(lhs, rhs_Index);
		_SNSetHandleIndex(rhs, lhs_Index);
	}

	
	/************************************************************************************************/


	void ReleaseNode(NodeHandle handle)
	{
		if (InvalidHandle == handle)
			return;

		SceneNodeTable.Flags[_SNHandleToIndex(handle)] = SceneNodes::FREE;
		SceneNodeTable.Nodes[_SNHandleToIndex(handle)].Parent = NodeHandle(-1);
		_SNSetHandleIndex(handle, -1);
	}

	
	/************************************************************************************************/


	NodeHandle GetParentNode(NodeHandle handle)
	{
		if (handle == FlexKit::InvalidHandle)
			return FlexKit::InvalidHandle;

		const auto idx  = SceneNodeTable.Indexes[handle];
		const auto node = SceneNodeTable.Nodes[idx];

		return node.Parent;
	}


	/************************************************************************************************/


	void SetParentNode(NodeHandle parent, NodeHandle node)
	{
		auto ParentIndex	= _SNHandleToIndex(parent);
		auto ChildIndex		= _SNHandleToIndex(node);

		if (ChildIndex < ParentIndex)
			SwapNodes(parent, node);

		SceneNodeTable.Nodes[SceneNodeTable.Indexes[node]].Parent = parent;

		SetFlag(node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	float3 GetPositionW(NodeHandle node)
	{
		DirectX::XMMATRIX wt;
		GetTransform(node, &wt);

		return float3(
			FlexKit::GetArray_ptr(wt.r[0])[3],
			FlexKit::GetArray_ptr(wt.r[1])[3],
			FlexKit::GetArray_ptr(wt.r[2])[3] );
	}


	/************************************************************************************************/


	float3 GetPositionL(NodeHandle Node)
	{
		auto Local = GetLocal(Node);

		return Local.T;
	}


	/************************************************************************************************/


	void SetPositionW(NodeHandle node, float3 in) // Sets Position in World Space
	{
		DirectX::XMMATRIX wt;
		DirectX::XMMATRIX parentWT;

		GetTransform(node, &wt);
		GetTransform(GetParentNode(node), &parentWT);

		GetArray_ptr(wt.r[0])[3] = in.x;
		GetArray_ptr(wt.r[1])[3] = in.y;
		GetArray_ptr(wt.r[2])[3] = in.z;

		auto tmp = DirectX::XMMatrixInverse(nullptr, parentWT) * wt;
		float3 lPosition = float3(
			FlexKit::GetArray_ptr(tmp.r[0])[3],
			FlexKit::GetArray_ptr(tmp.r[1])[3],
			FlexKit::GetArray_ptr(tmp.r[2])[3] );

		// Set New Local Position
		LT_Entry Local = GetLocal(node);

		FlexKit::GetArray_ptr(Local.T)[0] = lPosition[0];
		FlexKit::GetArray_ptr(Local.T)[1] = lPosition[1];
		FlexKit::GetArray_ptr(Local.T)[2] = lPosition[2];

		SetWT       (node, &wt);
		SetLocal	(node, &Local);
		SetFlag		(node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	void SetPositionL(NodeHandle Node, float3 V)
	{
		auto Local = GetLocal(Node);
		Local.T = V;
		SetLocal	(Node, &Local);
		SetFlag		(Node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	float3 LocalToGlobal(NodeHandle Node, float3 POS)
	{
		float4x4 WT; GetTransform(Node, &WT);
		return (WT * float4(POS, 1)).xyz();
	}


	/************************************************************************************************/


	LT_Entry GetLocal(NodeHandle Node){ 
		return SceneNodeTable.LT[_SNHandleToIndex(Node)];
	}


	/************************************************************************************************/


	float3 GetLocalScale(NodeHandle Node)
	{
		float3 L_s = GetLocal(Node).S;
		return L_s;
	}


	/************************************************************************************************/


	void GetTransform(NodeHandle node, DirectX::XMMATRIX* __restrict out)
	{
		auto index		= _SNHandleToIndex(node);
#if 0
		auto WorldQRS	= Nodes->WT[index];
		DirectX::XMMATRIX wt = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(WorldQRS.World.R) * DirectX::XMMatrixTranslationFromVector(WorldQRS.World.T)); // Seperate this
		*out = wt;
#else
		*out = SceneNodeTable.WT[index].m4x4;
#endif
	}


	/************************************************************************************************/


	float4x4 GetWT(NodeHandle node)
	{
		auto index  = _SNHandleToIndex(node);
		auto WT     = XMMatrixToFloat4x4(SceneNodeTable.WT[index].m4x4);

		return WT;
	}


	/************************************************************************************************/


	void GetTransform(NodeHandle node, float4x4* __restrict out)
	{
		auto index		= _SNHandleToIndex(node);
#if 0
		auto WorldQRS	= Nodes->WT[index];
		DirectX::XMMATRIX wt = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(WorldQRS.World.R) * DirectX::XMMatrixTranslationFromVector(WorldQRS.World.T)); // Seperate this
		*out = wt;
#else
		*out = XMMatrixToFloat4x4(&SceneNodeTable.WT[index].m4x4).Transpose();
#endif
	}


	/************************************************************************************************/

	void SetScale(NodeHandle Node, float3 XYZ)
	{
		LT_Entry Local(GetLocal(Node));

		FlexKit::GetArray_ptr(Local.S)[0] = XYZ[0];
		FlexKit::GetArray_ptr(Local.S)[1] = XYZ[1];
		FlexKit::GetArray_ptr(Local.S)[2] = XYZ[2];

		SetLocal(Node, &Local, SceneNodes::SCALE);
	}


	/************************************************************************************************/


	void SetWT(NodeHandle node, DirectX::XMMATRIX* __restrict In)
	{
		SceneNodeTable.WT[_SNHandleToIndex(node)].m4x4 = *In;
		SetFlag(node, SceneNodes::DIRTY);
	}


	void SetWT(NodeHandle node, const float4x4  in)
	{
		SceneNodeTable.WT[_SNHandleToIndex(node)].m4x4 = Float4x4ToXMMATIRX(in);
		SetFlag(node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	void SetLocal(NodeHandle node, LT_Entry* __restrict In, uint32_t extraFlags)
	{
		auto& lt = SceneNodeTable.LT[_SNHandleToIndex(node)];
		lt = *In;
		SetFlag(node, SceneNodes::StateFlags::DIRTY | extraFlags);
	}


	/************************************************************************************************/


	bool GetFlag(NodeHandle Node, size_t m)
	{
		auto index	= _SNHandleToIndex(Node);
		auto F		= SceneNodeTable.Flags[index];
		return (F & m) != 0;
	}


	/************************************************************************************************/


	uint32_t GetFlags(NodeHandle Node)
	{
		auto index  = _SNHandleToIndex(Node);
		auto F      = SceneNodeTable.Flags[index];
		return F;
	}


	/************************************************************************************************/


	Quaternion GetOrientation(NodeHandle node)
	{
		DirectX::XMMATRIX WT;
		GetTransform(node, &WT);

		DirectX::XMVECTOR q = DirectX::XMQuaternionRotationMatrix(WT);

		return q;
	}

	Quaternion GetOrientationLocal(NodeHandle node)
	{
		auto local = GetLocal(node);

		return local.R;
	}


	/************************************************************************************************/


	void SetOrientation(NodeHandle node, const Quaternion& in)
	{
		DirectX::XMMATRIX wt;
		LT_Entry Local(GetLocal(node));
		GetTransform(FlexKit::GetParentNode(node), &wt);

		const auto transposed = DirectX::XMMatrixTranspose(wt);
		const auto tmp2 = FlexKit::Matrix2Quat(FlexKit::XMMatrixToFloat4x4(&transposed)).Inverse();

		Local.R = DirectX::XMQuaternionMultiply(in, tmp2);
		SetLocal(node, &Local);
		SetFlag(node, SceneNodes::DIRTY);
	}

	
	/************************************************************************************************/


	void SetOrientationL(NodeHandle node, const Quaternion& in)
	{
		LT_Entry Local(GetLocal(node));
		Local.R = in;
		SetLocal(node, &Local);
		SetFlag(node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	bool UpdateTransforms()
	{
		using DirectX::XMMatrixIdentity;
		using DirectX::XMMatrixMultiply;
		using DirectX::XMMatrixTranspose;
		using DirectX::XMMatrixTranslationFromVector;
		using DirectX::XMMatrixScalingFromVector;
		using DirectX::XMMatrixRotationQuaternion;

		SceneNodeTable.WT[0].SetToIdentity();// Making sure root is Identity 
#if 0
		for (size_t itr = 1; itr < Nodes->used; ++itr)// Skip Root
		{
			if ( Nodes->Flags[itr]												| SceneNodes::DIRTY || 
				 Nodes->Flags[_SNHandleToIndex(Nodes, Nodes->Nodes[itr].Parent)]| SceneNodes::DIRTY )
			{
				Nodes->Flags[itr] = Nodes->Flags[itr] | SceneNodes::DIRTY; // To cause Further children to update
				bool			  EnableScale = Nodes->Nodes[itr].Scaleflag;
				FlexKit::WT_Entry Parent;
				FlexKit::WT_Entry WOut;
				FlexKit::LT_Entry LIn;

				// Gather
				GetTransform(Nodes, Nodes->Nodes[itr].Parent, &Parent);

				WOut = Nodes->WT[itr];
				LIn  = Nodes->LT[itr];

				// Calculate
				WOut.World.R = DirectX::XMQuaternionMultiply(LIn.R, Parent.World.R);
				WOut.World.S = DirectX::XMVectorMultiply	(LIn.S, Parent.World.S);

				LIn.T.m128_f32[3] = 0;// Should Always Be Zero
				//if(EnableScale)	WOut.World.T = DirectX::XMVectorMultiply(Parent.World.S, LIn.T);
				WOut.World.T = DirectX::XMQuaternionMultiply(DirectX::XMQuaternionConjugate(Parent.World.R), LIn.T);
				WOut.World.T = DirectX::XMQuaternionMultiply(WOut.World.T, Parent.World.R);
				WOut.World.T = DirectX::XMVectorAdd			(WOut.World.T, Parent.World.T);

				// Write Results
				Nodes->WT[itr] = WOut;
			}
		}
#else

		SceneNodeTable.Flags[0] = SceneNodes::CLEAR;

		size_t Unused_Nodes = 0;
		for (size_t itr = 1; itr < SceneNodeTable.size(); ++itr)
		{
			const auto flag         = SceneNodeTable.Flags[itr];
			const auto parentFlag   = SceneNodeTable.Flags[_SNHandleToIndex(SceneNodeTable.Nodes[itr].Parent)];
			const auto scaleFlag    = flag & SceneNodes::SCALE;


			if((flag & SceneNodes::DIRTY) || (parentFlag & SceneNodes::UPDATED))
			{
				DirectX::XMMATRIX LT = XMMatrixIdentity();
				LT_Entry TRS = GetLocal(SceneNodeTable.Nodes[itr].handle);

				const auto newFlag          = scaleFlag | SceneNodes::UPDATED;
				SceneNodeTable.Flags[itr]   = newFlag;

				bool sf = (SceneNodeTable.Flags[itr] & SceneNodes::StateFlags::SCALE) != 0;
				LT =(	XMMatrixRotationQuaternion(TRS.R) *
						XMMatrixScalingFromVector(sf ? TRS.S : float3(1.0f, 1.0f, 1.0f).pfloats)) *
						XMMatrixTranslationFromVector(TRS.T);

				auto ParentIndex = _SNHandleToIndex(SceneNodeTable.Nodes[itr].Parent);
				auto PT = SceneNodeTable.WT[ParentIndex].m4x4;
				auto WT = XMMatrixTranspose(XMMatrixMultiply(LT, XMMatrixTranspose(PT)));

				auto temp						= SceneNodeTable.WT[itr].m4x4;
				SceneNodeTable.WT[itr].m4x4	= WT;
			}
			else
				SceneNodeTable.Flags[itr] = scaleFlag;

			Unused_Nodes += (SceneNodeTable.Flags[itr] & SceneNodes::FREE);
		}
#endif

		SceneNodeTable.WT[0].SetToIdentity();// Making sure root is Identity 
		return ((float(Unused_Nodes) / float(SceneNodeTable.size())) > 0.25f);
	}


	/************************************************************************************************/


	void UpdateNode(NodeHandle node)
	{
		static_vector<NodeHandle> stack;

		stack.push_back(node);

		const auto root = SceneNodeTable.root;
		for(NodeHandle itr = node; itr != root;)
		{
			auto parent = GetParentNode(itr);
			stack.push_back(parent);
			itr = parent;
		}

		float4x4 transform = float4x4::Identity();

		for (auto itr = stack.rbegin(); itr != stack.rend(); itr++)
		{
			const float4x4 parent = GetWT(*itr);
			transform = parent * transform;
		}

		SetWT(node, transform);
	}


	/************************************************************************************************/


	void SceneNodeComponent::FreeComponentView(void* _ptr)
	{
		static_cast<SceneNodeView*>(_ptr)->Release();
	}


	/************************************************************************************************/


	SceneNodeView::SceneNodeView(GameObject& gameObject, const float3 XYZ) :
		node{ GetComponent().CreateNode() }
	{
		auto& triggerView = gameObject.AddView<TriggerView>();
		triggerView->CreateTrigger(ParentChangeSignalID);
		triggerView->CreateTrigger(TranslationSignalID);
		triggerView->CreateTrigger(SetOrientationmSignalID);
		triggerView->CreateTrigger(SetTransformSignalID);

		triggers = triggerView.handle;

		SetPosition(XYZ);
	}


	/************************************************************************************************/


	SceneNodeView::SceneNodeView(GameObject& gameObject, NodeHandle IN_Node) :
		node{ IN_Node }
	{
		auto& triggerView = gameObject.AddView<TriggerView>();
		triggerView->CreateTrigger(ParentChangeSignalID);
		triggerView->CreateTrigger(TranslationSignalID);
		triggerView->CreateTrigger(SetOrientationmSignalID);
		triggerView->CreateTrigger(SetTransformSignalID);

		triggers = triggerView.handle;
	}


	/************************************************************************************************/


	void SceneNodeView::Release()
	{
		if (node != InvalidHandle)
			ReleaseNode(node);

		node = InvalidHandle;
	}


	/************************************************************************************************/


	SceneNodeView::SceneNodeView(SceneNodeView&& rhs)
	{
		node			= rhs.node;
		triggers		= rhs.triggers;
		triggerEnable	= rhs.triggerEnable;

		rhs.node			= InvalidHandle;
		rhs.triggers		= InvalidHandle;
		rhs.triggerEnable	= false;
	}


	/************************************************************************************************/


	SceneNodeView& SceneNodeView::operator = (SceneNodeView&& rhs)
	{
		node			= rhs.node;
		triggers		= rhs.triggers;
		triggerEnable	= rhs.triggerEnable;

		rhs.node			= InvalidHandle;
		rhs.triggers		= InvalidHandle;
		rhs.triggerEnable	= false;

		return *this;
	}


	/************************************************************************************************/


	SceneNodeView::operator NodeHandle () const noexcept { return node; }


	/************************************************************************************************/


	NodeHandle SceneNodeView::GetParentNode() const
	{
		return FlexKit::GetParentNode(node);
	}


	/************************************************************************************************/


	void SceneNodeView::SetParentNode(NodeHandle parent) noexcept
	{
		FlexKit::SetParentNode(parent, node);

		TriggerComponent::GetComponent()[triggers].Trigger(ParentChangeSignalID, &parent, GetTypeGUID(NodeHandle));
	}


	void SceneNodeView::Yaw(float r) noexcept
	{
		FlexKit::Yaw(node, r);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(SetOrientationmSignalID, &r, GetTypeGUID(float));
	}


	void SceneNodeView::Roll(float r) noexcept
	{
		FlexKit::Roll(node, r);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(SetOrientationmSignalID, &r, GetTypeGUID(float));
	}


	void SceneNodeView::Pitch(float r) noexcept
	{
		FlexKit::Pitch(node, r);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(SetOrientationmSignalID, &r, GetTypeGUID(float));
	}

	void SceneNodeView::Scale(float3 xyz) noexcept
	{
		FlexKit::Scale(node, xyz);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(SetScaleSignalID);
	}


	void SceneNodeView::TranslateLocal(float3 xyz) noexcept
	{
		FlexKit::TranslateLocal(node, xyz);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(TranslationSignalID);
	}


	void SceneNodeView::TranslateWorld(float3 xyz) noexcept
	{
		FlexKit::TranslateWorld(node, xyz);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(TranslationSignalID, {});
	}


	void SceneNodeView::ToggleScaling(bool scalable) noexcept
	{
		SetFlag(node, SceneNodes::SCALE);
	}


	float3	SceneNodeView::GetPosition() const noexcept
	{
		return FlexKit::GetPositionW(node);
	}

	float3	SceneNodeView::GetPositionL() const noexcept
	{
		return FlexKit::GetPositionL(node);
	}

	float3	SceneNodeView::GetScale() const noexcept
	{
		return GetLocalScale(node);
	}

	Quaternion SceneNodeView::GetOrientation() const noexcept
	{

		return FlexKit::GetOrientation(node);
	}

	Quaternion SceneNodeView::GetOrientationL() const noexcept
	{

		return FlexKit::GetOrientationLocal(node);
	}

	float4x4 SceneNodeView::GetWT() const noexcept
	{
		return FlexKit::GetWT(node);
	}

	void SceneNodeView::SetScale(float3 scale) noexcept
	{
		FlexKit::SetScale(node, scale);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(SetScaleSignalID);
	}

	void SceneNodeView::SetPosition(const float3 xyz) noexcept
	{
		FlexKit::SetPositionW(node, xyz);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(TranslationSignalID);
	}

	void SceneNodeView::SetPositionL(const float3 xyz) noexcept
	{
		FlexKit::SetPositionL(node, xyz);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(TranslationSignalID);
	}

	void SceneNodeView::SetOrientation(const Quaternion q) noexcept
	{
		FlexKit::SetOrientation(node, q);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(SetOrientationmSignalID, (void*)&q, GetTypeGUID(Quaternion));
	}

	void SceneNodeView::SetOrientationL(const Quaternion q) noexcept
	{
		FlexKit::SetOrientationL(node, q);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(SetOrientationmSignalID, (void*)&q, GetTypeGUID(Quaternion));
	}

	void SceneNodeView::SetWT(const float4x4& wt) noexcept
	{
		FlexKit::SetWT(node, wt);

		if (triggerEnable)
			TriggerComponent::GetComponent()[triggers].Trigger(SetTransformSignalID, (void*)&wt, GetTypeGUID(float4x4));
	}


	/************************************************************************************************/


	NodeHandle ZeroNode(NodeHandle node)
	{
		FlexKit::LT_Entry LT;
		LT.S = DirectX::XMVectorSet(1, 1, 1, 1);
		LT.R = DirectX::XMQuaternionIdentity();
		LT.T = DirectX::XMVectorZero();
		FlexKit::SetLocal(node, &LT);

		DirectX::XMMATRIX WT;
		WT = DirectX::XMMatrixIdentity();
		FlexKit::SetWT(node, &WT);

		SceneNodeTable.Nodes[_SNHandleToIndex(node)].Scaleflag  = false;
		SceneNodeTable.Nodes[_SNHandleToIndex(node)].Parent     = NodeHandle(0);

		return node;
	}


	/************************************************************************************************/


	NodeHandle	GetZeroedNode()
	{
		return ZeroNode(GetNewNode());
	}


	/************************************************************************************************/


	void SetFlag(NodeHandle node, uint32_t f)
	{
		auto index = _SNHandleToIndex(node);
		SceneNodeTable.Flags[index] = SceneNodeTable.Flags[index] | f;
	}


	/************************************************************************************************/


	void Scale(NodeHandle node, float3 XYZ)
	{
		LT_Entry Local(GetLocal(node));

		FlexKit::GetArray_ptr(Local.S)[0] *= XYZ[0];
		FlexKit::GetArray_ptr(Local.S)[1] *= XYZ[1];
		FlexKit::GetArray_ptr(Local.S)[2] *= XYZ[2];

		SetLocal(node, &Local);
	}


	/************************************************************************************************/


	void TranslateLocal(NodeHandle node, float3 XYZ)
	{
		LT_Entry Local(GetLocal(node));

		FlexKit::GetArray_ptr(Local.T)[0] += XYZ[0];
		FlexKit::GetArray_ptr(Local.T)[1] += XYZ[1];
		FlexKit::GetArray_ptr(Local.T)[2] += XYZ[2];

		SetLocal(node, &Local);
	}


	/************************************************************************************************/


	void TranslateWorld(NodeHandle Node, float3 XYZ)
	{
		DirectX::XMMATRIX WT;
		GetTransform(GetParentNode(Node), &WT);

		auto MI = DirectX::XMMatrixInverse(nullptr, WT);
		auto V	= DirectX::XMVector4Transform(XYZ.pfloats, MI);
		TranslateLocal(Node, float3(V));
	}


	/************************************************************************************************/


	void Yaw(NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		FlexKit::GetArray_ptr(rot)[0] = 0;
		FlexKit::GetArray_ptr(rot)[1] = std::sin(r / 2);
		FlexKit::GetArray_ptr(rot)[2] = 0;
		FlexKit::GetArray_ptr(rot)[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);
		Local.R = DirectX::XMQuaternionNormalize(Local.R);

		FlexKit::SetLocal(Node, &Local);
	}


	/************************************************************************************************/


	void Pitch(NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		FlexKit::GetArray_ptr(rot)[0] = std::sin(r / 2);;
		FlexKit::GetArray_ptr(rot)[1] = 0;
		FlexKit::GetArray_ptr(rot)[2] = 0;
		FlexKit::GetArray_ptr(rot)[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);

		FlexKit::SetLocal(Node, &Local);
	}


	/************************************************************************************************/


	void Roll(NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		FlexKit::GetArray_ptr(rot)[0] = 0;
		FlexKit::GetArray_ptr(rot)[1] = 0;
		FlexKit::GetArray_ptr(rot)[2] = std::sin(r / 2);
		FlexKit::GetArray_ptr(rot)[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);

		FlexKit::SetLocal(Node, &Local);
	}


	/************************************************************************************************/


	void Translate(GameObject& go, const float3 xyz)
	{
		Apply(go,
			[&](SceneNodeView& node)
			{
				node.TranslateWorld(xyz);
			});
	}


	/************************************************************************************************/

	float3 GetLocalPosition(GameObject& go)
	{
		return Apply(go, 
			[&](SceneNodeView& node)
			{	return node.GetPositionL(); }, 
			[]
			{ return float3{ 0, 0, 0 }; });
	}


	/************************************************************************************************/


	float3 GetWorldPosition(GameObject& go)
	{
		return Apply(go,
			[&](SceneNodeView& node)
			{	return node.GetPosition(); },
			[]
			{ return float3{ 0, 0, 0 }; });
	}


	/************************************************************************************************/


	void SetWorldPosition(GameObject& go, const float3 pos)
	{
		return Apply(go,
			[&](SceneNodeView& view)
			{   SetPositionW(view.node, pos); });
	}


	/************************************************************************************************/


	void SetLocalPosition(GameObject& go, const float3 pos)
	{
		return Apply(go,
			[&](SceneNodeView& view)
			{   SetPositionL(view.node, pos); });
	}


	/************************************************************************************************/


	float3 GetScale(GameObject& go)
	{
		return Apply(go,
			[&](SceneNodeView& node)
			{ return node.GetScale(); },
			[]
			{ return float3{ 1, 1, 1 }; });
	}

	float3 GetScale(NodeHandle node)
	{
		const float4x4 WT				= GetWT(node);
		const Quaternion Q				= Matrix2Quat(WT);
		const float4x4 InverseRotation	= Quaternion2Matrix(Q.Inverse());
		const float4x4 scaleMatrix		= InverseRotation * WT;

		return { scaleMatrix[0][0], scaleMatrix[1][1], scaleMatrix[2][2] };
	}


	/************************************************************************************************/


	void ClearParent(GameObject& go)
	{
		return Apply(go,
			[&](SceneNodeView& node)
			{
				return node.SetParentNode(node.GetComponent().GetRoot());
			}
		);
	}


	/************************************************************************************************/


	NodeHandle GetParentNode(GameObject& go)
	{
		return Apply(go,
			[&](const SceneNodeView& node) -> NodeHandle
			{
				return node.GetParentNode();
			},
			[]() -> NodeHandle
			{
				return InvalidHandle;
			}
		);
	}

	void SetParentNode(GameObject& go, NodeHandle node)
	{
		return Apply(go,
			[&](SceneNodeView& nodeView)
			{
				nodeView.SetParentNode(node);
			});
	}


	/************************************************************************************************/


	void EnableScale(GameObject& go, bool scale)
	{
		return Apply(go,
			[&](SceneNodeView& node)
			{
				return node.ToggleScaling(scale);
			});
	}


	/************************************************************************************************/


	void SetScale(GameObject& go, float3 scale)
	{
		return Apply(go,
			[&](SceneNodeView& node)
			{
				return node.SetScale(scale);
			}
			);
	}


	/************************************************************************************************/


	void Pitch(GameObject& go, float theta)
	{
		return Apply(go,
			[&](SceneNodeView& node)
			{
				return node.Pitch(theta);
			}
		);
	}


	/************************************************************************************************/


	void Yaw(GameObject& go, float theta)
	{
		return Apply(go,
			[&](SceneNodeView& node)
			{
				return node.Yaw(theta);
			}
		);
	}


	/************************************************************************************************/


	Quaternion GetOrientation(GameObject& go)
	{
		return Apply(go,
			[&](const SceneNodeView& node)
			{ return node.GetOrientation(); },
			[]
			{ return Quaternion{ 0, 0, 0, 1 }; });
	}

	Quaternion	GetOrientationLocal(GameObject& go)
	{
		return Apply(go,
			[&](const SceneNodeView& node)
			{ return node.GetOrientationL(); },
			[]
			{ return Quaternion{ 0, 0, 0, 1 }; });
	}


	/************************************************************************************************/


	NodeHandle GetSceneNode(GameObject& go)
	{
		return Apply(
			go,
			[](SceneNodeView& node)
			{
				return node.node;
			},
			[]
			{
				return NodeHandle(InvalidHandle);
			});
	}


	/************************************************************************************************/


	float4x4 GetWT(GameObject& go)
	{
		return GetWT(GetSceneNode(go));
	}


	/************************************************************************************************/


	void SetWT(GameObject& go, const float4x4 newMatrix)
	{
		auto* sceneNodeView		= go.GetView<SceneNodeView>();
		if (!sceneNodeView)
			return;

		const auto node			= sceneNodeView->node;
		const auto parentWT		= GetWT(FlexKit::GetParentNode(node));

		auto PI			= Inverse(parentWT);
		auto localT		= newMatrix * PI.Transpose();

		FlexKit::LT_Entry local	= FlexKit::GetLocal(node);
		local.R					= Matrix2Quat(localT);
		local.T					= FlexKit::Vect4ToFloat4(localT[3]).xyz();

		FlexKit::SetLocal(node, &local);
		FlexKit::SetFlag(node, SceneNodes::StateFlags::UPDATED);

		if(sceneNodeView->triggerEnable)
			Trigger(go, SetTransformSignalID, (void*)&newMatrix, GetTypeGUID(float4x4));
	}


	/************************************************************************************************/


	void SetOrientation(GameObject& go, const Quaternion q)
	{
		return Apply(go,
			[&](SceneNodeView& view)
			{ SetOrientation(view.node, q); });
	}


	/************************************************************************************************/


	void SetOrientationLocal(GameObject& go, const Quaternion q)
	{
		return Apply(go,
			[&](SceneNodeView& view)
			{ SetOrientation(view.node, q); });
	}



}   /************************************************************************************************/



/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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

