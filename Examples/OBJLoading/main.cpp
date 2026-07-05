#include <ExampleFramework.hpp>
#include <FrameGraph.hpp>
#include <objloader.hpp>
#include <RenderSystemInterface.hpp>
#include <ResourceHandles.hpp>
#include <TriMeshResource.hpp>

using namespace FlexKit;

constexpr GUID_t VertexShaderAssetID	= GetCRCGUID(VertexShader);
constexpr GUID_t PixelShaderAssetID		= GetCRCGUID(PixelShader);

struct ObjLoaderExample : ExampleState
{
	ObjLoaderExample()
	{
		GetRenderSystem().RegisterPSOLoader(GetTypeGUID(Trangle),
			[](IRenderSystem& renderSystem, iAllocator& allocator)
			{
				PipelineBuilder builder(renderSystem, allocator);
				builder.AddInputLayout({
					.inputs = {
						{
							.name			= "POSITION",
							.index			= 0,
							.format			= DeviceFormat::R32G32B32_FLOAT,
							.inputSlotClass = EInputClassification::PerVertex,
						}},
					.count = 1
					});
				//builder.AddVertexShader("VMain");
				//builder.AddPixelShader("PMain");
				builder.AddVertexShader("VMain", "assets/shaders/TestShader.hlsl");
				builder.AddPixelShader("PMain", "assets/shaders/TestShader.hlsl");
				builder.AddRasterizerState();
				builder.AddRenderTargetState({
						.targetCount	= 1,
						.targetFormats	= { DeviceFormat::R16G16B16A16_FLOAT },
					});

				return builder.Build(renderSystem, allocator);
			});

		GetRenderSystem().QueuePSOLoad(GetTypeGUID(Trangle));

		shape = LoadObj("Test.obj");
	}

	virtual ~ObjLoaderExample() override
	{
		ReleaseMesh(shape);
	}

	UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT) override
	{
		t += dT;

	    return nullptr;
	}


	UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher&, double dT, FrameGraph& frameGraph) override
	{
		struct DrawTrangle
		{
			FrameResourceHandle renderTarget;
		};

		auto node = frameGraph.AddNode2(
			[&](FrameGraphNodeBuilder& ){},
			[](const ResourceHandler&, IDirectContext&, iAllocator&){});

		frameGraph.AddNode2(
			[&](FrameGraphNodeBuilder& builder) -> DrawTrangle
			{
				builder.Requires(GetTypeGUID(Trangle));
				builder.AddNodeDependency(node);

			    return DrawTrangle{
				    .renderTarget = builder.RenderTarget(GetRenderWindow().GetBackBuffer())
			    };
			},
			[=, this](const DrawTrangle& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
			{
				float fTime = (float)t;
				
				struct
				{
					float time;
				} constants0{
					.time = fTime,
				};

				const IPipelineInterface* pipelineInterface = resources.GetPipelineState(GetTypeGUID(Trangle), threadLocalAllocator)->GetInterface();

				ctx.SetGraphicsPipelineState(GetTypeGUID(Trangle), threadLocalAllocator);

				auto mesh = GetMeshResource(shape);
				auto& lod = mesh->lods[0];

				ctx.SetInputPrimitive(EInputPrimitive::INPUTPRIMITIVETRIANGLELIST);
				ctx.AddVertexBuffers(mesh, 0, { VERTEXBUFFER_TYPE::POSITION });
				ctx.AddIndexBuffer(mesh, 0);

				ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
				ctx.SetRenderTargets({ resources.GetResource(data.renderTarget) });
				ctx.SetGraphicsConstantValue(0, 1, &constants0);


				ctx.DrawIndexed(lod.GetIndexCount());
			});

	    return nullptr;
	}

	double			t = 0.0;
	TriMeshHandle	shape = InvalidHandle;
};

int main()
{
	return RunExample<ObjLoaderExample>();
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
