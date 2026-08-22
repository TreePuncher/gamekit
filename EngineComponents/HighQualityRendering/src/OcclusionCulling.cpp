#include "RenderSystemInterface.hpp"
#include "OcclusionCulling.hpp"

namespace FlexKit
{
	OcclusionQueries::OcclusionQueries(
		QueryHandle			IN_occlusionQueries,
		ResourceHandle		IN_occlusionResults,
		iAllocator&			allocator) :
		occlusionQueries{ IN_occlusionQueries },
		occlusionResults{ IN_occlusionResults },
		drawableOffsetMappings{ allocator }
	{
		IRenderSystem::GetInstance().SetDebugName(IN_occlusionResults, "OcclusionResultsBuffer");
	}


	OcclusionQueries::~OcclusionQueries()
	{
		if (occlusionQueries != InvalidHandle)
		{
			IRenderSystem::GetInstance().ReleaseQuery(occlusionQueries);
			IRenderSystem::GetInstance().ReleaseResource(occlusionResults);
		}
		occlusionQueries = InvalidHandle;
	}


	OcclusionQueries::OcclusionQueries(OcclusionQueries&& rhs) noexcept :
		counter{ std::exchange(rhs.counter, 0) },
		occlusionQueries{ std::exchange(rhs.occlusionQueries, InvalidHandle) },
		occlusionResults{ std::exchange(rhs.occlusionResults, InvalidHandle) } {}


	OcclusionQueries& OcclusionQueries::operator = (OcclusionQueries&& rhs) noexcept
	{
		counter				= std::exchange(rhs.counter, 0);
		occlusionQueries	= std::exchange(rhs.occlusionQueries, InvalidHandle);
		occlusionResults	= std::exchange(rhs.occlusionResults, InvalidHandle);

		return *this;
	}


	uint32_t OcclusionQueries::GetQueryIdx(uint32_t id)
	{
		std::scoped_lock sl{ m };

		auto res = drawableOffsetMappings.find(id);
		if (res)
			return *res;
		else
		{
			const uint32_t idx = counter++;
			drawableOffsetMappings.emplace(id, idx);
			return idx;
		}
	}


	void PassHistory::EndFrame()
	{
		idx = (idx + 1) % 3;
		Current().drawableOffsetMappings.clear();
		Current().counter = 0;
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


	PassHistory* PassHistoryTable::GetHistory(class IRenderSystem& renderSystem, CameraHandle camera)
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
