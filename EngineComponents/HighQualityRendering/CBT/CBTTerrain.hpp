#include "CBT.hpp"
#include "MathUtilities.hpp"
#include "FrameGraph.hpp"

#include <filesystem>

namespace FlexKit
{
	struct CBTTerain
	{
		CBTTerain(iAllocator& persistent, IRenderSystem& renderSystem);

		static constexpr PSOHandle AdaptiveTerrainUpdateArgs	= PSOHandle(GetTypeGUID(AdaptiveTerrainUpdateArgs));
		static constexpr PSOHandle AdaptiveTerrainUpdate		= PSOHandle(GetTypeGUID(AdaptiveTerrainUpdate));
		static constexpr PSOHandle AdaptiveTerrainDrawArgs		= PSOHandle(GetTypeGUID(AdaptiveTerrainDrawArgs));
		static constexpr PSOHandle RenderTerrain				= PSOHandle(GetTypeGUID(DrawCBTTree));
		static constexpr PSOHandle RenderTerrainWireframe		= PSOHandle(GetTypeGUID(DrawCBTTreeWireframe));

		static void RegisterAdaptiveUpdateCBT(IRenderSystem& renderSystem);

		void LoadHeightMapFromPath(IRenderSystem& renderSystem, std::filesystem::path heightMapPath, iAllocator& allocator);
		void SetHeightMap(ResourceHandle handle);

		void AdaptiveLODUpdate(CameraHandle camera, ReserveConstantBufferFunction& cbAllocator, FrameGraph& frameGraph, double dT);
		void Render(CameraHandle camera, UpdateTask* update, ResourceHandle renderTarget, ResourceHandle depthTarget, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph);
		void Upload(FrameGraph& frameGraph);

		CBTBuffer				cbt;
		ResourceHandle			heightMap = InvalidHandle;
		DescriptorRange			textureDesc;
		bool					wireframe = false;

		inline static IndirectLayout	indirectDispatchLayout;
		inline static IndirectLayout	indirectDrawLayout;
	};
}


/**********************************************************************

Copyright (c) 2024 Robert May

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
