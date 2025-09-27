#pragma once
#include "Components.hpp"
#include "Handle.hpp"
#include "MathUtilities.hpp"
#include "RuntimeComponentIDs.hpp"
#include "ResourceHandles.hpp"
#include "TriggerComponent.hpp"

namespace FlexKit
{
	typedef static_vector<NodeHandle, 32> ChildrenVector;

	struct Node
	{
		NodeHandle	handle; //?
		NodeHandle	Parent;
		NodeHandle	ChildrenList;
		bool		Scaleflag;// Calculates Scale only when set to on, Off By default
	};

	
	__declspec(align(16))  struct LT_Entry
	{
		float4		T;
		Quaternion	R;
		float4		S;
		float4		Padding;

		static LT_Entry Zero()
		{
			LT_Entry zero;
			zero.T = float4::Zero();
		    zero.R = Quaternion::Identity();
			zero.S = float3{ 1, 1, 1 };

			return zero;
		}
	};


	struct WT_Entry
	{
		//LT_Entry			World;
		float4x4	m4x4;// Cached

		void SetToIdentity()	
		{	
			m4x4  = float4x4::Identity(); 
			//World = LT_Entry::Zero();
		}
	};
	

	struct SceneNodes
	{
		enum StateFlags : char
		{
			CLEAR				= 0x00,
			DIRTY				= 0x01,
			FREE				= 0x02,
			SCALE				= 0x04,
			UPDATED				= 0x08,
			UPDATEDIMMEDIATE	= 0x10,
		};

		Vector<Node>            Nodes;
		Vector<LT_Entry>	    LT;
		Vector<WT_Entry>        WT;
		Vector<char>            Flags;
		Vector<ChildrenVector>  Children;

		NodeHandle  root;

		HandleUtilities::HandleTable<NodeHandle> Indexes;

		~SceneNodes()
		{
			Release();
		}

		void Release()
		{
			Nodes.Release();
			LT.Release();
			WT.Release();
			Flags.Release();
			Children.Release();
			Indexes.Release();
		}

		size_t _AddNode()
		{
			const auto idx0 = Nodes.emplace_back();
			const auto idx1 = LT.emplace_back();
			const auto idx2 = WT.emplace_back(WT_Entry{ float4x4::Identity() });
			const auto idx3 = Flags.emplace_back();
			const auto idx4 = Children.emplace_back();

			FK_ASSERT((idx0 == idx1) && (idx2 == idx3) && (idx1 == idx2) && (idx3 == idx4));

			return idx0;
		}

		size_t size() const { return Nodes.size(); }
	}inline SceneNodeTable;


	/************************************************************************************************/
	// TODO: add no except where applicable

	uint16_t		_SNHandleToIndex			(NodeHandle Node);
	void			_SNSetHandleIndex			(NodeHandle Node, uint16_t index);

	void			InitiateSceneNodeBuffer		(iAllocator* persistent);
	void			SortNodes					(StackAllocator* Temp);
	void			ReleaseNode					(NodeHandle Node);

	float3			LocalToGlobal				(NodeHandle Node, float3 POS);

	LT_Entry		GetLocal					(NodeHandle Node);
	float3			GetLocalScale				(NodeHandle Node);
	float4x4		GetWT						(NodeHandle Node);
	float4x4		GetLT						(NodeHandle Node);
	Quaternion		GetOrientation				(NodeHandle Node);
	Quaternion		GetOrientationLocal			(NodeHandle Node);
	float3			GetPositionW				(NodeHandle Node);
	float3			GetPositionL				(NodeHandle Node);
	NodeHandle		GetNewNode					();
	NodeHandle		GetZeroedNode				();
	bool			GetFlag						(NodeHandle Node, size_t f);
	uint32_t		GetFlags					(NodeHandle Node);
	NodeHandle		GetParentNode				(NodeHandle Node);

	void			SetFlag						(NodeHandle Node,	uint32_t f );
	void			SetLocal					(NodeHandle Node,	LT_Entry* __restrict In, uint32_t extraFlags = 0);
	void			SetOrientation				(NodeHandle Node,	const Quaternion& In);	// Sets World Orientation
	void			SetOrientationL				(NodeHandle Node,	const Quaternion& In);	// Sets World Orientation
	void			SetParentNode				(NodeHandle Parent, NodeHandle Node);
	void			SetPositionW				(NodeHandle Node,	float3 in);
	void			SetPositionL				(NodeHandle Node,	float3 in);
	void			SetWT						(NodeHandle Node,	const float4x4& in); // Set World Transform
	void			SetScale					(NodeHandle Node,	float3 In);

	void			Scale						(NodeHandle Node,	float3 In);
	void			TranslateLocal				(NodeHandle Node,	float3 In);
	void			TranslateWorld				(NodeHandle Node,	float3 In);
	NodeHandle		ZeroNode					(NodeHandle Node );


	/************************************************************************************************/


	bool		UpdateTransforms();


