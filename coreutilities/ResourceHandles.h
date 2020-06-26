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
	typedef Handle_t<32u, GetTypeGUID(ConstantBuffer)>			ConstantBufferHandle;
	typedef Handle_t<32u, GetTypeGUID(VertexBuffer)>			VertexBufferHandle;
    typedef Handle_t<32u, GetTypeGUID(ResourceHandle)>		    ResourceHandle;
    typedef Handle_t<32u, GetTypeGUID(ReadBackResourceHandle)>  ReadBackResourceHandle;
	typedef Handle_t<32u, GetTypeGUID(StreamTexture2DHandle)>	StreamingTexture2DHandle;
    typedef Handle_t<32u, GetTypeGUID(ShaderResourceHandle)>	ShaderResourceHandle;
    typedef Handle_t<32u, GetTypeGUID(UAVResourceHandle)>		UAVResourceHandle;
	typedef Handle_t<32u, GetTypeGUID(UAVTextureHandle)>		UAVTextureHandle;
	typedef Handle_t<32u, GetTypeGUID(SOResourceHandle)>		SOResourceHandle;
	typedef Handle_t<32u, GetTypeGUID(QueryBuffer)>				QueryHandle;
	typedef Handle_t<16u, GetTypeGUID(TriMesh)>					TriMeshHandle;
    typedef Handle_t<32u, GetTypeGUID(CopyContextHandle)>		CopyContextHandle;
    typedef Handle_t<32u, GetTypeGUID(DeviceHeapHandle)>        DeviceHeapHandle;

    constexpr uint32_t MaterialComponentID = GetTypeGUID(MaterialID);
    using MaterialHandle = Handle_t <32, MaterialComponentID>;
}

#endif
