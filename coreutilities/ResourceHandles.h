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

#ifndef RESOURCEHANDLES_H_INCLUDED
#define RESOURCEHANDLES_H_INCLUDED

#include "..\coreutilities\Handle.h"
#include "..\coreutilities\type.h"

namespace FlexKit
{
	typedef Handle_t<32, GetTypeGUID(ConstantBuffer)>			ConstantBufferHandle;
	typedef Handle_t<32, GetTypeGUID(VertexBuffer)>				VertexBufferHandle;
    typedef Handle_t<32, GetTypeGUID(ResourceHandle)>		    ResourceHandle;
    typedef Handle_t<32, GetTypeGUID(ReadBackResourceHandle)>   ReadBackResourceHandle;
	typedef Handle_t<32, GetTypeGUID(StreamTexture2DHandle)>	StreamingTexture2DHandle;
    typedef Handle_t<32, GetTypeGUID(ShaderResourceHandle)>		ShaderResourceHandle;
    typedef Handle_t<32, GetTypeGUID(UAVResourceHandle)>		UAVResourceHandle;
	typedef Handle_t<32, GetTypeGUID(UAVTextureHandle)>			UAVTextureHandle;
	typedef Handle_t<32, GetTypeGUID(SOResourceHandle)>			SOResourceHandle;
	typedef Handle_t<32, GetTypeGUID(QueryBuffer)>				QueryHandle;
	typedef Handle_t<16, GetTypeGUID(TriMesh)>					TriMeshHandle;
    typedef Handle_t<32, GetTypeGUID(CopyContextHandle)>		CopyContextHandle;

}

#endif
