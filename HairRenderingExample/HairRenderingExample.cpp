#include "HairRenderingExample.hpp"
#include <Win32Graphics.h>
#include <FrameGraph.h>
#include <fmt/printf.h>
#include <fmt/format.h>
#include <cstdio>
#include <fp16.h>
#include <scn/scn.h>
#include <ranges>
#include <imgui.h>

/************************************************************************************************/


using namespace FlexKit;

constexpr FlexKit::PSOHandle StrandRenderPSO				= FlexKit::PSOHandle{ GetCRCGUID(StrandRenderPSO) };
constexpr FlexKit::PSOHandle BlendStatePSO					= FlexKit::PSOHandle{ GetCRCGUID(BlendStatePSO) };
constexpr FlexKit::PSOHandle DebugRenderPSO					= FlexKit::PSOHandle{ GetCRCGUID(DebugRenderPSO) };
constexpr FlexKit::PSOHandle ApplyForcesPSO					= FlexKit::PSOHandle{ GetCRCGUID(ApplyForcesPSO) };
constexpr FlexKit::PSOHandle ApplyShapeConstraintsPSO		= FlexKit::PSOHandle{ GetCRCGUID(ApplyShapeConstraintsPSO) };
constexpr FlexKit::PSOHandle ApplyEdgeLengthConstraintPSO	= FlexKit::PSOHandle{ GetCRCGUID(ApplyEdgeLengthConstraintPSO) };


/************************************************************************************************/


HairStyle CreateStyle(FlexKit::RenderSystem& renderSystem, const uint32_t strandLength, const uint32_t strandCount)
{
	HairStyle style;

	const size_t bufferSize = sizeof(ControlPoint) * strandLength* strandCount;

	style.hairBuffers[0] = renderSystem.CreateUAVBufferResource(bufferSize, false);
	style.hairBuffers[1] = renderSystem.CreateUAVBufferResource(bufferSize, false);

	style.strandbuffer	= renderSystem.CreateUAVBufferResource(bufferSize, false);
	style.styleBuffer	= renderSystem.CreateGPUResource(GPUResourceDesc::StructuredResource((uint32_t)bufferSize));

	style.strandCount	= strandCount;
	style.strandLength	= strandLength;

	return style;
}


void ReleaseStyle(HairStyle& style, FlexKit::RenderSystem& renderSystem)
{
	renderSystem.ReleaseResource(style.hairBuffers[0]);
	renderSystem.ReleaseResource(style.hairBuffers[1]);
	renderSystem.ReleaseResource(style.strandbuffer);
	renderSystem.ReleaseResource(style.styleBuffer);
}


/************************************************************************************************/


void UploadHairStyle(HairStyle& style, const ImportedStyleBuffer& stylePoints, FlexKit::RenderSystem& renderSystem)
{
	auto copyCtx = renderSystem.GetImmediateCopyQueue();
	renderSystem.UpdateResourceByUploadQueue(renderSystem.GetDeviceResource(style.hairBuffers[0]), copyCtx, stylePoints.controlPoints.data(), stylePoints.controlPoints.ByteSize(), 1, DASCommon);
	renderSystem.UpdateResourceByUploadQueue(renderSystem.GetDeviceResource(style.styleBuffer), copyCtx, stylePoints.controlPoints.data(), stylePoints.controlPoints.ByteSize(), 1, DASCommon);

	style.currentBuffer = 0;
}


/************************************************************************************************/


