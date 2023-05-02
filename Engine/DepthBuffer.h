#pragma once

#include "containers.h"
#include "MathUtils.h"
#include "ResourceHandles.h"

namespace FlexKit
{
	class RenderSystem;


	struct DepthBuffer
	{
		DepthBuffer(RenderSystem& IN_renderSystem, uint2 IN_WH, bool useFloatFormat = true) :
			floatingPoint   { useFloatFormat    },
			renderSystem    { IN_renderSystem   },
			WH              { IN_WH             }
		{
			buffers.push_back(renderSystem.CreateDepthBuffer(WH, useFloatFormat, 1));
			buffers.push_back(renderSystem.CreateDepthBuffer(WH, useFloatFormat, 1));
			buffers.push_back(renderSystem.CreateDepthBuffer(WH, useFloatFormat, 1));

			renderSystem.SetDebugName(buffers[0], "DepthBuffer0");
			renderSystem.SetDebugName(buffers[1], "DepthBuffer1");
			renderSystem.SetDebugName(buffers[2], "DepthBuffer2");
		}

		~DepthBuffer()
		{
			Release();
		}


		DepthBuffer(const DepthBuffer& rhs)     = delete;
		DepthBuffer(const DepthBuffer&& rhs)    = delete;


		DepthBuffer& operator = (const DepthBuffer& rhs)    = delete;
		DepthBuffer& operator = (DepthBuffer&& rhs)         = delete;


		ResourceHandle Get() const noexcept
		{
			return buffers[idx];
		}

		ResourceHandle GetPrevious() const noexcept
		{
			auto previousIdx = (idx + 2) % 3;
			return buffers[previousIdx];
		}

		void Resize(uint2 newWH)
		{
			Release();

			WH = newWH;

			buffers.push_back(renderSystem.CreateDepthBuffer(WH, floatingPoint, 1));
			buffers.push_back(renderSystem.CreateDepthBuffer(WH, floatingPoint, 1));
			buffers.push_back(renderSystem.CreateDepthBuffer(WH, floatingPoint, 1));
		}

		void Release()
		{
			renderSystem.ReleaseResource(buffers.pop_back());
			renderSystem.ReleaseResource(buffers.pop_back());
			renderSystem.ReleaseResource(buffers.pop_back());
		}

		void Increment() noexcept
		{
			idx = (idx + 1) % 3;
		}

		RenderSystem&						renderSystem;
		uint								idx = 0;
		bool								floatingPoint;
		uint2								WH;
		CircularBuffer<ResourceHandle, 3>	buffers;
	};

}


/**********************************************************************

Copyright (c) 2016-2022 Robert May

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

