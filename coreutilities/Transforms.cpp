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


#include "Transforms.h"
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


	void InitiateSceneNodeBuffer(SceneNodes* Nodes, byte* pmem, size_t MemSize)
	{
		size_t NodeFootPrint = sizeof(SceneNodes::BOILERPLATE);
		size_t NodeMax = (MemSize - sizeof(SceneNodes)) / NodeFootPrint  - 0x20;

		Nodes->Nodes = (Node*)pmem;
		{
			const size_t aligment = 0x10;
			auto Memory = pmem + NodeMax;
			size_t alignoffset = (size_t)Memory % aligment;
			if(alignoffset)
				Memory += aligment - alignoffset;	// 16 Byte byte Align
			Nodes->LT = (LT_Entry*)(Memory);
			int c = 0; // Debug Point
		}
		{
			const size_t aligment = 0x40;
			char* Memory = (char*)Nodes->LT + NodeMax;
			size_t alignoffset = (size_t)Memory % aligment; // Cache Align
			if(alignoffset)
				Memory += aligment - alignoffset;
			Nodes->WT = (WT_Entry*)(Memory);
			int c = 0; // Debug Point
		}
		{
			Nodes->Flags = (char*)(Nodes->WT + NodeMax);
			for (size_t I = 0; I < NodeMax; ++I)
				Nodes->Flags[I] = SceneNodes::FREE;

			int c = 0; // Debug Point
		}
		{
			Nodes->Indexes = (uint16_t*)(Nodes->Flags + NodeMax);
			for (size_t I = 0; I < NodeMax; ++I)
				Nodes->Indexes[I] = 0xffff;

			int c = 0; // Debug Point
		}

		for (size_t I = 0; I < NodeMax; ++I)
		{
			Nodes->LT[I].R = DirectX::XMQuaternionIdentity();
			Nodes->LT[I].S = DirectX::XMVectorSet(1, 1, 1, 1);
			Nodes->LT[I].T = DirectX::XMVectorSet(0, 0, 0, 0);
		}

		for (size_t I = 0; I < NodeMax; ++I)
			Nodes->WT[I].m4x4 = DirectX::XMMatrixIdentity();

		Nodes->used		= 0;
		Nodes->max		= NodeMax;
	}


	/************************************************************************************************/


	void PushAllChildren(SceneNodes* Nodes, size_t CurrentNode, fixed_vector<size_t>& Out)
	{
		for (size_t I = 1; I < Nodes->used; ++I)
		{
			auto ParentIndex = _SNHandleToIndex(Nodes, Nodes->Nodes[I].Parent);
			if(ParentIndex == CurrentNode)
			{
				Out.push_back(I);
				PushAllChildren(Nodes, I, Out);
			}
		}
	}

	void SwapNodeEntryies(SceneNodes* Nodes, size_t LHS, size_t RHS)
	{
		LT_Entry	Local_Temp = Nodes->LT[LHS];
		WT_Entry	World_Temp = Nodes->WT[LHS];
		Node		Node_Temp  = Nodes->Nodes[LHS];
		char		Flags_Temp = Nodes->Flags[LHS];

		Nodes->LT[LHS]		= Nodes->LT		[RHS];
		Nodes->WT[LHS]		= Nodes->WT		[RHS];
		Nodes->Nodes[LHS]	= Nodes->Nodes	[RHS];
		Nodes->Flags[LHS]	= Nodes->Flags	[RHS];

		Nodes->LT[RHS]		= Local_Temp;
		Nodes->WT[RHS]		= World_Temp;
		Nodes->Nodes[RHS]	= Node_Temp;
		Nodes->Flags[RHS]	= Flags_Temp;
	}

	size_t FindFreeIndex(SceneNodes* Nodes)
	{
		size_t FreeIndex = -1;
		for (size_t I = Nodes->used - 1; I >= 0; ++I)
		{
			if (Nodes->Flags[I] & SceneNodes::FREE) {
				FreeIndex = I;
				break;
			}
		}
		return FreeIndex;
	}

	void SortNodes(SceneNodes* Nodes, StackAllocator* Temp)
	{
		#ifdef _DEBUG
			std::cout << "Node Usage Before\n";
			for (size_t I = 0; I < Nodes->used; ++I)
				if (Nodes->Flags[I] & SceneNodes::FREE)
					std::cout << "Node: " << I << " Unused\n";
				else
					std::cout << "Node: " << I << " Used\n";
		#endif

		// Find Order
		if (Nodes->used > 1)
		{
			size_t NewLength = Nodes->used;
			fixed_vector<size_t>& Out = fixed_vector<size_t>::Create(Nodes->used - 1, Temp);
			for (size_t I = 1; I < Nodes->used; ++I)// First Node Is Always Root
			{
				if (Nodes->Flags[I] & SceneNodes::FREE) {
					size_t II = I + 1;
					for (; II < Nodes->used; ++II)
						if (!(Nodes->Flags[II] & SceneNodes::FREE))
							break;
					
					SwapNodeEntryies(Nodes, I, II);
					Nodes->Indexes[I] = I;
					Nodes->Indexes[II]= II;
					NewLength--;
					int x = 0;
				}
				if(Nodes->Nodes[I].Parent == NodeHandle(-1))
					continue;

				size_t ParentIndex = _SNHandleToIndex(Nodes, Nodes->Nodes[I].Parent);
				if (ParentIndex > I)
				{					
					SwapNodeEntryies(Nodes, ParentIndex, I);
					Nodes->Indexes[ParentIndex] = I;
					Nodes->Indexes[I]			= ParentIndex;
				}
			}
			Nodes->used = NewLength + 1;
#ifdef _DEBUG
			std::cout << "Node Usage After\n";
			for (size_t I = 0; I < Nodes->used; ++I)
				if (Nodes->Flags[I] & SceneNodes::FREE)
					std::cout << "Node: " << I << " Unused\n";
				else
					std::cout << "Node: " << I << " Used\n";
#endif
		}
	}


	/************************************************************************************************/

	// TODO: Search an optional Free List
	NodeHandle GetNewNode(SceneNodes* Nodes )
	{
		if (Nodes->max < Nodes->used)
			FK_ASSERT(0);

		auto HandleIndex	= 0;
		auto NodeIndex		= 0;

		{
			size_t itr = 0;
			size_t end = Nodes->max;
			for (; itr < end; ++itr)
			{
				if (Nodes->Indexes[itr] == 0xffff)
					break;
			}
			NodeIndex = itr;

			itr = 0;
			for (; itr < end; ++itr)
			{
				if (Nodes->Flags[itr] & SceneNodes::FREE) break;
			}

			HandleIndex = itr;
		}

		Nodes->Flags[NodeIndex] = SceneNodes::DIRTY;
		auto node = NodeHandle(HandleIndex);

		Nodes->Indexes[HandleIndex] = NodeIndex;
		Nodes->Nodes[node.INDEX].TH	= node;
		Nodes->used++;

		return node;
	}


	/************************************************************************************************/


	void SwapNodes(SceneNodes* Nodes, NodeHandle lhs, NodeHandle rhs)
	{
		auto lhs_Index = _SNHandleToIndex(Nodes, lhs);
		auto rhs_Index = _SNHandleToIndex(Nodes, rhs);
		SwapNodeEntryies(Nodes, lhs_Index, rhs_Index);

		_SNSetHandleIndex(Nodes, lhs, rhs_Index);
		_SNSetHandleIndex(Nodes, rhs, lhs_Index);
	}

	
	/************************************************************************************************/


	void ReleaseNode(SceneNodes* Nodes, NodeHandle handle)
	{
		Nodes->Flags[_SNHandleToIndex(Nodes, handle)] = SceneNodes::FREE;
		Nodes->Nodes[_SNHandleToIndex(Nodes, handle)].Parent = NodeHandle(-1);
		_SNSetHandleIndex(Nodes, handle, -1);
	}

	
	/************************************************************************************************/


	NodeHandle GetParentNode(SceneNodes* Nodes, NodeHandle node )
	{
		return Nodes->Nodes[Nodes->Indexes[node.INDEX]].Parent;
	}


	/************************************************************************************************/


	void SetParentNode(SceneNodes* Nodes, NodeHandle parent, NodeHandle node)
	{
		auto ParentIndex	= _SNHandleToIndex(Nodes, parent);
		auto ChildIndex		= _SNHandleToIndex(Nodes, node);

		if (ChildIndex < ParentIndex)
			SwapNodes(Nodes, parent, node);

		Nodes->Nodes[Nodes->Indexes[node.INDEX]].Parent = parent;
		SetFlag(Nodes, node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	float3 GetPositionW(SceneNodes* Nodes, NodeHandle node)
	{
		DirectX::XMMATRIX wt;
		GetWT(Nodes, node, &wt);
		return float3( wt.r[0].m128_f32[3], wt.r[1].m128_f32[3], wt.r[2].m128_f32[3] );
	}


	/************************************************************************************************/


	float3 GetPositionL(SceneNodes* Nodes, NodeHandle Node)
	{
		auto Local = GetLocal(Nodes, Node);
		return Local.T;
	}


	/************************************************************************************************/


	void SetPositionW(SceneNodes* Nodes, NodeHandle node, float3 in) // Sets Position in World Space
	{
#if 0
		using DirectX::XMQuaternionConjugate;
		using DirectX::XMQuaternionMultiply;
		using DirectX::XMVectorSubtract;

		// Set New Local Position
		LT_Entry Local;
		WT_Entry Parent;

		// Gather
		GetLocal(Nodes, node, &Local);
		GetWT(Nodes, GetParentNode(Nodes, node), &Parent);

		// Calculate
		Local.T = XMVectorSubtract( Parent.World.T, in.pfloats );
		SetLocal(Nodes, node, &Local);
#else
		DirectX::XMMATRIX wt;
		GetWT(Nodes, node, &wt);
		auto tmp =  DirectX::XMMatrixInverse(nullptr, wt) * DirectX::XMMatrixTranslation(in[0], in[1], in[2] );
		float3 lPosition = float3( tmp.r[3].m128_f32[0], tmp.r[3].m128_f32[1], tmp.r[3].m128_f32[2] );

		wt.r[0].m128_f32[3] = in.x;
		wt.r[1].m128_f32[3] = in.y;
		wt.r[2].m128_f32[3] = in.z;

		SetWT(Nodes, node, &wt);
		// Set New Local Position
		LT_Entry Local(GetLocal(Nodes, node));

		Local.T.m128_f32[0] = lPosition[0];
		Local.T.m128_f32[1] = lPosition[1];
		Local.T.m128_f32[2] = lPosition[2];

		SetLocal(Nodes, node, &Local);
		SetFlag(Nodes, node, SceneNodes::DIRTY);
#endif
	}


	/************************************************************************************************/


	void SetPositionL(SceneNodes* Nodes, NodeHandle Node, float3 V)
	{
		auto Local = GetLocal(Nodes, Node);
		Local.T = V;
		SetLocal(Nodes, Node, &Local);
		SetFlag(Nodes, Node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	float3 LocalToGlobal(SceneNodes* Nodes, NodeHandle Node, float3 POS)
	{
		float4x4 WT; GetWT(Nodes, Node, &WT);
		return (WT * float4(POS, 1)).xyz();
	}


	/************************************************************************************************/


	LT_Entry GetLocal(SceneNodes* Nodes, NodeHandle Node){ 
		return Nodes->LT[_SNHandleToIndex(Nodes, Node)]; 
	}


	/************************************************************************************************/


	float3 GetLocalScale(SceneNodes* Nodes, NodeHandle Node)
	{
		float3 L_s = GetLocal(Nodes, Node).S;
		return L_s;
	}


	/************************************************************************************************/


	void GetWT(SceneNodes* Nodes, NodeHandle node, DirectX::XMMATRIX* __restrict out)
	{
		auto index		= _SNHandleToIndex(Nodes, node);
#if 0
		auto WorldQRS	= Nodes->WT[index];
		DirectX::XMMATRIX wt = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(WorldQRS.World.R) * DirectX::XMMatrixTranslationFromVector(WorldQRS.World.T)); // Seperate this
		*out = wt;
#else
		*out = Nodes->WT[index].m4x4;
#endif
	}


	/************************************************************************************************/


	void GetWT(SceneNodes* Nodes, NodeHandle node, float4x4* __restrict out)
	{
		auto index		= _SNHandleToIndex(Nodes, node);
#if 0
		auto WorldQRS	= Nodes->WT[index];
		DirectX::XMMATRIX wt = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(WorldQRS.World.R) * DirectX::XMMatrixTranslationFromVector(WorldQRS.World.T)); // Seperate this
		*out = wt;
#else
		*out = XMMatrixToFloat4x4(&Nodes->WT[index].m4x4).Transpose();
#endif
	}


	/************************************************************************************************/
	
	
	void GetWT(SceneNodes* Nodes, NodeHandle node, WT_Entry* __restrict out )
	{
		*out = Nodes->WT[_SNHandleToIndex(Nodes, node)];
	}


	/************************************************************************************************/

	void SetScale(SceneNodes* Nodes, NodeHandle Node, float3 XYZ)
	{
		LT_Entry Local(GetLocal(Nodes, Node));

		Local.S.m128_f32[0] *= XYZ.pfloats.m128_f32[0];
		Local.S.m128_f32[1] *= XYZ.pfloats.m128_f32[1];
		Local.S.m128_f32[2] *= XYZ.pfloats.m128_f32[2];

		SetLocal(Nodes, Node, &Local);
	}


	/************************************************************************************************/


	void SetWT(SceneNodes* Nodes, NodeHandle node, DirectX::XMMATRIX* __restrict In)
	{
		Nodes->WT[_SNHandleToIndex(Nodes, node)].m4x4 = *In;
		SetFlag(Nodes, node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	void SetLocal(SceneNodes* Nodes, NodeHandle node, LT_Entry* __restrict In)
	{
		Nodes->LT[_SNHandleToIndex(Nodes, node)] = *In;
		SetFlag(Nodes, node, SceneNodes::StateFlags::DIRTY);
	}


	/************************************************************************************************/


	bool GetFlag(SceneNodes* Nodes, NodeHandle Node, size_t m)
	{
		auto index = _SNHandleToIndex(Nodes, Node);
		auto F = Nodes->Flags[index];
		return (F & m) != 0;
	}


	/************************************************************************************************/


	Quaternion GetOrientation(SceneNodes* Nodes, NodeHandle node)
	{
		DirectX::XMMATRIX WT;
		GetWT(Nodes, node, &WT);

		DirectX::XMVECTOR q = DirectX::XMQuaternionRotationMatrix(WT);

		return q;
	}


	/************************************************************************************************/


	void SetOrientation(SceneNodes* Nodes, NodeHandle node, Quaternion& in)
	{
		DirectX::XMMATRIX wt;
		LT_Entry Local(GetLocal(Nodes, node));
		GetWT(Nodes, FlexKit::GetParentNode(Nodes, node), &wt);

		auto tmp2 = FlexKit::Matrix2Quat(FlexKit::XMMatrixToFloat4x4(&DirectX::XMMatrixTranspose(wt)));
		tmp2 = tmp2.Inverse();

		Local.R = DirectX::XMQuaternionMultiply(in, tmp2);
		SetLocal(Nodes, node, &Local);
		SetFlag(Nodes, node, SceneNodes::DIRTY);
	}

	
	/************************************************************************************************/


	void SetOrientationL(SceneNodes* Nodes, NodeHandle node, Quaternion& in)
	{
		LT_Entry Local(GetLocal(Nodes, node));
		Local.R = in;
		SetLocal(Nodes, node, &Local);
		SetFlag(Nodes, node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	bool UpdateTransforms(SceneNodes* Nodes)
	{
		using DirectX::XMMatrixIdentity;
		using DirectX::XMMatrixMultiply;
		using DirectX::XMMatrixTranspose;
		using DirectX::XMMatrixTranslationFromVector;
		using DirectX::XMMatrixScalingFromVector;
		using DirectX::XMMatrixRotationQuaternion;

		Nodes->WT[0].SetToIdentity();// Making sure root is Identity 
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
				GetWT(Nodes, Nodes->Nodes[itr].Parent, &Parent);

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
		for (size_t itr = 1; itr < Nodes->used; ++itr)
			if (Nodes->Flags[itr] | SceneNodes::UPDATED && !(Nodes->Flags[itr] | SceneNodes::DIRTY))
				Nodes->Flags[itr] ^= SceneNodes::CLEAR;

		size_t Unused_Nodes = 0;;
		for (size_t itr = 1; itr < Nodes->used; ++itr)
		{
			if (!(Nodes->Flags[itr] & SceneNodes::FREE)  && 
				 (Nodes->Flags[itr] | SceneNodes::DIRTY) ||
				  Nodes->Flags[_SNHandleToIndex(Nodes, Nodes->Nodes[itr].Parent)] | SceneNodes::UPDATED)
			{
				Nodes->Flags[itr] ^= SceneNodes::DIRTY;		
				Nodes->Flags[itr] |= SceneNodes::UPDATED; // Propagates Dirty Status of Panret to Children to update

				DirectX::XMMATRIX LT = XMMatrixIdentity();
				LT_Entry TRS = GetLocal(Nodes, Nodes->Nodes[itr].TH);

				bool sf = (Nodes->Flags[itr] & SceneNodes::StateFlags::SCALE) != 0;
				LT =(	XMMatrixRotationQuaternion(TRS.R) *
						XMMatrixScalingFromVector(sf ? TRS.S : float3(1.0f, 1.0f, 1.0f).pfloats)) *
						XMMatrixTranslationFromVector(TRS.T);

				auto ParentIndex = _SNHandleToIndex(Nodes, Nodes->Nodes[itr].Parent);
				auto PT = Nodes->WT[ParentIndex].m4x4;
				auto WT = XMMatrixTranspose(XMMatrixMultiply(LT, XMMatrixTranspose(PT)));

				auto temp	= Nodes->WT[itr].m4x4;
				Nodes->WT[itr].m4x4 = WT;
			}

			Unused_Nodes += (Nodes->Flags[itr] & SceneNodes::FREE);
		}
#endif

		Nodes->WT[0].SetToIdentity();// Making sure root is Identity 
		return ((float(Unused_Nodes) / float(Nodes->used)) > 0.25f);
	}


	/************************************************************************************************/


	NodeHandle ZeroNode(SceneNodes* Nodes, NodeHandle node)
	{
		FlexKit::LT_Entry LT;
		LT.S = DirectX::XMVectorSet(1, 1, 1, 1);
		LT.R = DirectX::XMQuaternionIdentity();
		LT.T = DirectX::XMVectorZero();
		FlexKit::SetLocal(Nodes, node, &LT);

		DirectX::XMMATRIX WT;
		WT = DirectX::XMMatrixIdentity();
		FlexKit::SetWT(Nodes, node, &WT);

		Nodes->Nodes[_SNHandleToIndex(Nodes, node)].Scaleflag = false;
		Nodes->Nodes[_SNHandleToIndex(Nodes, node)].Parent = NodeHandle(0);

		return node;
	}


	/************************************************************************************************/


	NodeHandle	GetZeroedNode(SceneNodes* nodes)
	{
		return ZeroNode(nodes, GetNewNode(nodes));
	}


	/************************************************************************************************/


	void SetFlag(SceneNodes* Nodes, NodeHandle node, SceneNodes::StateFlags f)
	{
		auto index = _SNHandleToIndex(Nodes, node);
		Nodes->Flags[index] = Nodes->Flags[index] | f;
	}


	/************************************************************************************************/


	void Scale(SceneNodes* Nodes, NodeHandle node, float3 XYZ)
	{
		LT_Entry Local(GetLocal(Nodes, node));

		Local.S.m128_f32[0] *= XYZ.pfloats.m128_f32[0];
		Local.S.m128_f32[1] *= XYZ.pfloats.m128_f32[1];
		Local.S.m128_f32[2] *= XYZ.pfloats.m128_f32[2];

		SetLocal(Nodes, node, &Local);
	}


	/************************************************************************************************/


	void TranslateLocal(SceneNodes* Nodes, NodeHandle node, float3 XYZ)
	{
		LT_Entry Local(GetLocal(Nodes, node));

		Local.T.m128_f32[0] += XYZ.pfloats.m128_f32[0];
		Local.T.m128_f32[1] += XYZ.pfloats.m128_f32[1];
		Local.T.m128_f32[2] += XYZ.pfloats.m128_f32[2];

		SetLocal(Nodes, node, &Local);
	}


	/************************************************************************************************/


	void TranslateWorld(SceneNodes* Nodes, NodeHandle Node, float3 XYZ)
	{
		WT_Entry WT;
		GetWT(Nodes, GetParentNode(Nodes, Node), &WT);

		auto MI = DirectX::XMMatrixInverse(nullptr, WT.m4x4);
		auto V = DirectX::XMVector4Transform(XYZ.pfloats, MI);
		TranslateLocal(Nodes, Node, float3(V));
	}


	/************************************************************************************************/


	void SceneNodeCtr::Rotate(Quaternion Q)
	{
		LT_Entry Local(GetLocal(SceneNodes, Node));
		DirectX::XMVECTOR R = DirectX::XMQuaternionMultiply(Local.R, Q.floats);
		Local.R = R;

		SetLocal(SceneNodes, Node, &Local);
	}


	/************************************************************************************************/


	void Yaw(FlexKit::SceneNodes* Nodes, NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		rot.m128_f32[0] = 0;
		rot.m128_f32[1] = std::sin(r / 2);
		rot.m128_f32[2] = 0;
		rot.m128_f32[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Nodes, Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);

		FlexKit::SetLocal(Nodes, Node, &Local);
	}


	/************************************************************************************************/


	void Pitch(FlexKit::SceneNodes* Nodes, NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		rot.m128_f32[0] = std::sin(r / 2);;
		rot.m128_f32[1] = 0;
		rot.m128_f32[2] = 0;
		rot.m128_f32[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Nodes, Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);

		FlexKit::SetLocal(Nodes, Node, &Local);
	}


	/************************************************************************************************/


	void Roll(FlexKit::SceneNodes* Nodes, NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		rot.m128_f32[0] = 0;
		rot.m128_f32[1] = 0;
		rot.m128_f32[2] = std::sin(r / 2);
		rot.m128_f32[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Nodes, Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);

		FlexKit::SetLocal(Nodes, Node, &Local);
	}


	/************************************************************************************************/
}