std::expected<ImportedStyleBuffer, int> ImportCSV(const std::filesystem::path& path, FlexKit::iAllocator& allocator)
{
	if (!std::filesystem::is_regular_file(path))
		return std::unexpected{ 1 };

	FILE* f = nullptr;
	auto err = fopen_s(&f, path.string().c_str(), "rb");

	if (err)
		return std::unexpected{ 2 };

	const size_t fileSize = std::filesystem::file_size(path);

	std::string buffer;
	buffer.resize(fileSize);

	const size_t bytesRead = fread(buffer.data(), 1, fileSize, f);
	buffer.resize(bytesRead + 1);

	constexpr std::string_view commaSeperator{ "," };
	constexpr std::string_view endLineDelim{ "\n" };
	
	FlexKit::Vector<ControlPoint> floats{ allocator };
	floats.reserve(4096);

	uint32_t lineCount = 0;
	uint32_t maxColumnCount = 0;

	bool resetColumn = true;
	for (const auto line : std::ranges::split_view(buffer, endLineDelim))
	{
		if (line.size())
		{
			lineCount++;

			resetColumn = true;
			uint32_t columnCount = 0;

			for (const auto stringView : std::ranges::split_view(line, commaSeperator) | std::views::transform([&](auto a)
				{
					if (a.begin() < buffer.end() && a.end() < buffer.end() && *a.end() != '\0')
					{
						auto b = a.begin();
						auto e = a.end();
						bool endline = false;
						bool fileEnd = false;

						while (b < buffer.end() && (std::isspace(*b) || *b == ','))
						{
							if (*b == '\n') endline = true;
							if (*b == '\0') fileEnd = true;
							b++;
						}
						while (e < buffer.end() && std::isspace(*e)) e--;

						return std::string_view{ b, e };
					}
					else return std::string_view{};
				}))
			{
				float x, y, z;

				if (scn::scan(stringView, "({} {} {})", x, y, z))
				{
					const auto d	= resetColumn ? FlexKit::float3(0, 0, 0) : (float3{ floats.back().position[0], floats.back().position[1], floats.back().position[2] });
					const float l	= resetColumn ? 0.0f : (d - float3{ x, y, z }).magnitude();

					ControlPoint controlPoint{
						.position	= { x, y, z },
						.w			= fp16_ieee_from_fp32_value(resetColumn ? 0.2f : 1.0f),
						.l			= fp16_ieee_from_fp32_value(l)
					};

					floats.emplace_back(controlPoint);

					resetColumn = false;
					columnCount++;
				}
			}

			maxColumnCount = FlexKit::Max(maxColumnCount, columnCount);
		}
	}

	fclose(f);

	return ImportedStyleBuffer{
		.strandLength	= maxColumnCount,
		.strandCount	= (uint32_t)floats.size() / maxColumnCount,
		.controlPoints	= std::move(floats) };
}


/************************************************************************************************/


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateApplyForcesPSO(iAllocator& tempMemory)
{
	PipelineBuilder builder{ tempMemory };
	builder.AddComputeShader("ApplyForces", R"(assets\shaders\HairRendering\Simulation.hlsl)", { .enable16BitTypes = true });
	builder.SetDebugName("ApplyForces");

	return builder.Build(framework.GetRenderSystem());
}


/************************************************************************************************/


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateApplyShapeConstraintsPSO(iAllocator& tempMemory)
{
	PipelineBuilder builder{ tempMemory };
	builder.AddComputeShader("ApplyShapeConstraints", R"(assets\shaders\HairRendering\Simulation.hlsl)", { .enable16BitTypes = true });
	builder.SetDebugName("ApplyShapeConstraints");

	return builder.Build(framework.GetRenderSystem());
}


/************************************************************************************************/


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateApplyEdgeLengthConstraintPSO(iAllocator& tempMemory)
{
	PipelineBuilder builder{ tempMemory };
	builder.AddComputeShader("ApplyEdgeLengthContraints", R"(assets\shaders\HairRendering\Simulation.hlsl)", { .enable16BitTypes = true });
	builder.SetDebugName("ApplyEdgeLengthContraints");

	return builder.Build(framework.GetRenderSystem());
}


/************************************************************************************************/


LoadPipelineStateRes HairRenderingTest::CreateStrandRenderPSO(iAllocator& tempMemory)
{
	PipelineBuilder builder{ tempMemory };

	builder.AddVertexShader		("VMain",	R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });
	builder.AddGeometryShader	("GMain",	R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });
	builder.AddPixelShader		("PS_Draw", R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });

	builder.AddInputTopology(ETopology::EIT_POINT);
	builder.AddInputLayout({
		.inputs = { { "POSITION",	0, DeviceFormat::R32G32B32A32_FLOAT, 0, 0,	EInputClassification::PerVertex, 0 }, },
		.count	= 1
		});

	builder.SetDebugName("DrawMLAB");

	return builder.Build(framework.core.RenderSystem);
}


