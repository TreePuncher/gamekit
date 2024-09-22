#include <Application.hpp>
#include <Win32Graphics.hpp>

struct HelloWorldState : FlexKit::FrameworkState
{
	HelloWorldState(FlexKit::GameFramework& in_framework) : 
		FlexKit::FrameworkState{ in_framework }
	{
		renderWindow = FlexKit::CreateWin32RenderWindow(framework.GetRenderSystem(), 
			{
				.height = 600,
				.width	= 800,
			});

		FlexKit::EventNotifier<>::Subscriber sub;
		sub.Notify	= &FlexKit::EventsWrapper;
		sub._ptr	= &framework;
		renderWindow->Handler.Subscribe(sub);
		renderWindow->SetWindowTitle("HelloWorld");

		vertexBuffer = framework.GetRenderSystem().CreateVertexBuffer(512, false);

		framework.GetRenderSystem().RegisterPSOLoader(FlexKit::DRAW_PSO,
			[](FlexKit::RenderSystem* renderSystem, FlexKit::iAllocator& allocator)
			{
				return FlexKit::CreateDrawTriStatePSO(renderSystem, allocator);
			});
	}

	FlexKit::UpdateTask* Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher&, double dT) override
	{ 
		FlexKit::UpdateInput();

		return nullptr; 
	};

	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph) override
	{ 
		auto color			= FlexKit::float4{ (rand() % 1024) / 1024.0f, (rand() % 1024) / 1024.0f, (rand() % 1024) / 1024.0f, 1 };
		auto vbPushBuffer	= FlexKit::CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemoryMT());

		frameGraph.AddOutput(renderWindow->GetBackBuffer());

		FlexKit::ClearBackBuffer(frameGraph, renderWindow->GetBackBuffer(), color);

		struct Trangle 
		{
			FlexKit::ReserveVertexBufferFunction	reserveVB;
			FlexKit::FrameResourceHandle			renderTarget;
		};

		frameGraph.AddNode<Trangle>(
			{
				.reserveVB = vbPushBuffer
			}, 
			[&](FlexKit::FrameGraphNodeBuilder& builder, Trangle& trangle)
			{
				builder.Requires(FlexKit::DRAW_PSO);
				trangle.renderTarget = builder.RenderTarget(renderWindow->GetBackBuffer());
			}, 
			[](Trangle& trangle, FlexKit::ResourceHandler& resources, FlexKit::Context& ctx, FlexKit::iAllocator& threadLocalAllocator)
			{
				struct Vertex
				{
					FlexKit::float2 XY;
					FlexKit::float2 UV;
					float color[3] = { 1.0f, 1.0f, 1.0f };
				} triangle[3] =
				{
					{
						.XY { -1.0f, -1.0f }
					},
					{
						.XY { 0.0f, 1.0f }
					},
					{
						.XY { 1.0f, -1.0f }
					}
				};


				FlexKit::VBPushBuffer			vbPushAllocator{ trangle.reserveVB(512) };
				FlexKit::VertexBufferDataSet	vb{ triangle, vbPushAllocator };

				auto renderTarget = resources.RenderTarget(trangle.renderTarget, ctx);
				ctx.SetGraphicsPipelineState(FlexKit::DRAW_PSO, threadLocalAllocator);
				ctx.SetVertexBuffers({ vb });
				ctx.SetRenderTargets({ renderTarget });
				ctx.SetScissorAndViewports({ renderTarget });
				ctx.Draw(3);
			});

		FlexKit::PresentBackBuffer(frameGraph, renderWindow->GetBackBuffer());

		return nullptr; 
	}

	void PostDrawUpdate(FlexKit::EngineCore& core, double dT) override
	{
		renderWindow->Present(1);
	}

	bool EventHandler(FlexKit::Event evt) 
	{
		return false; 
	}

	FlexKit::VertexBufferHandle	vertexBuffer;
	FlexKit::Win32RenderWindow* renderWindow = nullptr;
};

int main()
{
	auto memory = FlexKit::CreateEngineMemory();
	FlexKit::FKApplication app{ memory };

	app.PushState<HelloWorldState>();
	app.Run();

	return 0;
}