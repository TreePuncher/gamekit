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


#include "Geometry.h"

namespace FlexKit
{

	/************************************************************************************************/


	VertexBufferView::VertexBufferView()
	{

		mBufferinError     = true;
		mBufferFormat      = VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_UNKNOWN;
		mBufferType        = VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ERROR;
		mBufferElementSize = 0;
		mBuffer			   = nullptr;
		mBufferUsed		   = 0;
		mBufferSize        = 0;
	}


	/************************************************************************************************/


	VertexBufferView::VertexBufferView(FlexKit::byte* _ptr, size_t size )
	{
		mBufferinError     = true;
		mBufferFormat      = VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_UNKNOWN;
		mBufferType        = VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ERROR;
		mBufferElementSize = 0;
		mBuffer			   = _ptr;
		mBufferUsed		   = 0;
		mBufferSize        = size;
		mBufferLock        = false;
	}


    VertexBufferView::VertexBufferView(FlexKit::byte* _ptr, size_t size, VERTEXBUFFER_FORMAT format, VERTEXBUFFER_TYPE type)
    {
        mBufferinError     = false;
		mBufferFormat      = format;
		mBufferType        = type;
		mBufferElementSize = (uint32_t)format;
		mBuffer			   = _ptr;
		mBufferUsed		   = size;
		mBufferSize        = size;
		mBufferLock        = true;
    }


	/************************************************************************************************/


	VertexBufferView::~VertexBufferView( void )
	{}


	/************************************************************************************************/


	VertexBufferView VertexBufferView::operator += ( const VertexBufferView& RHS )
	{	//TODO: THIS FUNCTION
        FK_ASSERT(RHS.GetBufferType() == GetBufferType() && GetBufferFormat() == RHS.GetBufferFormat());



		return *this;
	}


	/************************************************************************************************/


	VertexBufferView& VertexBufferView::operator = ( const VertexBufferView& RHS )
	{
		if( &RHS != this )
		{
			mBufferFormat      = RHS.GetBufferFormat();
			mBufferType        = RHS.GetBufferType();
			mBufferElementSize = RHS.GetElementSize();
			mBufferinError     = false;
			memcpy( &mBuffer[0], RHS.GetBuffer(), mBufferUsed );
		}
		return *this;
	}


	/************************************************************************************************/


	void VertexBufferView::Begin( VERTEXBUFFER_TYPE Type, VERTEXBUFFER_FORMAT Format )
	{
		mBufferinError         = false;
		mBufferType            = Type;
		mBufferFormat          = Format;
		mBufferUsed			   = 0;
		mBufferElementSize     = static_cast<uint32_t>( Format );
	}


	/************************************************************************************************/


	bool VertexBufferView::End()
	{
		return !mBufferinError;
	}


	/************************************************************************************************/


	bool VertexBufferView::UnloadBuffer()
	{
		mBufferUsed = 0;
		return true;
	}


	/************************************************************************************************/


	FlexKit::byte* VertexBufferView::GetBuffer() const
	{
		return mBuffer;
	}


	/************************************************************************************************/


	size_t VertexBufferView::GetElementSize() const
	{
		return mBufferElementSize;
	}


	/************************************************************************************************/


	size_t VertexBufferView::GetBufferSize() const // Size of elements
	{
		return mBufferSize / static_cast<uint32_t>( mBufferFormat );
	}


	/************************************************************************************************/


	size_t VertexBufferView::GetBufferSizeUsed() const
	{
		return mBufferUsed / static_cast<uint32_t>( mBufferFormat );
	}


	/************************************************************************************************/


	size_t VertexBufferView::GetBufferSizeRaw() const // Size of buffer in bytes
	{
		return mBufferUsed;
	}


	/************************************************************************************************/
	

	VERTEXBUFFER_FORMAT VertexBufferView::GetBufferFormat() const
	{
		return mBufferFormat;
	}
	

	/************************************************************************************************/
	

	VERTEXBUFFER_TYPE VertexBufferView::GetBufferType() const
	{
		return mBufferType;
	}
	

	/************************************************************************************************/


	void VertexBufferView::SetTypeFormatSize(VERTEXBUFFER_TYPE Type, VERTEXBUFFER_FORMAT Format, size_t count)
	{
		mBufferinError         = false;
		mBufferType            = Type;
		mBufferFormat          = Format;
		mBufferElementSize     = static_cast<uint32_t>( Format );
		mBufferUsed			   = mBufferElementSize * count;
	}