/************************************************************************************************/


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateBlendState(iAllocator& tempMemory)
{
	PipelineBuilder builder{ tempMemory };

	builder.AddVertexShader		("VS_FullScreen",	R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true });
	builder.AddPixelShader		("PS_Blend",		R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true });

	builder.AddInputTopology(ETopology::EIT_TRIANGLE);

	builder.AddRasterizerState({
			.CullMode	= ECullMode::NONE
		});

	builder.AddRenderTargetState({
			.targetCount	= 1,
			.targetFormats	= { DeviceFormat::R16G16B16A16_FLOAT },
		});

	builder.SetDebugName("Blend");

	return builder.Build(framework.core.RenderSystem);
}


/************************************************************************************************/


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateDebugRenderPSO(iAllocator& tempMemory)
{
	PipelineBuilder builder{ tempMemory };
	builder.AddVertexShader		("VMain",	R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true });
	builder.AddGeometryShader	("GDebug",	R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true });
	builder.AddPixelShader		("PDebug",	R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true });

	builder.AddInputTopology(EIT_POINT);

	builder.AddInputLayout({
			.inputs	= { { "POSITION",	0, DeviceFormat::R32G32B32A32_FLOAT, 0, 0,	EInputClassification::PerVertex, 0 }, }, 
			.count = 1
		});
	
	builder.AddDepthStencilState({
			.depthEnable	= false,
		});
	
	builder.AddRenderTargetState({
			.targetCount	= 1,
			.targetFormats	= { DeviceFormat::R16G16B16A16_FLOAT },
		});

	builder.SetDebugName("DrawStrandsDebug");

	return builder.Build(framework.core.RenderSystem);
}


/************************************************************************************************/


HairRenderingTest::HairRenderingTest(GameFramework& IN_framework) :
	FrameworkState				{ IN_framework },
	vertexBuffer				{ IN_framework.GetRenderSystem().CreateVertexBuffer(16 * MEGABYTE, false) },
	constantBuffer				{ IN_framework.GetRenderSystem().CreateConstantBuffer(16 * MEGABYTE, false) },
	cameras						{ IN_framework.core.GetBlockMemory() },
	runOnceQueue				{ IN_framework.core.GetBlockMemory() },
	depthBuffer					{ IN_framework.GetRenderSystem().CreateDepthBuffer({ 1920, 1080 }, true) },
	debugUI						{ IN_framework.GetRenderSystem(), IN_framework.core.GetBlockMemory() },
	gpuAllocator				{ IN_framework.GetRenderSystem(), 1024 * MEGABYTE, 64 * KILOBYTE, DeviceHeapFlags::UAVTextures | DeviceHeapFlags::UAVBuffer, IN_framework.core.GetBlockMemory() }
{
	if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
		renderWindow = std::move(res.value());
	else
		throw std::runtime_error{ "Unable to create render window!" };

	EventNotifier<>::Subscriber sub;
	sub.Notify = &EventsWrapper;
	sub._ptr = &framework;

	renderWindow.Handler->Subscribe(sub);
	renderWindow.SetWindowTitle("Hair Rendering - WIP");


	framework.GetRenderSystem().RegisterPSOLoader(ApplyForcesPSO,				[&](auto renderSystem, auto& allocator) { return CreateApplyForcesPSO(allocator); });
	framework.GetRenderSystem().RegisterPSOLoader(ApplyShapeConstraintsPSO,		[&](auto renderSystem, auto& allocator) { return CreateApplyShapeConstraintsPSO(allocator); });
	framework.GetRenderSystem().RegisterPSOLoader(ApplyEdgeLengthConstraintPSO,	[&](auto renderSystem, auto& allocator) { return CreateApplyEdgeLengthConstraintPSO(allocator); });

	framework.GetRenderSystem().RegisterPSOLoader(StrandRenderPSO,	[&](auto renderSystem, auto& allocator) { return CreateStrandRenderPSO(allocator); });
	framework.GetRenderSystem().RegisterPSOLoader(BlendStatePSO,	[&](auto renderSystem, auto& allocator) { return CreateBlendState(allocator); });

	framework.GetRenderSystem().QueuePSOLoad(ApplyForcesPSO);
	framework.GetRenderSystem().QueuePSOLoad(ApplyShapeConstraintsPSO);
	framework.GetRenderSystem().QueuePSOLoad(ApplyEdgeLengthConstraintPSO);

	framework.GetRenderSystem().QueuePSOLoad(StrandRenderPSO);
	framework.GetRenderSystem().QueuePSOLoad(BlendStatePSO);

	camera		= cameras.CreateCamera();
	cameraRig	= GetZeroedNode();
	cameras.SetCameraNode(camera, GetZeroedNode());

	auto cameraNode = cameras.GetCamera(camera).Node;
	FlexKit::SetParentNode(cameraRig, cameraNode);
	FlexKit::TranslateWorld(cameraNode, float3(0, 0, 15));

	if (const auto import = ImportCSV(R"(assets\hair.csv)", framework.core.GetTempMemory()); import)
	{
		const auto& controlPoints = import.value();
		style = CreateStyle(framework.GetRenderSystem(), controlPoints.strandLength, controlPoints.strandCount);
		UploadHairStyle(style, controlPoints, framework.GetRenderSystem());
	}
}


