/**********************************************************************

Copyright (c) 2015 - 2018 Robert May

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

	}SceneNodeTable;


	/************************************************************************************************/
	

	FLEXKITAPI size_t	_SNHandleToIndex	(NodeHandle Node);
	FLEXKITAPI void		_SNSetHandleIndex	(NodeHandle Node, size_t index);

	FLEXKITAPI void			InitiateSceneNodeBuffer		( byte* pmem, size_t );
	FLEXKITAPI void			SortNodes					( StackAllocator* Temp );
	FLEXKITAPI void			ReleaseNode					( NodeHandle Node );

	FLEXKITAPI float3		LocalToGlobal				( NodeHandle Node, float3 POS);

	FLEXKITAPI LT_Entry		GetLocal					( NodeHandle Node );
	FLEXKITAPI float3		GetLocalScale				( NodeHandle Node );
	FLEXKITAPI void			GetTransform				( NodeHandle Node,	DirectX::XMMATRIX* __restrict out );
	FLEXKITAPI void			GetTransform				( NodeHandle node,	float4x4* __restrict out );
	FLEXKITAPI Quaternion	GetOrientation				( NodeHandle Node );
	FLEXKITAPI float3		GetPositionW				( NodeHandle Node );
	FLEXKITAPI float3		GetPositionL				( NodeHandle Node );
	FLEXKITAPI NodeHandle	GetNewNode					();
	FLEXKITAPI NodeHandle	GetZeroedNode				();
	FLEXKITAPI bool			GetFlag						( NodeHandle Node,	size_t f );
	FLEXKITAPI NodeHandle	GetParentNode				( NodeHandle Node );

	FLEXKITAPI void			SetFlag						( NodeHandle Node,	SceneNodes::StateFlags f );
	FLEXKITAPI void			SetLocal					( NodeHandle Node,	LT_Entry* __restrict In );
	FLEXKITAPI void			SetOrientation				( NodeHandle Node,	Quaternion& In );	// Sets World Orientation
	FLEXKITAPI void			SetOrientationL				( NodeHandle Node,	Quaternion& In );	// Sets World Orientation
	FLEXKITAPI void			SetParentNode				( NodeHandle Parent, NodeHandle Node );
	FLEXKITAPI void			SetPositionW				( NodeHandle Node,	float3 in );
	FLEXKITAPI void			SetPositionL				( NodeHandle Node,	float3 in );
	FLEXKITAPI void			SetWT						( NodeHandle Node,	DirectX::XMMATRIX* __restrict in  );				// Set World Transfo
	FLEXKITAPI void			SetScale					( NodeHandle Node,	float3 In );

	FLEXKITAPI void			Scale						( NodeHandle Node,	float3 In );
	FLEXKITAPI void			TranslateLocal				( NodeHandle Node,	float3 In );
	FLEXKITAPI void			TranslateWorld				( NodeHandle Node,	float3 In );
	FLEXKITAPI bool			UpdateTransforms			();
	FLEXKITAPI NodeHandle	ZeroNode					( NodeHandle Node );

	FLEXKITAPI inline void Yaw							( NodeHandle Node,	float r );
	FLEXKITAPI inline void Roll							( NodeHandle Node,	float r );
	FLEXKITAPI inline void Pitch						( NodeHandle Node,	float r );


	/************************************************************************************************/
}


#endif