	/************************************************************************************************/


	void Yaw		(NodeHandle Node,	float r );
	void Roll	(NodeHandle Node,	float r );
	void Pitch	(NodeHandle Node,	float r );


    void UpdateNode(NodeHandle Node);

	/************************************************************************************************/



	class SceneNodeComponent : 
		public Component<SceneNodeComponent, TransformComponentID>
	{
	public:
		~SceneNodeComponent()
		{
			SceneNodeTable.Release();
		}
		void FreeComponentView(void* _ptr) final;

		NodeHandle CreateZeroedNode()
		{
			return FlexKit::GetZeroedNode();
		}

		NodeHandle CreateNode()
		{
			return FlexKit::GetNewNode();
		}

		NodeHandle GetRoot() 
		{
			return NodeHandle{ 0 };
		}
	};


	/************************************************************************************************/


	inline auto& QueueTransformUpdateTask(UpdateDispatcher& Dispatcher)
	{
		struct TransformUpdateData
		{};

		auto& TransformUpdate = Dispatcher.Add<TransformUpdateData>(
			TransformComponentID,
			[&](auto& Builder, TransformUpdateData& Data)
			{
				Builder.SetDebugString("UpdateTransform");
			},
			[](auto& Data, iAllocator& threadAllocator)
			{
				ProfileFunction();

				FK_LOG_9("Transform Update");
				UpdateTransforms();
			});

		return TransformUpdate;
	}


	/************************************************************************************************/


	class SceneNodeView : public ComponentView_t<SceneNodeComponent>
	{
	public:

		SceneNodeView(GameObject& gameObject, const float3 XYZ);
		SceneNodeView(GameObject& gameObject, NodeHandle IN_Node = GetComponent().CreateZeroedNode());
		SceneNodeView(SceneNodeView&& rhs);

		void Release();

		SceneNodeView& operator = (SceneNodeView&& rhs);

		SceneNodeView				(SceneNodeView&) = delete;
		SceneNodeView& operator =	(SceneNodeView&) = delete;


		operator NodeHandle () const noexcept;


		NodeHandle GetParentNode() const;

		void SetParentNode(NodeHandle parent) noexcept;

		void Yaw(float r) noexcept;
		void Roll(float r) noexcept;
		void Pitch(float r) noexcept;

		void Scale(float3 xyz) noexcept;

		void TranslateLocal(float3 xyz) noexcept;
		void TranslateWorld(float3 xyz) noexcept;

		void ToggleScaling(bool scalable) noexcept;

		float3		GetPosition() const noexcept;
		float3		GetPositionL() const noexcept;
		float3		GetScale() const noexcept;
		Quaternion	GetOrientation() const noexcept;
		Quaternion	GetOrientationL() const noexcept;
		float4x4	GetWT() const noexcept;

		void SetScale(float3 scale) noexcept;
		void SetPosition(const float3 xyz) noexcept;
		void SetPositionL(const float3 xyz) noexcept;
		void SetOrientation(const Quaternion q) noexcept;
		void SetOrientationL(const Quaternion q) noexcept;
		void SetWT(const float4x4& wt) noexcept;

		NodeHandle		node;
		TriggerHandle	triggers;
		bool			triggerEnable = false;
	};


	void		Translate(GameObject& go, const float3 xyz);
	float3		GetLocalPosition(GameObject& go);
	float3		GetWorldPosition(GameObject& go);


	void		ClearParent(GameObject& go);

	float3		GetScale(GameObject& go);
	float3		GetScale(NodeHandle node);

	NodeHandle	GetParentNode(GameObject& go);
	void		SetParentNode(GameObject& go, NodeHandle);
	void		EnableScale(GameObject& go, bool scale);

	void		Pitch(GameObject& go, float theta);
	void		Yaw(GameObject& go, float theta);

	Quaternion	GetOrientation(GameObject& go);
	Quaternion	GetOrientationLocal(GameObject& go);
	NodeHandle	GetSceneNode(GameObject& go);
	float4x4	GetWT(GameObject& go);

	void		SetLocalPosition(GameObject& go, const float3 pos);
	void		SetWorldPosition(GameObject& go, const float3 pos);
	void		SetScale(GameObject& go, float3 scale);

	void		SetWT(GameObject& go, const float4x4 newMatrix);
	void		SetOrientation(GameObject& go, const Quaternion q);
	void		SetOrientationLocal(GameObject& go, const Quaternion q);


	/************************************************************************************************/


	struct SceneNodeReq
	{
		using Type		= SceneNodeView&;
		using ValueType = SceneNodeView;

		static constexpr bool IsConst() { return false; }

		bool Available(const GameObject& gameObject)
		{
			return gameObject.hasView(SceneNodeView::GetComponentID());
		}

		decltype(auto) GetValue(GameObject& gameObject)
		{
			return GetView<SceneNodeView>(gameObject);
		}
	};


}	/************************************************************************************************/


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