/************************************************************************************************/


HairRenderingTest::~HairRenderingTest()
{
	ReleaseStyle(style, framework.GetRenderSystem());

	framework.GetRenderSystem().ReleaseVB(vertexBuffer);
	framework.GetRenderSystem().ReleaseResource(depthBuffer);
	framework.GetRenderSystem().ReleaseCB(constantBuffer);
}


/************************************************************************************************/


void HairRenderingTest::ClearStyleBuffers(HairStyle& style)
{
	runOnceQueue.push_back(
		[&](FlexKit::UpdateDispatcher& dispatcher, FlexKit::FrameGraph& frameGraph)
		{
			struct Buffers
			{
				FrameResourceHandle strand;
				FrameResourceHandle hair1;
				FrameResourceHandle hair2;
			};

			frameGraph.AddNode<Buffers> (
				Buffers{},
				[&](FrameGraphNodeBuilder& builder, Buffers& data)
				{
					data.strand = builder.UnorderedAccess(style.strandbuffer);
					data.hair1 = builder.UnorderedAccess(style.hairBuffers[0]);
					data.hair2 = builder.UnorderedAccess(style.hairBuffers[1]);
				},
				[=](Buffers& buffers, const ResourceHandler& resources, Context& ctx, iAllocator& threadLocalAllocator)
				{
					ctx.ClearUAVBuffer(resources.GetResource(buffers.hair1));
					ctx.ClearUAVBuffer(resources.GetResource(buffers.hair2));
					ctx.ClearUAVBuffer(resources.GetResource(buffers.strand));
				});
		});
}


/************************************************************************************************/


UpdateTask* HairRenderingTest::Update(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT)
{
	UpdateInput();
	renderWindow.UpdateCapturedMouseInput(dT);


	auto cameraNode = cameras.GetCamera(camera).Node;
	FlexKit::Yaw(cameraRig, pi / 8.0f * dT);
	FlexKit::SetCameraAspectRatio(camera, renderWindow.GetAspectRatio());
	cameras.MarkDirty(camera);
	
	auto& transformUpdate	= FlexKit::QueueTransformUpdateTask(dispatcher);
	auto& cameraUpdate		= cameras.QueueCameraUpdate(dispatcher);

	cameraUpdate.AddInput(transformUpdate);

	debugUI.Update(renderWindow, core, dispatcher, dT);

	counter++;


	ImGui::NewFrame();
	ImGui::Begin("Hello");

	auto str = fmt::format(
		"FrameRate: {}hz\n"
		"Update Time: {}ms\n",
		fps, framework.stats.du_average);

	ImGui::Text(str.c_str());

	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();

	T += dT;

	if (T >= 1.0f)
	{
		T = 0;
		fps = counter;
		counter = 0;
	}

	return &cameraUpdate;
}


/************************************************************************************************/


