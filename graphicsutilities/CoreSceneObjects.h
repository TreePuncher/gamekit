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

#ifndef CORESCENEOBJECTS_H_INCLUDED
#define CORESCENEOBJECTS_H_INCLUDED

#include "buildsettings.h"
#include "MathUtils.h"
#include "ResourceHandles.h"
#include "Transforms.h"

#include <DirectXMath/DirectXMath.h>

using DirectX::XMMATRIX;

namespace FlexKit
{	/************************************************************************************************/


	class FLEXKITAPI Camera
	{
	public:
		struct alignas(256) ConstantBuffer
		{
			float4x4	View;
			float4x4	ViewI;
			float4x4	Proj;
			float4x4	PV;			//  Projection x View
			float4x4	PVI;		// (Projection x View)^-1
			float4		WPOS;
			float		MinZ;
			float		MaxZ;
			float       AspectRatio;
			float       FOV;

			float3   TLCorner_VS;
			float3   TRCorner_VS;

			float3   BLCorner_VS;
			float3   BRCorner_VS;

			constexpr static const size_t GetBufferSize()
			{
				return sizeof(ConstantBuffer);
			}

		};


		FustrumPoints			    GetFrustumPoints(float3 XYZ, Quaternion Q);

		ConstantBuffer			    GetConstants() const;
		ConstantBuffer			    GetCameraPreviousConstants() const;
		float4x4				    GetPV();

		NodeHandle	Node;

		float       FOV;
		float       AspectRatio;
		float       Near;
		float       Far;
		bool        invert;
		float       fStop;	// Future
		float       ISO;		// Future

		float4x4    View;
		float4x4    Proj;
		float4x4    WT;	// World Transform
		float4x4    PV;	// Projection x View
		float4x4    IV;	// Inverse Transform


		struct PreviousFrameState
		{
			float4x4 View   = float4x4::Identity();
			float4x4 Proj   = float4x4::Identity();
			float4x4 WT     = float4x4::Identity();	// World Transform
			float4x4 PV     = float4x4::Identity();	// Projection x View
			float4x4 IV     = float4x4::Identity();	// Inverse Transform

			float FOV           = 1.0f;
			float aspectRatio   = 1.0f;
			float nearClip      = 0.01f;
			float farClip       = 10000.0f;
		}   previous;

		void UpdateMatrices();

		static MinMax	GetAABS_XZ(FustrumPoints Points);
	};


	/************************************************************************************************/

	struct BrushAnimationState;
	struct PoseState;

	struct FLEXKITAPI Brush
	{
		NodeHandle						Node		= InvalidHandle; // 2
		MaterialHandle					material	= InvalidHandle; // 2
		TriMeshHandle					Occluder	= InvalidHandle; // 2
		static_vector<TriMeshHandle>	meshes;
		//TriMeshHandle		MeshHandle			= InvalidHandle;	// 2

		bool					DrawLast		= false; // 1
		bool					Transparent		= false; // 1
		bool					Textured		= false; // 1
		bool					Dirty			= false; // 1
		bool                    Skinned         = false;
		bool					Padding[1];		// 5
		char*					id;				// 8 - string ID, null terminated 

		struct MaterialProperties
		{
			float3	albedo		= float3{1.0f, 1.0f, 1.0f};
			float	kS			= 0.5f;
			float	IOR			= 0.45f;
			float	roughness	= 0.5f;
			float	anisotropic	= 0.0f;
			float	metallic	= 0.0f;
		};	// 32 

		struct alignas(256) VConstantsLayout
		{
			MaterialProperties	MP;
			float4x4			Transform;
			uint32_t			textureCount;
			uint32_t			padding[3];
			uint4				textureHandles[16];
		};

		VConstantsLayout GetConstants() const;
	};

	constexpr const Type_t BRUSH_ID = GetTypeGUID(Brush);


	/************************************************************************************************/


	struct SortingField
	{
		unsigned int Posed			: 1;
		unsigned int Textured		: 1;
		unsigned int MaterialID		: 7;
		unsigned int InvertDepth	: 1;
		unsigned long long Depth	: 53;

		operator uint64_t(){return *(uint64_t*)(this + 8);}

	};

	struct PVEntry
	{
		size_t			SortID			= 0;
		const Brush*	brush			= nullptr;
		GameObject*		gameObject		= nullptr;
		uint32_t		OcclusionID		= -1;

		static_vector<uint8_t>	LODlevel{ 0 };

		const Brush* operator -> ()			{ return brush; }
		const Brush* operator -> () const	{ return brush; }

		operator const Brush* ()	{ return brush; }
		operator size_t ()			{ return SortID; }

		bool operator < (PVEntry rhs)
		{
			return SortID < rhs.SortID;
		}
	};

	
	typedef Vector<PVEntry> PVS;

	size_t CreateSortingID(bool Posed, bool Textured, size_t Depth);

	FLEXKITAPI void SortPVS				(PVS* PVS_, Camera* C);
	FLEXKITAPI void SortPVSTransparent	(PVS* PVS_, Camera* C);


	/************************************************************************************************/
}
#endif
