#pragma once
#include "ResourceHandles.h"
#include "containers.h"


namespace FlexKit
{
	struct OcclusionQueries
	{
		OcclusionQueries() = default;

		OcclusionQueries(
			QueryHandle			IN_occlusionQueries,
			ResourceHandle		IN_occlusionResults,
			iAllocator&			allocator	) :
				occlusionQueries		{ IN_occlusionQueries },
				occlusionResults		{ IN_occlusionResults },
				drawableOffsetMappings	{ allocator } {}

		~OcclusionQueries();

							OcclusionQueries	(const OcclusionQueries&)	= delete;
		OcclusionQueries&	operator =			(OcclusionQueries& rhs)		= delete;

		OcclusionQueries(OcclusionQueries&& rhs) :
			counter				{ std::exchange(rhs.counter, 0) },
			occlusionQueries	{ std::exchange(rhs.occlusionQueries, InvalidHandle) },
			occlusionResults	{ std::exchange(rhs.occlusionResults, InvalidHandle) } {}

		OcclusionQueries& operator = (OcclusionQueries&& rhs)
		{
			counter				= std::exchange(rhs.counter, 0);
			occlusionQueries	= std::exchange(rhs.occlusionQueries, InvalidHandle);
			occlusionResults	= std::exchange(rhs.occlusionResults, InvalidHandle);
		}

		uint32_t GetQueryIdx(uint32_t);

		uint32_t						counter = 0;
		QueryHandle						occlusionQueries	= InvalidHandle;
		ResourceHandle					occlusionResults	= InvalidHandle;
		HashTable<uint32_t, uint32_t>	drawableOffsetMappings;
	};

	struct PassHistory
	{
		CircularBuffer<OcclusionQueries, 3> occlusionQueryHistory;
		uint32_t							idx;

		void				EndFrame();
		OcclusionQueries&	PreviousHistory();
		OcclusionQueries&	Current();
	};

	class PassHistoryTable
	{
	public:
		PassHistoryTable(iAllocator& IN_allocator)
			: passState{ IN_allocator }
			, allocator{ IN_allocator } {}

		PassHistory* GetHistory(class RenderSystem&, CameraHandle);

		void ResetAll();

		HashTable<PassHistory, uint32_t>	passState;
		iAllocator& allocator;
	};
}


/**********************************************************************

Copyright (c) 2014-2024 Robert May

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