void HairRenderingTest::Simulate(
	FlexKit::UpdateTask*					update,
	FlexKit::EngineCore&					core,
	FlexKit::UpdateDispatcher&				dispatcher,
	double									dT,
	FlexKit::FrameGraph&					frameGraph,
	FlexKit::ReserveVertexBufferFunction&	reserveVB,
	FlexKit::ReserveConstantBufferFunction&	reserveCB)
{
	struct RenderStrands
	{
		FlexKit::FrameResourceHandle			sourceBuffer;
		FlexKit::FrameResourceHandle			destinationTarget;
		FlexKit::FrameResourceHandle			strandBuffer;
		FlexKit::FrameResourceHandle			styleBuffer;
		FlexKit::ReserveVertexBufferFunction	reserveVB;
		FlexKit::ReserveConstantBufferFunction	reserveCB;
	};

	static double T = 0.0f;

	frameGraph.AddNode<RenderStrands>(
		RenderStrands{
			.reserveVB = reserveVB,
			.reserveCB = reserveCB 
		},
		[&](FrameGraphNodeBuilder& builder, RenderStrands& data)
		{
			builder.AddDataDependency(*update);
			data.sourceBuffer		= builder.NonPixelShaderResource(style.GetCurrentSourceBuffer());
			data.destinationTarget	= builder.UnorderedAccess(style.GetCurrentHairBuffer());
			data.strandBuffer		= builder.UnorderedAccess(style.strandbuffer);
			data.styleBuffer		= builder.NonPixelShaderResource(style.styleBuffer);
		},
		[=, this](RenderStrands& data, const ResourceHandler& resources, Context& ctx, iAllocator& threadLocalAllocator)
		{
			ctx.BeginEvent_DEBUG("Simulate");

			struct Constants
			{
				float dT;
				float T;
				uint32_t strandCount;
				uint32_t strandLength;
			} shaderConstants
			{
				.dT				= (float)dT,
				.T				= (float)T,
				.strandCount	= style.strandCount,
				.strandLength	= style.strandLength
			};

			T += dT;

			const auto x = (style.strandCount * style.strandLength) / 1024 + ((style.strandCount * style.strandLength) % 1024 == 0 ? 0 : 1);

			// Simulate A
			ctx.SetComputePipelineState(ApplyForcesPSO, threadLocalAllocator);

			ctx.SetComputeConstantValue(0, 4, &shaderConstants);
			ctx.SetComputeShaderResourceView(1, resources.GetResource(data.styleBuffer));

			ctx.SetComputeShaderResourceView(2, resources.GetResource(data.sourceBuffer));
			ctx.SetComputeUnorderedAccessView(3, resources.GetResource(data.destinationTarget));
			ctx.Dispatch({ x, 1, 1 });

			// Simulate B
			ctx.SetComputePipelineState(ApplyShapeConstraintsPSO, threadLocalAllocator);
			ctx.SetComputeShaderResourceView(2, resources.NonPixelShaderResource(data.destinationTarget, ctx));
			ctx.SetComputeUnorderedAccessView(3, resources.UAV(data.sourceBuffer, ctx));
			ctx.Dispatch({ x, 1, 1 });

			// Simulate C
			ctx.SetComputePipelineState(ApplyEdgeLengthConstraintPSO, threadLocalAllocator);
			ctx.SetComputeShaderResourceView(2, resources.NonPixelShaderResource(data.sourceBuffer, ctx));
			ctx.SetComputeUnorderedAccessView(3, resources.UAV(data.strandBuffer, ctx));
			ctx.Dispatch({ (style.strandCount) / 1024 + ((style.strandCount) % 1024 == 0 ? 0 : 1), 1, 1 });

			ctx.EndEvent_DEBUG();
		});
}


/************************************************************************************************/


