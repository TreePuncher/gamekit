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
		NodeHandle	handle			= InvalidHandle; //?
		NodeHandle	parent			= InvalidHandle;
		NodeHandle	childrenFirst	= InvalidHandle;
		NodeHandle	childrenLast	= InvalidHandle;
		NodeHandle	next			= InvalidHandle;
		bool		Scaleflag		= false;// Calculates Scale only when set to on, Off By default
	};

	
	struct alignas(32) LT_Entry
	{
		double4		T;
		Quaternion	R;
		float3		S;

		static LT_Entry Zero();
	};


	struct WT_Entry
	{
		double4x4 m4x4; // Cached

		void SetToIdentity();
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

		NodeHandle  root;

		HandleUtilities::HandleTable<NodeHandle> Indexes;

		~SceneNodes();
		void Release();

		size_t _AddNode();

		size_t size() const;
	}inline SceneNodeTable;


	/************************************************************************************************/
	// TODO: add no except where applicable

	uint16_t		_SNHandleToIndex			(NodeHandle Node);
	void			_SNSetHandleIndex			(NodeHandle Node, uint16_t index);

	void			InitiateSceneNodeBuffer		(iAllocator* persistent);
	void			SortNodes					(StackAllocator* Temp);
	void			ReleaseNode					(NodeHandle Node);

	double3			LocalToGlobal				(NodeHandle Node, float3 POS);

	LT_Entry		GetLocal					(NodeHandle Node);
	float3			GetLocalScale				(NodeHandle Node);
	double4x4		GetWT						(NodeHandle Node);
	double4x4		GetLT						(NodeHandle Node);
	Quaternion		GetOrientation				(NodeHandle Node);
	Quaternion		GetOrientationLocal			(NodeHandle Node);
	double3			GetPositionW				(NodeHandle Node);
	double3			GetPositionL				(NodeHandle Node);
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
	void			SetPositionW				(NodeHandle Node,	double3 in);
	void			SetPositionL				(NodeHandle Node,	double3 in);
	void			SetWT						(NodeHandle Node,	const double4x4& in); // Set World Transform
	void			SetScale					(NodeHandle Node,	float3 In);

	void			Scale						(NodeHandle Node,	float3 In);
	void			TranslateLocal				(NodeHandle Node,	double3 In);
	void			TranslateWorld				(NodeHandle Node,	double3 In);
	NodeHandle		ZeroNode					(NodeHandle Node );


	/************************************************************************************************/


	bool		UpdateTransforms();


	/************************************************************************************************/


	void Yaw	(NodeHandle Node,	float r );
	void Roll	(NodeHandle Node,	float r );
	void Pitch	(NodeHandle Node,	float r );


    void UpdateNode(NodeHandle Node);


	/************************************************************************************************/


	class SceneNodeComponent : 
		public Component<SceneNodeComponent, TransformComponentID>
	{
	public:
		~SceneNodeComponent();
		void FreeComponentView(void* _ptr) final;

		NodeHandle CreateZeroedNode();
	    NodeHandle CreateNode();
        NodeHandle GetRoot();
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

		void TranslateLocal(double3 xyz) noexcept;
		void TranslateWorld(double3 xyz) noexcept;

		void ToggleScaling(bool scalable) noexcept;

		double3		GetPosition() const noexcept;
		double3		GetPositionL() const noexcept;
		float3		GetScale() const noexcept;
		Quaternion	GetOrientation() const noexcept;
		Quaternion	GetOrientationL() const noexcept;
		double4x4	GetWT() const noexcept;

		void SetScale(float3 scale) noexcept;
		void SetPosition(const double3 xyz) noexcept;
		void SetPositionL(const double3 xyz) noexcept;
		void SetOrientation(const Quaternion q) noexcept;
		void SetOrientationL(const Quaternion q) noexcept;
		void SetWT(const double4x4& wt) noexcept;

		NodeHandle		node;
		TriggerHandle	triggers;
		bool			triggerEnable = false;
	};


	void		Translate(GameObject& go, const float3 xyz);
	double3		GetLocalPosition(GameObject& go);
	double3		GetWorldPosition(GameObject& go);


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
	double4x4	GetWT(GameObject& go);

	void		SetLocalPosition(GameObject& go, const float3 pos);
	void		SetWorldPosition(GameObject& go, const float3 pos);
	void		SetScale(GameObject& go, float3 scale);

	void		SetWT(GameObject& go, const double4x4 newMatrix);
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
