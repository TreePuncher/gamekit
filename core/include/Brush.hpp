#pragma once

#include "BuildSettings.hpp"
#include "MathUtilities.hpp"
#include "ResourceHandles.hpp"


namespace FlexKit
{	/************************************************************************************************/


	struct BrushAnimationState;
	struct PoseState;

	struct Brush
	{
		NodeHandle							Node		= InvalidHandle; // 4
		MaterialHandle						material	= InvalidHandle; // 4
		uint32_t							brushID		= 0xffffffff; // 4
		Vector<TriMeshHandle, 16, uint8_t>	meshes;

		bool					DrawLast		= false; // 1
		bool					Transparent		= false; // 1
		bool					Textured		= false; // 1
		bool					Dirty			= false; // 1
		bool					Skinned			= false;
		bool					Padding[1];		// 5
		char*					id;				// 8 - string ID, null terminated 

		struct MaterialProperties
		{
			float3		albedo		= float3{ 1.0f, 1.0f, 1.0f };
			float		kS			= 0.5f;
			float		IOR			= 0.45f;
			float		roughness	= 0.5f;
			float		anisotropic	= 0.0f;
			float		metallic	= 0.0f;
			uint32_t	textureCount = 0;
			uint32_t	textureChannels = 0;
		};	// 32 

		struct alignas(256) VConstantsLayout
		{
			float4x4_GPU		Transform;
			MaterialProperties	MP;
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

	struct BrushEntry
	{
		uint64_t		SortID			= 0u;
		const Brush*	brush			= nullptr;
		GameObject*		gameObject		= nullptr;
		uint32_t		submissionID	= (uint32_t)-1;

		static_vector<uint8_t>	LODlevel{ 0 };

		const Brush* operator -> ()			{ return brush; }
		const Brush* operator -> () const	{ return brush; }

		operator const Brush* ()	{ return brush; }
		operator size_t ()			{ return SortID; }

		bool operator < (BrushEntry rhs)
		{
			return SortID < rhs.SortID;
		}
	};

	
	typedef Vector<BrushEntry> BrushDrawList;

	size_t CreateSortingID(bool Posed, bool Textured, size_t Depth);

	void SortDrawList				(std::span<BrushEntry>, struct Camera* C);
	void SortDrawListTransparent	(std::span<BrushEntry>, struct Camera* C);


	/************************************************************************************************/
}


/**********************************************************************

Copyright (c) 2015 - 2025 Robert May

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