void HairRenderingTest::DrawStrands(
	FlexKit::UpdateTask*					update,
	FlexKit::EngineCore&					core,
	FlexKit::UpdateDispatcher&				dispatcher,
	double									dT,
	FlexKit::FrameGraph&					frameGraph,
	FlexKit::ReserveVertexBufferFunction&	reserveVB,
	FlexKit::ReserveConstantBufferFunction&	reserveCB)
{
	
	struct RenderStrands
	{
		FlexKit::FrameResourceHandle			blendSamples;
		FlexKit::FrameResourceHandle			depthSamples;
		FlexKit::FrameResourceHandle			renderTarget;
		FlexKit::FrameResourceHandle			strandBuffer;
		FlexKit::FrameResourceHandle			depthBuffer;
		FlexKit::ReserveVertexBufferFunction	reserveVB;
		FlexKit::ReserveConstantBufferFunction	reserveCB;
	};

	struct MLABSample
	{
		uint16_t sampleBuffer[4 * 4];
	};

	struct MLABDepthSamples
	{
		float sampleBuffer[sizeof(float) * 4];
	};

	frameGraph.AddNode(
		RenderStrands{
			.reserveVB = reserveVB,
			.reserveCB = reserveCB 
		},
		[&](FrameGraphNodeBuilder& builder, RenderStrands& data)
		{
			builder.AddDataDependency(*update);

			data.blendSamples	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(1920 * 1080 * sizeof(MLABSample)), DASUAV, VirtualResourceScope::Frame);
			data.depthSamples	= builder.AcquireVirtualResource(GPUResourceDesc::UAVResource(1920 * 1080 * sizeof(MLABDepthSamples)), DASUAV, VirtualResourceScope::Frame);
			data.renderTarget	= builder.RenderTarget(renderWindow.GetBackBuffer());
			data.strandBuffer	= builder.NonPixelShaderResource(style.strandbuffer);
			data.depthBuffer	= builder.DepthTarget(depthBuffer);
		},
		[=, backBuffer = renderWindow.GetBackBuffer(), this](RenderStrands& data, const ResourceHandler& resources, Context& ctx, iAllocator& threadLocalAllocator)
		{
			ctx.BeginEvent_DEBUG("Draw Strands");

			auto blendSamples	= resources.GetResource(data.blendSamples);
			auto depthSamples	= resources.GetResource(data.depthSamples);
			auto strandBuffer	= resources.GetResource(data.strandBuffer);

			uint32_t f32INF	= std::bit_cast<uint32_t, float>(INFINITY);

			ctx.ClearUAVBuffer(blendSamples, { 0x00, 0x00, 0x00, 0x00 });
			ctx.ClearUAVBuffer(depthSamples, { f32INF, f32INF, f32INF, f32INF });
			ctx.AddUAVBarrier(blendSamples, -1, DeviceLayout_UnorderedAccess);
			ctx.AddUAVBarrier(depthSamples, -1, DeviceLayout_UnorderedAccess);

			ctx.SetScissorAndViewports({ backBuffer });
			ctx.SetGraphicsPipelineState(StrandRenderPSO, threadLocalAllocator);

			const auto CameraValues = GetCameraConstants(camera);

			struct Constants
			{
				float4x4	PV;
				uint32_t	debugOffset;
			} shaderConstants
			{
				.PV				= CameraValues.PV,
				.debugOffset	= debugOffset
			};

			ctx.SetGraphicsConstantValue(0, 17, &shaderConstants);
			ctx.SetGraphicsShaderResourceView(1, strandBuffer);
			ctx.SetGraphicsUnorderedAccessView(2, blendSamples);
			ctx.SetGraphicsUnorderedAccessView(3, depthSamples);

			ctx.SetInputPrimitive(INPUTPRIMITIVEPOINTLIST);
			ctx.Draw((style.strandLength - 1) * style.strandCount);

			ctx.AddUAVBarrier(blendSamples, -1, DeviceLayout_UnorderedAccess);

			ctx.SetGraphicsPipelineState(BlendStatePSO, threadLocalAllocator);
			ctx.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);
			ctx.SetRenderTargets({ backBuffer }, false);
			ctx.Draw(3);

			ctx.EndEvent_DEBUG();
		});
}


/************************************************************************************************/


void HairRenderingTest::DrawDebug(
	FlexKit::UpdateTask*					update,
	FlexKit::EngineCore&					core,
	FlexKit::UpdateDispatcher&				dispatcher,
	double									dT,
	FlexKit::FrameGraph&					frameGraph,
	FlexKit::ReserveVertexBufferFunction&	reserveVB,
	FlexKit::ReserveConstantBufferFunction&	reserveCB)
{
	struct RenderDebug
	{
		FlexKit::FrameResourceHandle			renderTarget;
		FlexKit::FrameResourceHandle			strandBuffer;
		FlexKit::ReserveVertexBufferFunction	reserveVB;
		FlexKit::ReserveConstantBufferFunction	reserveCB;
	};

	frameGraph.AddNode(
		RenderDebug{
			.reserveVB = reserveVB,
			.reserveCB = reserveCB 
		},
		[&](FrameGraphNodeBuilder& builder, RenderDebug& data)
		{
			builder.AddDataDependency(*update);

			data.renderTarget	= builder.RenderTarget(renderWindow.GetBackBuffer());
			data.strandBuffer	= builder.NonPixelShaderResource(style.strandbuffer);
		},
		[=, backBuffer = renderWindow.GetBackBuffer(), this](RenderDebug& data, const ResourceHandler& resources, Context& ctx, iAllocator& threadLocalAllocator)
		{
			ctx.SetScissorAndViewports({ backBuffer });
			ctx.SetRenderTargets({ backBuffer });
			ctx.SetGraphicsPipelineState(StrandRenderPSO, threadLocalAllocator);

			DescriptorHeap table{ctx, ctx.CurrentGraphicsRootSig()->GetDescHeap(0), threadLocalAllocator };
			table.SetUAVTexture(ctx, 0, resources.GetResource(data.renderTarget));

			ctx.SetGraphicsDescriptorTable(3, table);

			const auto CameraValues = GetCameraConstants(camera);

			struct Constants
			{
				float4x4 PV;
			} shaderConstants
			{
				.PV = CameraValues.PV,
			};

			ctx.SetGraphicsShaderResourceView(1, resources.GetResource(data.strandBuffer));
			ctx.SetGraphicsConstantValue(0, 16, &shaderConstants);
			ctx.SetInputPrimitive(INPUTPRIMITIVEPOINTLIST);
			ctx.Draw((style.strandLength - 1) * 20);
		});
}


