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


#ifndef GEOMETRY_H_INCLUDED
#define GEOMETRY_H_INCLUDED

#include "BuildSettings.hpp"
#include "AnimationUtilities.hpp"
#include "Intersection.hpp"
#include "MathUtilities.hpp"
#include "MemoryUtilities.hpp"
#include "Serialization.hpp"
#include "RenderSystemInterface.hpp"


namespace FlexKit
{	/************************************************************************************************/


	class VertexBufferView
	{
	public:
		VertexBufferView();
		VertexBufferView(const VertexBufferView&) = delete;
		VertexBufferView(VertexBufferView&&) = delete;

		VertexBufferView(std::byte* _ptr, size_t size);
		VertexBufferView(std::byte* _ptr, size_t size, VERTEXBUFFER_FORMAT format, VERTEXBUFFER_TYPE type);

		~VertexBufferView();

		//VertexBufferView& operator += (const VertexBufferView& RHS);

		VertexBufferView& operator = (const VertexBufferView& RHS);	// Assignment Operator
		VertexBufferView& operator = (VertexBufferView&& RHS );	// Movement Operator


		template<typename Ty>
		class Typed_Iteration
		{
			typedef Typed_Iteration<Ty> This_Type;
		public:
			Typed_Iteration(std::byte* _ptr, size_t Size) : m_position((Ty*)(_ptr)), m_size(Size) {}

			class Typed_iterator
			{
			public:
				Typed_iterator(Ty* _ptr) : m_position(_ptr) {}
				Typed_iterator(const Typed_iterator& rhs) : m_position(rhs.m_position) {}

				inline void operator ++ ()		{ m_position++; }
				inline void operator ++ (int)	{ m_position++; }
				inline void operator -- ()		{ m_position--; }
				inline void operator -- (int)	{ m_position--; }

				inline Typed_iterator	operator =	(const Typed_iterator& rhs) {}

				inline bool operator <	(const Typed_iterator& rhs) { return (this->m_position < rhs.m_position); }


				inline bool				operator == (const Typed_iterator& rhs) { return (m_position == rhs.m_position); }
				inline bool				operator != (const Typed_iterator& rhs) { return (m_position != rhs.m_position); }

				inline Ty&				operator * () { return *m_position; }
				inline Ty				operator * ()	const { return *m_position; }

				inline const Ty&		peek_forward()	const { return *(m_position + sizeof(Ty)); }

			private:

				Ty*	m_position;
			};

			Ty&	operator [] (size_t index) { return m_position[index]; }
			Typed_iterator begin() { return Typed_iterator(m_position); }
			Typed_iterator end() { return Typed_iterator(m_position + m_size); }

			inline const size_t	size() { return m_size / sizeof(Ty); }

		private:
			Ty*			m_position;
			size_t		m_size;
		};

				/************************************************************************************************/


		template< typename TY >
		inline bool Push(const TY& in)
		{
			if (mBufferUsed + sizeof(TY) > mBufferSize)
				return false;

			auto size = sizeof(TY);
			if (sizeof(TY) != static_cast<uint32_t>(mBufferFormat))
				mBufferinError = true;
			else if (!mBufferinError)
			{
				std::byte* Val = (std::byte*)&in;
				for (size_t itr = 0; itr < static_cast<uint32_t>(mBufferFormat); itr++)
					mBuffer[mBufferUsed++] = Val[itr];
			}
			return !mBufferinError;
		}


		/************************************************************************************************/


		template<typename TY>
		inline bool Push(const TY& in, size_t bytesize)
		{
			if (mBufferUsed + bytesize > mBufferSize)
				return false;

			auto Val = (std::byte*)&in;
			for (size_t itr = 0; itr < bytesize; itr++)
				mBuffer[mBufferUsed++] = Val[itr];

			return !mBufferinError;
		}


		/************************************************************************************************/


