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

#ifndef TRANSFORMS_H
#define TRANSFORMS_H

#include "MathUtils.h"
#include "Handle.h"

#include <DirectXMath.h>

namespace FlexKit
{
	const	size_t							NodeHandleSize = 16;
	typedef Handle_t<NodeHandleSize>		NodeHandle;
	typedef Handle_t<16>					TextureSetHandle;
	typedef static_vector<NodeHandle, 32>	ChildrenVector;


	struct Node
	{
		NodeHandle	TH;
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


	inline float4x4 XMMatrixToFloat4x4(DirectX::XMMATRIX* M)
	{
		float4x4 Mout;
		Mout = *(float4x4*)M;
		return Mout;
	}


	inline DirectX::XMMATRIX Float4x4ToXMMATIRX(float4x4* M)
	{
		DirectX::XMMATRIX Mout;
		Mout = *(DirectX::XMMATRIX*)M;
		return Mout;
	}

	

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

		operator SceneNodes* () { return this; }

		size_t used;
		size_t max;

		Node*			Nodes;
		LT_Entry*		LT;
		WT_Entry*		WT;
		char*			Flags;

		uint16_t*		Indexes;
		ChildrenVector* Children;


		#pragma pack(push, 1)
		struct BOILERPLATE
		{
			Node		N;
			LT_Entry	LT;
			WT_Entry	WT;
			uint16_t	I;
			char		State;
		};
		#pragma pack(pop)
	};


	/************************************************************************************************/
	

	inline size_t	_SNHandleToIndex	( SceneNodes* Nodes, NodeHandle Node)				{ return Nodes->Indexes[Node.INDEX];	}
	inline void		_SNSetHandleIndex	( SceneNodes* Nodes, NodeHandle Node, size_t index)	{ Nodes->Indexes[Node.INDEX] = index;	}

	FLEXKITAPI void			InitiateSceneNodeBuffer		( SceneNodes* Nodes, byte* pmem, size_t );
	FLEXKITAPI void			SortNodes					( SceneNodes* Nodes, StackAllocator* Temp );
	FLEXKITAPI void			ReleaseNode					( SceneNodes* Nodes, NodeHandle Node );

	FLEXKITAPI float3		LocalToGlobal				( SceneNodes* Nodes, NodeHandle Node, float3 POS);


	FLEXKITAPI LT_Entry		GetLocal					( SceneNodes* Nodes, NodeHandle Node );
	FLEXKITAPI float3		GetLocalScale				( SceneNodes* Nodes, NodeHandle Node );
	FLEXKITAPI void			GetWT						( SceneNodes* Nodes, NodeHandle Node,	DirectX::XMMATRIX* __restrict out );
	FLEXKITAPI void			GetWT						( SceneNodes* Nodes, NodeHandle Node,	WT_Entry* __restrict out );		
	FLEXKITAPI void			GetWT						( SceneNodes* Nodes, NodeHandle node,	float4x4* __restrict out );
	FLEXKITAPI Quaternion	GetOrientation				( SceneNodes* Nodes, NodeHandle Node );
	FLEXKITAPI float3		GetPositionW				( SceneNodes* Nodes, NodeHandle Node );
	FLEXKITAPI float3		GetPositionL				( SceneNodes* Nodes, NodeHandle Node );
	FLEXKITAPI NodeHandle	GetNewNode					( SceneNodes* Nodes );
	FLEXKITAPI NodeHandle	GetZeroedNode				( SceneNodes* Nodes );
	FLEXKITAPI bool			GetFlag						( SceneNodes* Nodes, NodeHandle Node,	size_t f );
	FLEXKITAPI NodeHandle	GetParentNode				( SceneNodes* Nodes, NodeHandle Node );

	FLEXKITAPI void			SetFlag						( SceneNodes* Nodes, NodeHandle Node,	SceneNodes::StateFlags f );
	FLEXKITAPI void			SetLocal					( SceneNodes* Nodes, NodeHandle Node,	LT_Entry* __restrict In );
	FLEXKITAPI void			SetOrientation				( SceneNodes* Nodes, NodeHandle Node,	Quaternion& In );	// Sets World Orientation
	FLEXKITAPI void			SetOrientationL				( SceneNodes* Nodes, NodeHandle Node,	Quaternion& In );	// Sets World Orientation
	FLEXKITAPI void			SetParentNode				( SceneNodes* Nodes, NodeHandle Parent, NodeHandle Node );
	FLEXKITAPI void			SetPositionW				( SceneNodes* Nodes, NodeHandle Node,	float3 in );
	FLEXKITAPI void			SetPositionL				( SceneNodes* Nodes, NodeHandle Node,	float3 in );
	FLEXKITAPI void			SetWT						( SceneNodes* Nodes, NodeHandle Node,	DirectX::XMMATRIX* __restrict in  );				// Set World Transform
	FLEXKITAPI void			SetScale					( SceneNodes* Nodes, NodeHandle Node,	float3 In );

	FLEXKITAPI void			Scale						( SceneNodes* Nodes, NodeHandle Node,	float3 In );
	FLEXKITAPI void			TranslateLocal				( SceneNodes* Nodes, NodeHandle Node,	float3 In );
	FLEXKITAPI void			TranslateWorld				( SceneNodes* Nodes, NodeHandle Node,	float3 In );
	FLEXKITAPI bool			UpdateTransforms			( SceneNodes* Nodes );
	FLEXKITAPI NodeHandle	ZeroNode					( SceneNodes* Nodes, NodeHandle Node );

	FLEXKITAPI inline void Yaw							( SceneNodes* Nodes, NodeHandle Node,	float r );
	FLEXKITAPI inline void Roll							( SceneNodes* Nodes, NodeHandle Node,	float r );
	FLEXKITAPI inline void Pitch						( SceneNodes* Nodes, NodeHandle Node,	float r );


	/************************************************************************************************/


	struct FLEXKITAPI SceneNodeCtr
	{
		inline void WTranslate	( float3 XYZ ) { TranslateWorld(SceneNodes, Node, XYZ); }
		inline void LTranslate	( float3 XYZ ) { TranslateLocal(SceneNodes, Node, XYZ); }
		void		Rotate		( Quaternion Q );

		NodeHandle	Node;
		SceneNodes*	SceneNodes;
	};


	/************************************************************************************************/
}


#endif
