#include "Graphics.hpp"
#include "OcclusionCulling.hpp"

namespace FlexKit
{
	OcclusionQueries::~OcclusionQueries()
	{
		if (occlusionQueries != InvalidHandle)
		{
			RenderSystem::_GetInstance().ReleaseQuery(occlusionQueries);
			RenderSystem::_GetInstance().ReleaseResource(occlusionResults);
		}
		occlusionQueries = InvalidHandle;
	}

	uint32_t OcclusionQueries::GetQueryIdx(uint32_t id)
	{
		std::scoped_lock sl{ m };

		return *drawableOffsetMappings.Find_Or(id, counter++);
	}

	void PassHistory::EndFrame()
	{
		Current().counter = 0;
		idx = (idx + 1) % 3;
		Current().drawableOffsetMappings.clear();
	}

	OcclusionQueries& PassHistory::PreviousHistory()
	{
		return occlusionQueryHistory[idx + 2];
	}

	OcclusionQueries& PassHistory::Current()
	{
		return occlusionQueryHistory[idx];
	}

	std::optional<uint32_t> PassHistory::QueryPrevious(uint32_t id) const
	{
		auto res = occlusionQueryHistory[idx + 2].drawableOffsetMappings.find(id);
		if (res)
			return { *res };
		else
			return{};
	}


	PassHistory* PassHistoryTable::GetHistory(class RenderSystem& renderSystem, CameraHandle camera)
	{
		auto res = passState.find(camera);

		if (!res)
		{
			res = passState.insert(
				camera,
				PassHistory{});

			res->occlusionQueryHistory.emplace_back(
				renderSystem.CreateOcclusionBuffer(4096),
				renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(4096 * 8)),
				allocator);

			res->occlusionQueryHistory.emplace_back(
				renderSystem.CreateOcclusionBuffer(4096),
				renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(4096 * 8)),
				allocator);

			res->occlusionQueryHistory.emplace_back(
				renderSystem.CreateOcclusionBuffer(4096),
				renderSystem.CreateGPUResource(GPUResourceDesc::UAVResource(4096 * 8)),
				allocator);
		}

		return res;
	}

	void PassHistoryTable::ResetAll()
	{
		for (size_t i = 0; i < passState.max; i++)
		{
			if (passState.keys[i] != 0xffffffff)
			{
				for (auto& qh : passState.values[i].occlusionQueryHistory)
					qh.drawableOffsetMappings.clear();
			}
		}
	}
}




/**********************************************************************

Copyright (c) 2014-2024 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following condi8tions:

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