		template<typename TY>
		bool Push(const TY& in) requires(std::is_same_v<TY, float3>)
		{
			if (mBufferUsed + static_cast<uint32_t>(mBufferFormat) > mBufferSize) {
				mBufferinError = true;
				return false;
			}

			const size_t elementSize = static_cast<uint32_t>(mBufferFormat);
			memcpy(mBuffer + mBufferUsed, &in, elementSize);
			mBufferUsed += elementSize;

			return !mBufferinError;
		}


		/************************************************************************************************/


		template<typename TY>
		inline bool Push(TY* in)
		{
			if (mBufferUsed + sizeof(TY) > mBufferSize)
				return false;

			if (!mBufferinError)
			{
				std::byte Val[128];
				memcpy(Val, &in, mBufferFormat);

				for (size_t itr = 0; itr < static_cast<uint32_t>(mBufferFormat); itr++)
					mBuffer[mBufferUsed++] = Val[itr];
			}
			return !mBufferinError;
		}


		/************************************************************************************************/


		template<typename Ty>
		inline Typed_Iteration<Ty> CreateTypedProxy()
		{
			return Typed_Iteration<Ty>(GetBuffer(), GetBufferSizeRaw() / sizeof(Ty));
		}


		template<template<typename TY_> class TY_V, typename Ty>
		inline bool Push(TY_V<Ty> static_vector)
		{
			if (mBufferUsed + sizeof(Ty) * static_vector.size() > mBufferSize)
				return false;

			for (auto in : static_vector)
				if (!Push(in)) return false;

			return true;
		}


		template<typename Ty, size_t count>
		inline bool Push(static_vector<Ty, count>& static_vector)
		{
			if (mBufferUsed + sizeof(Ty)*static_vector.size() > mBufferSize)
				return false;

			for (auto in : static_vector)
				if (!Push(in)) return false;

			return true;
		}


		void Begin(VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT);
		bool End();

		void MarkFull()
		{
			mBufferUsed = mBufferSize;
		}

		bool				LoadBuffer();
		bool				UnloadBuffer();

		std::byte*			GetBuffer()			const;
		size_t				GetElementSize()	const;
		size_t				GetBufferSize()		const;
		size_t				GetBufferSizeUsed()	const;
		size_t				GetBufferSizeRaw()	const;
		VERTEXBUFFER_FORMAT	GetBufferFormat()	const;
		VERTEXBUFFER_TYPE	GetBufferType()		const;


		void SetTypeFormatSize(VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT, size_t count);

		void Serialize(auto& ar)
		{
			void* temp = (void*)mBuffer;
			ar& RawBuffer{ temp, mBufferSize };
			ar& mBufferUsed;
			ar& mBufferElementSize;
			ar& mBufferFormat;
			ar& mBufferType;
			ar& mBufferinError;
			ar& mBufferLock;

			mBuffer = reinterpret_cast<std::byte*>(temp);
		}

	private:
		bool _combine(const VertexBufferView& LHS, const VertexBufferView& RHS, char* out);

		void				SetElementSize(size_t) {}
		std::byte*			mBuffer;
		size_t				mBufferSize;
		size_t				mBufferUsed;
		size_t				mBufferElementSize;
		VERTEXBUFFER_FORMAT	mBufferFormat;
		VERTEXBUFFER_TYPE	mBufferType;
		bool				mBufferinError;
		bool				mBufferLock;
	};


	/************************************************************************************************/


	FLEXKITAPI VertexBufferView* CreateVertexBufferView(iAllocator* Memory, size_t BufferLength);


	/************************************************************************************************/


	inline void CreateBufferView(size_t size, iAllocator* memory, VertexBufferView*& View, VERTEXBUFFER_TYPE T, VERTEXBUFFER_FORMAT F)
	{
		size_t VertexBufferSize = size * (size_t)F + sizeof(VertexBufferView);
		View = FlexKit::CreateVertexBufferView(memory, VertexBufferSize);
		View->Begin(T, F);
	}


