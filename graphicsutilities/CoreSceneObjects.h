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

#include "..\\buildsettings.h"
#include "..\\coreutilities\MathUtils.h"
#include "..\\coreutilities\ResourceHandles.h"

#include <DirectXMath.h>

using DirectX::XMMATRIX;

namespace FlexKit
{	/************************************************************************************************/


	class FLEXKITAPI Camera
	{
	public:
		struct __declspec(align(16)) CameraConstantBuffer
		{
			XMMATRIX	View;
			XMMATRIX	ViewI;
			XMMATRIX	Proj;
			XMMATRIX	PV;			//  Projection x View
			XMMATRIX	PVI;		// (Projection x View)^-1
			float4		WPOS;
			float		MinZ;
			float		MaxZ;

			float Padding[2];

			float3  WSTopLeft;
			float3  WSTopRight;
			float3  WSBottomLeft;
			float3  WSBottomRight;

			float3  WSTopLeft_Near;
			float3  WSTopRight_Near;
			float3  WSBottomLeft_Near;
			float3  WSBottomRight_Near;

			constexpr static const size_t GetBufferSize()
			{
				//return sizeof(BufferLayout);	
				return 4096;	
			}

		};


		FustrumPoints			GetFrustumPoints	(float3 XYZ, Quaternion Q);
		CameraConstantBuffer	GetConstants		();
		float4x4				GetPV				();
		NodeHandle				Node;

		float FOV;
		float AspectRatio;
		float Near;
		float Far;
		bool  invert;
		float fStop;	// Future
		float ISO;		// Future

		float4x4 View;
		float4x4 Proj;
		float4x4 WT;	// World Transform
		float4x4 PV;	// Projection x View
		float4x4 IV;	// Inverse Transform


		void UpdateMatrices();

		static MinMax	GetAABS_XZ(FustrumPoints Points);
	};


	/************************************************************************************************/

	struct DrawableAnimationState;
	struct DrawablePoseState;
	struct TextureSet;

	struct FLEXKITAPI Drawable
	{
		NodeHandle			Node				= InvalidHandle_t;	// 2
		TriMeshHandle		Occluder			= InvalidHandle_t;	// 2
		TriMeshHandle		MeshHandle			= InvalidHandle_t;	// 2

		bool					DrawLast		= false; // 1
		bool					Transparent		= false; // 1
		bool					Textured		= false; // 1
		bool					Dirty			= false; // 1
		bool					Padding[5];		// 5
		char*					id;				// 8 - string ID, null terminated 

		struct MaterialProperties
		{
			MaterialProperties(float4 a, float4 m) : Albedo(a), Spec(m)
			{
				Albedo.w = min(a.w, 1.0f);
				Spec.w	 = min(m.w, 1.0f);
			}

			MaterialProperties() : Albedo(1.0, 1.0f, 1.0f, 0.1f), Spec(1, 1, 1, 1) {}

			float4		Albedo;		// Term 4 is Roughness
			float4		Spec;		// Metal Is first 4, Specular is rgb
		}MatProperties;	// 32 

		Drawable&	SetAlbedo(float4 RGBA)		{ MatProperties.Albedo	= RGBA; return *this; }
		Drawable&	SetSpecular(float4 RGBA)	{ MatProperties.Spec	= RGBA; return *this; }
		Drawable&	SetNode(NodeHandle H)		{ Node					= H;	return *this; }

		struct VConsantsLayout
		{
			MaterialProperties	MP;
			DirectX::XMMATRIX	Transform;
		};

		VConsantsLayout GetConstants();
	};

	constexpr const Type_t DRAWABLE_ID = GetTypeGUID(DRAWABLEHANDLE);


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
		PVEntry() {}
		PVEntry(Drawable& d) : OcclusionID(-1), D(&d){}
		PVEntry(Drawable& d, size_t ID, size_t sortID) : OcclusionID(ID), D(&d), SortID(sortID) {}

		size_t		SortID;
		size_t		OcclusionID;
		Drawable*	D;

		operator Drawable* ()	{ return D;			}
		operator size_t ()		{ return SortID;	}

		bool operator < (PVEntry rhs)
		{
			return SortID < rhs.SortID;
		}
	};

	
	typedef Vector<PVEntry> PVS;

	inline void PushPV(Drawable& e, PVS* pvs)
	{
		if (e.MeshHandle.to_uint() != INVALIDHANDLE)
			pvs->push_back(PVEntry( e, pvs->size(), 0u));
	}

	FLEXKITAPI void SortPVS				(PVS* PVS_, Camera* C);
	FLEXKITAPI void SortPVSTransparent	(PVS* PVS_, Camera* C);


	/************************************************************************************************/
}
#endif