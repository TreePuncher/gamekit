/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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

#ifndef TRANSFORMS_H
#define TRANSFORMS_H

#include "MathUtils.h"
#include "Handle.h"
#include "Components.h"
#include "RuntimeComponentIDs.h"
#include "ResourceHandles.h"
#include "XMMathConversion.h"
#include <DirectXMath/DirectXMath.h>

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
        DirectX::XMVECTOR T;
        DirectX::XMVECTOR R;
        DirectX::XMVECTOR S;
        DirectX::XMVECTOR Padding;

        static LT_Entry Zero()
        {
            LT_Entry zero;
            zero.T = DirectX::XMVectorZero();
            zero.R = DirectX::XMQuaternionIdentity();
            zero.S = DirectX::XMVectorSet(1, 1, 1, 0);

            return zero;
        }
    };


    __declspec(align(16))  struct WT_Entry
    {
        //LT_Entry			World;
        DirectX::XMMATRIX	m4x4;// Cached

        void SetToIdentity()	
        {	
            m4x4  = DirectX::XMMatrixIdentity(); 
            //World = LT_Entry::Zero();
        }
    };
    

    struct SceneNodes
    {
        enum StateFlags : char
        {
            CLEAR   = 0x00,
            DIRTY   = 0x01,
            FREE    = 0x02,
            SCALE   = 0x04,
            UPDATED = 0x08
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
            const auto idx2 = WT.emplace_back(WT_Entry{DirectX::XMMatrixIdentity()});
            const auto idx3 = Flags.emplace_back();
            const auto idx4 = Children.emplace_back();

            FK_ASSERT((idx0 == idx1) && (idx2 == idx3) && (idx1 == idx2) && (idx3 == idx4));

            return idx0;
        }

        size_t size() const { return Nodes.size(); }
    }inline SceneNodeTable;


    /************************************************************************************************/
    // TODO: add no except where applicable

    FLEXKITAPI uint16_t	_SNHandleToIndex	(NodeHandle Node);
    FLEXKITAPI void		_SNSetHandleIndex	(NodeHandle Node, uint16_t index);

    FLEXKITAPI void			InitiateSceneNodeBuffer		( iAllocator* persistent );
    FLEXKITAPI void			SortNodes					( StackAllocator* Temp );
    FLEXKITAPI void			ReleaseNode					( NodeHandle Node );

    FLEXKITAPI float3		LocalToGlobal				( NodeHandle Node, float3 POS);

    FLEXKITAPI LT_Entry		GetLocal					( NodeHandle Node );
    FLEXKITAPI float3		GetLocalScale				( NodeHandle Node );
    FLEXKITAPI void			GetTransform				( NodeHandle Node,	DirectX::XMMATRIX* __restrict out );
    FLEXKITAPI float4x4		GetWT						( NodeHandle Node );
    FLEXKITAPI void			GetTransform				( NodeHandle node,	float4x4* __restrict out );
    FLEXKITAPI Quaternion	GetOrientation				( NodeHandle Node );
    FLEXKITAPI float3		GetPositionW				( NodeHandle Node );
    FLEXKITAPI float3		GetPositionL				( NodeHandle Node );
    FLEXKITAPI NodeHandle	GetNewNode					();
    FLEXKITAPI NodeHandle	GetZeroedNode				();
    FLEXKITAPI bool			GetFlag						( NodeHandle Node,	size_t f );
    FLEXKITAPI uint32_t     GetFlags    				( NodeHandle Node);
    FLEXKITAPI NodeHandle	GetParentNode				( NodeHandle Node );

    FLEXKITAPI void			SetFlag						( NodeHandle Node,	uint32_t f );
    FLEXKITAPI void			SetLocal					( NodeHandle Node,	LT_Entry* __restrict In, uint32_t extraFlags = 0);
    FLEXKITAPI void			SetOrientation				( NodeHandle Node,	const Quaternion& In );	// Sets World Orientation
    FLEXKITAPI void			SetOrientationL				( NodeHandle Node,	const Quaternion& In );	// Sets World Orientation
    FLEXKITAPI void			SetParentNode				( NodeHandle Parent, NodeHandle Node );
    FLEXKITAPI void			SetPositionW				( NodeHandle Node,	float3 in );
    FLEXKITAPI void			SetPositionL				( NodeHandle Node,	float3 in );
    FLEXKITAPI void			SetWT						( NodeHandle Node,	DirectX::XMMATRIX* __restrict in  ); // Set World Transform
    FLEXKITAPI void			SetWT						( NodeHandle Node,	const float4x4  in); // Set World Transform
    FLEXKITAPI void			SetScale					( NodeHandle Node,	float3 In );

    FLEXKITAPI void			Scale						( NodeHandle Node,	float3 In );
    FLEXKITAPI void			TranslateLocal				( NodeHandle Node,	float3 In );
    FLEXKITAPI void			TranslateWorld				( NodeHandle Node,	float3 In );
    FLEXKITAPI NodeHandle	ZeroNode					( NodeHandle Node );


    /************************************************************************************************/


    FLEXKITAPI bool		UpdateTransforms();


    /************************************************************************************************/


    FLEXKITAPI void Yaw		(NodeHandle Node,	float r );
    FLEXKITAPI void Roll	(NodeHandle Node,	float r );
    FLEXKITAPI void Pitch	(NodeHandle Node,	float r );


    FLEXKITAPI  void UpdateNode(NodeHandle Node);

    /************************************************************************************************/



    class SceneNodeComponent : 
        public Component<SceneNodeComponent, TransformComponentID>
    {
    public:

        ~SceneNodeComponent()
        {
            SceneNodeTable.Release();
        }

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


    FLEXKITAPI inline auto& QueueTransformUpdateTask(UpdateDispatcher& Dispatcher)
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


    struct  NullSceneNodeBehaviorOverrides
    {
        template<typename ... discard>
        void SetDirty(discard ...) {}

        using Parent_TY = void;
    };


    /************************************************************************************************/


    template<typename Overrides_TY = NullSceneNodeBehaviorOverrides>
    class SceneNodeView : 
        public ComponentView_t<SceneNodeComponent>,
        public Overrides_TY
    {
    public:

        SceneNodeView(GameObject& gameObject, const float3 XYZ) :
            node{ GetComponent().CreateNode() }
        {
            SetPosition(XYZ);
        }


        SceneNodeView(GameObject& gameObject, NodeHandle IN_Node = GetComponent().CreateZeroedNode()) :
            node{ IN_Node }
        {
        }


        ~SceneNodeView() override
        {
            if (node != InvalidHandle)
                ReleaseNode(node);
        }


        SceneNodeView(SceneNodeView&& rhs)
        {
            node		= rhs.node;
            rhs.node	= InvalidHandle;
        }


        SceneNodeView& operator = (SceneNodeView&& rhs)
        {
            node		= rhs.node;
            rhs.node	= InvalidHandle;

            return *this;
        }


        SceneNodeView               (SceneNodeView&) = delete;
        SceneNodeView& operator =	(SceneNodeView&) = delete;


        operator NodeHandle () { return node; }


        NodeHandle GetParentNode() const
        {
            return FlexKit::GetParentNode(node);
        }


        void SetParentNode(NodeHandle Parent) noexcept
        {
            FlexKit::SetParentNode(Parent, node);
            Overrides_TY::SetDirty(static_cast<typename Overrides_TY::Parent_TY*>(this));
        }


        void Yaw(float r) noexcept
        {
            FlexKit::Yaw(node, r);
            Overrides_TY::SetDirty(static_cast<typename Overrides_TY::Parent_TY*>(this));
        }


        void Roll(float r) noexcept
        {
            FlexKit::Roll(node, r);
            Overrides_TY::SetDirty(static_cast<typename Overrides_TY::Parent_TY*>(this));
        }


        void Pitch(float r) noexcept
        {
            FlexKit::Pitch(node, r);
            Overrides_TY::SetDirty(static_cast<typename Overrides_TY::Parent_TY*>(this));
        }

        void Scale(float3 xyz) noexcept
        {
            FlexKit::Scale(node, xyz);
            Overrides_TY::SetDirty(static_cast<typename Overrides_TY::Parent_TY*>(this));
        }


        void TranslateLocal(float3 xyz) noexcept
        {
            FlexKit::TranslateLocal(node, xyz);
            Overrides_TY::SetDirty(static_cast<typename Overrides_TY::Parent_TY*>(this));
        }


        void TranslateWorld(float3 xyz) noexcept
        {
            FlexKit::TranslateWorld(node, xyz);
            Overrides_TY::SetDirty(static_cast<typename Overrides_TY::Parent_TY*>(this));
        }


        void ToggleScaling(bool scalable) noexcept
        {
            SetFlag(node, SceneNodes::SCALE);
        }


        float3	GetPosition() const noexcept
        {
            return FlexKit::GetPositionW(node);
        }

        float3	GetPositionL() const noexcept
        {
            return FlexKit::GetPositionL(node);
        }

        float3	GetScale() const noexcept
        {
            return GetLocalScale(node);
        }

        Quaternion GetOrientation() const noexcept
        {
            return FlexKit::GetOrientation(node);
        }

        float4x4 GetWT() const noexcept
        {
            return FlexKit::GetWT(node);
        }

        void SetScale(float3 scale) noexcept
        {
            FlexKit::SetScale(node, scale);
        }

        void Parent(NodeHandle child) noexcept
        {
            FlexKit::SetParentNode(node, child);
        }

        void SetPosition(const float3 xyz) noexcept
        {
            FlexKit::SetPositionW(node, xyz);
        }

        void SetPositionL(const float3 xyz) noexcept
        {
            FlexKit::SetPositionL(node, xyz);
        }

        void SetOrientation(const Quaternion q) noexcept
        {
            FlexKit::SetOrientation(node, q);
        }

        void SetOrientationL(const Quaternion q) noexcept
        {
            FlexKit::SetOrientationL(node, q);
        }

        void SetWT(const float4x4& wt) noexcept
        {
            FlexKit::SetWT(node, wt);
        }


        NodeHandle node;
    };


    void    Translate(GameObject& go, const float3 xyz);
    float3  GetLocalPosition(GameObject& go);
    float3  GetWorldPosition(GameObject& go);



    void        ClearParent(GameObject& go);

    float3      GetScale(GameObject& go);
    float3      GetScale(NodeHandle node);

    NodeHandle  GetParentNode(GameObject& go);
    void        EnableScale(GameObject& go, bool scale);

    void Pitch(GameObject& go, float theta);
    void Yaw(GameObject& go, float theta);

    Quaternion  GetOrientation(GameObject& go);
    NodeHandle  GetSceneNode(GameObject& go);
    float4x4    GetWT(GameObject& go);

    void SetLocalPosition(GameObject& go, const float3 pos);
    void SetWorldPosition(GameObject& go, const float3 pos);
    void SetScale(GameObject& go, float3 scale);

    void SetWT(GameObject& go, const float4x4 newMatrix);
    void SetOrientation(GameObject& go, const Quaternion q);


	/************************************************************************************************/


	struct SceneNodeReq
	{
		using Type		= SceneNodeView<>&;
		using ValueType = SceneNodeView<>;

		static constexpr bool IsConst() { return false; }

		bool Available(const GameObject& gameObject)
		{
			return gameObject.hasView(SceneNodeView<>::GetComponentID());
		}

		decltype(auto) GetValue(GameObject& gameObject)
		{
			return GetView<SceneNodeView<>>(gameObject);
		}
	};


}	/************************************************************************************************/


#endif