	/************************************************************************************************/


	inline void CreateBufferView(std::byte* buffer, size_t bufferSize, VertexBufferView*& View, VERTEXBUFFER_TYPE T, VERTEXBUFFER_FORMAT F, iAllocator* allocator)
	{
		size_t blobSize = bufferSize + sizeof(VertexBufferView);
		std::byte* blob = (std::byte*)allocator->malloc(blobSize);

		View = new(blob) VertexBufferView(blob + sizeof(VertexBufferView), bufferSize, F, T);

		memcpy(blob + sizeof(VertexBufferView), buffer, bufferSize);
	}


	/************************************************************************************************/


	template<typename Ty_Container, typename FetchFN, typename TranslateFN>
	bool FillBufferView(const Ty_Container* Container, const size_t vertexCount, VertexBufferView* out, TranslateFN Translate, FetchFN Fetch)
	{
		for (uint32_t itr = 0; itr < vertexCount; ++itr) {
			auto temp = Translate(Fetch(itr, Container));
			out->Push(temp);
		}

		return out->End();
	}


	/************************************************************************************************/


	template<typename Ty_Container, typename FetchFN, typename TranslateFN, typename ProcessFN>
	void ScanBufferView(const Ty_Container* Container, const uint32_t vertexCount, TranslateFN Translate, FetchFN Fetch, ProcessFN Scan)
	{
		for (uint32_t itr = 0; itr < vertexCount; ++itr) {
			auto Vert = Translate(Fetch(itr, Container));
			Scan(Vert);
		}
	}


	/************************************************************************************************/


	template<typename Ty_Container, typename ProcessFN>
	void TransformBuffer(Ty_Container& Container, ProcessFN Transform)
	{
		const auto end = Container.end();
		for (auto e = Container.begin(); e != end; e++) {
			*e = Transform(*e);
		}
	}


	/************************************************************************************************/


	struct SkinDeformer
	{
		const char*	BoneNames;
		size_t		BoneCount;
	};

	union BoundingVolume
	{
		BoundingVolume() {}

		AABB			OBB;
		BoundingSphere	BS;
	};


	struct Vertex
	{
		void Clear() { xyz = { 0.0f, 0.0f, 0.0f }; }

		void AddWithWeight(Vertex const &src, float weight)
		{
			xyz.x += weight * src.xyz.x;
			xyz.y += weight * src.xyz.y;
			xyz.z += weight * src.xyz.z;
		}

		float3 xyz;
	};


	/************************************************************************************************/

	
	enum EAnimationData
	{
		EAD_None	= 0,
		EAD_Skin	= 1,
		EAD_Vertex	= 2
	};

	enum EOcclusion_Volume
	{
		EOV_ORIENTEDBOUNDINGBOX,
		EOV_ORIENTEDSPHERE,
	};


	/************************************************************************************************/


	struct TriMeshResource
	{
		size_t AnimationData;
		size_t IndexCount;
		size_t TriMeshID;
		size_t IndexBuffer_Idx;

		struct SubDivInfo
		{
			size_t  numVertices;
			size_t  numFaces;
			int*	numVertsPerFace;
			int*	IndicesPerFace;
		}*SubDiv;

		const char*			ID;
		SkinDeformer*		skinTable;
		Skeleton*			skeleton;

		struct RInfo
		{
			float3 Offset;
			float3 Min, Max;
			float  r;
		}Info;

		// Visibility Information
		AABB			aabb;
		BoundingSphere	bs;

		size_t	skeletonGUID;

		VertexBufferView*	buffers[16];
	};


	bool GenerateTangents(static_vector<VertexBufferView*>& buffers, iAllocator*);
	void UploadVertexBuffer(CopyContextHandle, const VertexBuffer&, const struct VertexBufferView&);


}	/************************************************************************************************/

#endif
