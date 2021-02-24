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


	inline uint16_t	_SNHandleToIndex(NodeHandle Node) 
	{ 
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
		size_t FreeIndex = -1;
		for (size_t I = 0; I >= 0; ++I)
		{
			if (SceneNodeTable.Flags[I] & SceneNodes::FREE) {
				FreeIndex = I;
				break;
			}
		}
		return FreeIndex;
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
        uint16_t nodeIdx = SceneNodeTable._AddNode();

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
		SceneNodeTable.Flags[_SNHandleToIndex(handle)] = SceneNodes::FREE;
		SceneNodeTable.Nodes[_SNHandleToIndex(handle)].Parent = NodeHandle(-1);
		_SNSetHandleIndex(handle, -1);
	}

	
	/************************************************************************************************/


	NodeHandle GetParentNode(NodeHandle handle)
	{
		if (handle == FlexKit::InvalidHandle_t)
			return FlexKit::InvalidHandle_t;

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

		return float3( wt.r[0].m128_f32[3], wt.r[1].m128_f32[3], wt.r[2].m128_f32[3] );
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
#if 0
		using DirectX::XMQuaternionConjugate;
		using DirectX::XMQuaternionMultiply;
		using DirectX::XMVectorSubtract;

		// Set New Local Position
		LT_Entry Local;
		WT_Entry Parent;

		// Gather
		GetLocal(Nodes, node, &Local);
		GetTransform(Nodes, GetParentNode(Nodes, node), &Parent);

		// Calculate
		Local.T = XMVectorSubtract( Parent.World.T, in.pfloats );
		SetLocal(Nodes, node, &Local);
#else
		DirectX::XMMATRIX wt;
		GetTransform(node, &wt);
		auto tmp =  DirectX::XMMatrixInverse(nullptr, wt) * DirectX::XMMatrixTranslation(in[0], in[1], in[2] );
		float3 lPosition = float3( tmp.r[3].m128_f32[0], tmp.r[3].m128_f32[1], tmp.r[3].m128_f32[2] );

		wt.r[0].m128_f32[3] = in.x;
		wt.r[1].m128_f32[3] = in.y;
		wt.r[2].m128_f32[3] = in.z;

		SetWT(node, &wt);
		// Set New Local Position
		LT_Entry Local(GetLocal(node));

		Local.T.m128_f32[0] = lPosition[0];
		Local.T.m128_f32[1] = lPosition[1];
		Local.T.m128_f32[2] = lPosition[2];

		SetLocal	(node, &Local);
		SetFlag		(node, SceneNodes::DIRTY);
#endif
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

		Local.S.m128_f32[0] = XYZ.pfloats.m128_f32[0];
		Local.S.m128_f32[1] = XYZ.pfloats.m128_f32[1];
		Local.S.m128_f32[2] = XYZ.pfloats.m128_f32[2];

		SetLocal(Node, &Local);
	}


	/************************************************************************************************/


	void SetWT(NodeHandle node, DirectX::XMMATRIX* __restrict In)
	{
		SceneNodeTable.WT[_SNHandleToIndex(node)].m4x4 = *In;
		SetFlag(node, SceneNodes::DIRTY);
	}


	/************************************************************************************************/


	void SetLocal(NodeHandle node, LT_Entry* __restrict In)
	{
		SceneNodeTable.LT[_SNHandleToIndex(node)] = *In;
		SetFlag(node, SceneNodes::StateFlags::DIRTY);
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


	void SetFlag(NodeHandle node, SceneNodes::StateFlags f)
	{
		auto index = _SNHandleToIndex(node);
		SceneNodeTable.Flags[index] = SceneNodeTable.Flags[index] | f;
	}


	/************************************************************************************************/


	void Scale(NodeHandle node, float3 XYZ)
	{
		LT_Entry Local(GetLocal(node));

		Local.S.m128_f32[0] *= XYZ.pfloats.m128_f32[0];
		Local.S.m128_f32[1] *= XYZ.pfloats.m128_f32[1];
		Local.S.m128_f32[2] *= XYZ.pfloats.m128_f32[2];

		SetLocal(node, &Local);
	}


	/************************************************************************************************/


	void TranslateLocal(NodeHandle node, float3 XYZ)
	{
		LT_Entry Local(GetLocal(node));

		Local.T.m128_f32[0] += XYZ.pfloats.m128_f32[0];
		Local.T.m128_f32[1] += XYZ.pfloats.m128_f32[1];
		Local.T.m128_f32[2] += XYZ.pfloats.m128_f32[2];

		SetLocal(node, &Local);
	}


	/************************************************************************************************/


	void TranslateWorld(NodeHandle Node, float3 XYZ)
	{
		XMMATRIX WT;
		GetTransform(GetParentNode(Node), &WT);

		auto MI = DirectX::XMMatrixInverse(nullptr, WT);
		auto V	= DirectX::XMVector4Transform(XYZ.pfloats, MI);
		TranslateLocal(Node, float3(V));
	}


	/************************************************************************************************/


	void Yaw(NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		rot.m128_f32[0] = 0;
		rot.m128_f32[1] = std::sin(r / 2);
		rot.m128_f32[2] = 0;
		rot.m128_f32[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);
        Local.R = DirectX::XMQuaternionNormalize(Local.R);

		FlexKit::SetLocal(Node, &Local);
	}


	/************************************************************************************************/


	void Pitch(NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		rot.m128_f32[0] = std::sin(r / 2);;
		rot.m128_f32[1] = 0;
		rot.m128_f32[2] = 0;
		rot.m128_f32[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);

		FlexKit::SetLocal(Node, &Local);
	}


	/************************************************************************************************/


	void Roll(NodeHandle Node, float r)
	{
		DirectX::XMVECTOR rot;
		rot.m128_f32[0] = 0;
		rot.m128_f32[1] = 0;
		rot.m128_f32[2] = std::sin(r / 2);
		rot.m128_f32[3] = std::cos(r / 2);

		FlexKit::LT_Entry Local(FlexKit::GetLocal(Node));

		Local.R = DirectX::XMQuaternionMultiply(rot, Local.R);

		FlexKit::SetLocal(Node, &Local);
	}


	/************************************************************************************************/
}