	/************************************************************************************************/
	

	FLEXKITAPI VertexBufferView* CreateVertexBufferView(iAllocator* allocator, const size_t BufferLength )
	{
        auto Memory = (FlexKit::byte*)allocator->_aligned_malloc(BufferLength);
		return new (Memory) VertexBufferView( Memory + sizeof(VertexBufferView), BufferLength - sizeof(VertexBufferView));
	}
	

	/************************************************************************************************/


    bool GenerateTangents(static_vector<VertexBufferView*>& buffers, iAllocator* allocator)
    {
        FK_LOG_2("GENERATING TANGENTS");
        auto GetBuffer = [&](auto VERTEXBUFFER_TYPE) -> VertexBufferView*
        {
            for (VertexBufferView* buffer : buffers)
                if (buffer && buffer->GetBufferType() == VERTEXBUFFER_TYPE)
                    return buffer;

            return nullptr;
        };

        bool UVsPresent     = buffers[2] != nullptr;
        auto indexBuffer    = GetBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX);
        auto vertices       = GetBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION)->CreateTypedProxy<float3>();
        auto normalBuffer   = GetBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL);
        auto normals        = normalBuffer->CreateTypedProxy<float3>();
        auto UVBuffer       = GetBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV);

        const size_t tangentBufferSize = normalBuffer->GetBufferSizeRaw() + sizeof(VertexBufferView);
        auto tangentBuffer = CreateVertexBufferView(allocator, tangentBufferSize);
        tangentBuffer->Begin(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32);


        auto GetIndex =
            [&](uint32_t index) -> uint32_t
        {
            if (indexBuffer)
                return indexBuffer->CreateTypedProxy<uint32_t>()[index];
            else
                return index;
        };

        auto GetTexcoord =
            [&](uint32_t index)
        {
            if (UVsPresent)
                return UVBuffer->CreateTypedProxy<float2>()[index];
            else
            {
                const auto temp = normals[index].cross(normals[index].zxy()).xy();
                return float2{ temp.x, temp.y };
            }
        };

        // Clear Tangents
        auto tangents   = tangentBuffer->CreateTypedProxy<float3>();
        const auto end  = normalBuffer->GetBufferSize();
        for (size_t itr = 0; itr < end; ++itr)
            tangentBuffer->Push(float3{ 0, 0, 0 });

        for (auto itr = 0; itr < end; itr += 3)
        {
            float3 V0 = vertices[GetIndex(itr + 0)];
            float3 V1 = vertices[GetIndex(itr + 1)];
            float3 V2 = vertices[GetIndex(itr + 2)];

            float2 UV1 = GetTexcoord(GetIndex(itr + 0));
            float2 UV2 = GetTexcoord(GetIndex(itr + 1));
            float2 UV3 = GetTexcoord(GetIndex(itr + 2));

            float3 edge1 = V1 - V0;
            float3 edge2 = V2 - V0;

            float2 ST1 = UV2 - UV1;
            float2 ST2 = UV3 - UV1;

            float r = 1.0f / ((ST1.x * ST2.y) - (ST2.x * ST1.y));

            float3 S = r * float3(
                (ST2.y * edge1.x) - (ST1.y * edge2.x),
                (ST2.y * edge1.y) - (ST1.y * edge2.y),
                (ST2.y * edge1.z) - (ST1.y * edge2.z));


            /*
            float3 T = r * float3(
                (-ST1.x * edge1.x) + (ST2.x * edge2.x),
                (-ST1.x * edge1.y) + (ST2.x * edge2.y),
                (-ST1.x * edge1.z) + (ST2.x * edge2.z));
            */

            S.normalize();
            //T.normalize();


            tangents[GetIndex(itr + 0)] = S;
            tangents[GetIndex(itr + 1)] = S;
            tangents[GetIndex(itr + 2)] = S;
        }

        // Normalise Averaged Tangents
        for (size_t itr = 0; itr < end; itr++)
            tangents[itr] = tangents[itr].normal();

        FK_ASSERT(tangentBuffer->End() != false);

        buffers.push_back(tangentBuffer);

        return true;
    }


}   /************************************************************************************************/