/************************************************************************************************/


UpdateTask* HairRenderingTest::Draw(
	FlexKit::UpdateTask*		update,
	FlexKit::EngineCore&		core,
	FlexKit::UpdateDispatcher&	dispatcher,
	double						dT,
	FrameGraph&					frameGraph)
{
	frameGraph.AddMemoryPool(&gpuAllocator);
	frameGraph.AddOutput(renderWindow.GetBackBuffer());

	ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), { 0.0f, 0.0f, 0.0f, 0.0f });
	ClearVertexBuffer(frameGraph, vertexBuffer);
	ClearDepthBuffer(frameGraph, depthBuffer, 1.0f);

	auto reserveVB = CreateVertexBufferReserveObject(vertexBuffer, framework.GetRenderSystem(), framework.core.GetTempMemory());
	auto reserveCB = CreateConstantBufferReserveObject(constantBuffer, framework.GetRenderSystem(), framework.core.GetTempMemory());

	runOnceQueue.Process(dispatcher, frameGraph);

	if(!pause)
		Simulate(update, core, dispatcher, dT, frameGraph, reserveVB, reserveCB);

	DrawStrands(update, core, dispatcher, dT, frameGraph, reserveVB, reserveCB);

	if(debugVis)
		DrawDebug(update, core, dispatcher, dT, frameGraph, reserveVB, reserveCB);

	debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderWindow.GetBackBuffer());

	PresentBackBuffer(frameGraph, renderWindow);

	return nullptr;
}


/************************************************************************************************/


void HairRenderingTest::PostDrawUpdate(EngineCore& core, double dT)
{
	renderWindow.Present(1, 0);

	core.RenderSystem.ResetConstantBuffer(constantBuffer);
}


/************************************************************************************************/


bool HairRenderingTest::EventHandler(Event evt)
{
	if (evt.InputSource == Event::Keyboard && evt.Action == Event::Release && evt.mData1.mKC[0] == FlexKit::KC_SPACE)
	{
		pause = !pause;

		fmt::print("Simulation {}\n", (pause ? " Paused\n" : " Running\n"));
	}

	if (evt.InputSource == Event::Keyboard && evt.Action == Event::Release && evt.mData1.mKC[0] == FlexKit::KC_R)
	{
		if (auto import = ImportCSV(R"(assets\hair.csv)", framework.core.GetTempMemory()); import)
		{
			fmt::print("Reloading Style\n");

			auto& controlPoints = import.value();
			UploadHairStyle(style, controlPoints, framework.GetRenderSystem());
		}
	}

	if (evt.InputSource == Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_I && evt.Action == Event::Pressed)
		debugOffset = (debugOffset != 3) ? debugOffset = FlexKit::clamp(0u, debugOffset += 1u, 3u) : 0u;

	if (evt.InputSource == Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_ESC)
	{
		framework.quit = true;
		return true;
	}

	if (evt.InputSource == Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_S && evt.Action == Event::Release)
	{
		fmt::print("Reloading Shaders\n");

		framework.GetRenderSystem().QueuePSOLoad(StrandRenderPSO);
		framework.GetRenderSystem().QueuePSOLoad(BlendStatePSO);

		framework.GetRenderSystem().QueuePSOLoad(ApplyForcesPSO);
		framework.GetRenderSystem().QueuePSOLoad(ApplyShapeConstraintsPSO);
		framework.GetRenderSystem().QueuePSOLoad(ApplyEdgeLengthConstraintPSO);
	}

	return debugUI.HandleInput(evt);
}


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2014-2023 Robert May

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
