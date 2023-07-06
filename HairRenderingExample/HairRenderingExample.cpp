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


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateApplyForcesPSO()
{
	PipelineBuilder builder{ framework.core.GetTempMemory() };
	builder.AddRootSignature(strandRenderRootSignature);
	builder.AddComputeShader("ApplyForces", R"(assets\shaders\HairRendering\Simulation.hlsl)", { .enable16BitTypes = true });
	builder.SetDebugName("ApplyForces");

	return builder.Build(framework.GetRenderSystem());
}


/************************************************************************************************/


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateApplyShapeConstraintsPSO()
{
	PipelineBuilder builder{ framework.core.GetTempMemory() };
	builder.AddRootSignature(strandRenderRootSignature);
	builder.AddComputeShader("ApplyShapeConstraints", R"(assets\shaders\HairRendering\Simulation.hlsl)", { .enable16BitTypes = true });
	builder.SetDebugName("ApplyShapeConstraints");

	return builder.Build(framework.GetRenderSystem());
}


/************************************************************************************************/


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateApplyEdgeLengthConstraintPSO()
{
	PipelineBuilder builder{ framework.core.GetTempMemory() };
	builder.AddRootSignature(strandRenderRootSignature);
	builder.AddComputeShader("ApplyEdgeLengthContraints", R"(assets\shaders\HairRendering\Simulation.hlsl)", { .enable16BitTypes = true });
	builder.SetDebugName("ApplyEdgeLengthContraints");

	return builder.Build(framework.GetRenderSystem());
}


/************************************************************************************************/


LoadPipelineStateRes HairRenderingTest::CreateStrandRenderPSO()
{
	PipelineBuilder builder{ framework.core.GetTempMemory() };
	builder.AddRootSignature(strandRenderRootSignature);

	builder.AddVertexShader		("VMain", R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true });
	builder.AddGeometryShader	("GMain", R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true });
	builder.AddPixelShader		("PMain", R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true });

	builder.AddInputTopology(ETopology::EIT_POINT);
	builder.AddInputLayout({
		.inputs = { { "POSITION",	0, DeviceFormat::R32G32B32A32_FLOAT, 0, 0,	EInputClassification::PerVertex, 0 }, },
		.count	= 1
		});

	builder.AddDepthStencilFormat(DeviceFormat::D32_FLOAT);

	builder.AddDepthStencilState({
			.depthEnable = true,
			.depthFunc = EComparison::LESS,
		});

	builder.AddRenderTargetState({
			.targetCount = 1,
			.targetFormats = { DeviceFormat::R16G16B16A16_FLOAT },
		});

	builder.SetDebugName("DrawStrands");

	return builder.Build(framework.core.RenderSystem);
}


/************************************************************************************************/


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateDebugRenderPSO()
{
	PipelineBuilder builder{ framework.core.GetTempMemory() };
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
	debugUI						{ IN_framework.GetRenderSystem(), IN_framework.core.GetBlockMemory() }
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

	FlexKit::RootSignatureBuilder builder{ framework.core.GetBlockMemory() };

	builder.AllowIA = true;
	builder.SetParameterAsUINT(0, 16, 0, 0);
	builder.SetParameterAsSRV(1, 0);
	builder.SetParameterAsSRV(2, 1);
	builder.SetParameterAsUAV(3, 0, 0);

	strandRenderRootSignature = builder.Build(framework.GetRenderSystem(), framework.core.GetTempMemory());
	FK_ASSERT(strandRenderRootSignature != nullptr, "Failed to create root signature!");

	framework.GetRenderSystem().RegisterPSOLoader(
		StrandRenderPSO,
		[&](auto) { return CreateStrandRenderPSO(); });

	framework.GetRenderSystem().RegisterPSOLoader(
		ApplyForcesPSO,
		[&](auto) { return CreateApplyForcesPSO(); });

	framework.GetRenderSystem().RegisterPSOLoader(
		ApplyShapeConstraintsPSO, 
		[&](auto) { return CreateApplyShapeConstraintsPSO(); });

	framework.GetRenderSystem().RegisterPSOLoader(
		ApplyEdgeLengthConstraintPSO,
		[&](auto) { return CreateApplyEdgeLengthConstraintPSO(); });

	framework.GetRenderSystem().QueuePSOLoad(StrandRenderPSO);
	framework.GetRenderSystem().QueuePSOLoad(ApplyForcesPSO);
	framework.GetRenderSystem().QueuePSOLoad(ApplyShapeConstraintsPSO);
	framework.GetRenderSystem().QueuePSOLoad(ApplyEdgeLengthConstraintPSO);

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

	auto& rootSignature = strandRenderRootSignature;
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
		[=, &rootSignature, this](RenderStrands& data, const ResourceHandler& resources, Context& ctx, iAllocator& threadLocalAllocator)
		{
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

			ctx.SetComputeRootSignature(rootSignature);

			const auto x = (style.strandCount * style.strandLength) / 1024 + ((style.strandCount * style.strandLength) % 1024 == 0 ? 0 : 1);

			// Simulate A
			ctx.SetComputeConstantValue(0, 4, &shaderConstants);
			ctx.SetComputeShaderResourceView(1, resources.GetResource(data.styleBuffer));

			ctx.SetPipelineState(resources.GetPipelineState(ApplyForcesPSO));
			ctx.SetComputeShaderResourceView(2, resources.GetResource(data.sourceBuffer));
			ctx.SetComputeUnorderedAccessView(3, resources.GetResource(data.destinationTarget));
			ctx.Dispatch({ x, 1, 1 });

			// Simulate B
			ctx.SetPipelineState(resources.GetPipelineState(ApplyShapeConstraintsPSO));
			ctx.SetComputeShaderResourceView(2, resources.NonPixelShaderResource(data.destinationTarget, ctx));
			ctx.SetComputeUnorderedAccessView(3, resources.UAV(data.sourceBuffer, ctx));
			ctx.Dispatch({ x, 1, 1 });

			// Simulate C
			ctx.SetPipelineState(resources.GetPipelineState(ApplyEdgeLengthConstraintPSO));
			ctx.SetComputeShaderResourceView(2, resources.NonPixelShaderResource(data.sourceBuffer, ctx));
			ctx.SetComputeUnorderedAccessView(3, resources.UAV(data.strandBuffer, ctx));
			ctx.Dispatch({ (style.strandCount) / 1024 + ((style.strandCount) % 1024 == 0 ? 0 : 1), 1, 1 });
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
		FlexKit::FrameResourceHandle			renderTarget;
		FlexKit::FrameResourceHandle			strandBuffer;
		FlexKit::FrameResourceHandle			depthBuffer;
		FlexKit::ReserveVertexBufferFunction	reserveVB;
		FlexKit::ReserveConstantBufferFunction	reserveCB;
	};

	auto& rootSignature = strandRenderRootSignature;
	frameGraph.AddNode(
		RenderStrands{
			.reserveVB = reserveVB,
			.reserveCB = reserveCB 
		},
		[&](FrameGraphNodeBuilder& builder, RenderStrands& data)
		{
			builder.AddDataDependency(*update);

			data.renderTarget	= builder.RenderTarget(renderWindow.GetBackBuffer());
			data.strandBuffer	= builder.NonPixelShaderResource(style.strandbuffer);
			data.depthBuffer	= builder.DepthTarget(depthBuffer);
		},
		[=, &rootSignature, backBuffer = renderWindow.GetBackBuffer(), this](RenderStrands& data, const ResourceHandler& resources, Context& ctx, iAllocator& threadLocalAllocator)
		{
			ctx.SetScissorAndViewports({ backBuffer });
			ctx.SetRenderTargets({ backBuffer }, true, resources.GetResource(data.depthBuffer));
			ctx.SetRootSignature(rootSignature);
			ctx.SetPipelineState(resources.GetPipelineState(StrandRenderPSO));

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
			ctx.Draw((style.strandLength - 1) * style.strandCount);
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

	auto& rootSignature = strandRenderRootSignature;
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
		[=, &rootSignature, backBuffer = renderWindow.GetBackBuffer(), this](RenderDebug& data, const ResourceHandler& resources, Context& ctx, iAllocator& threadLocalAllocator)
		{
			ctx.SetScissorAndViewports({ backBuffer });
			ctx.SetRenderTargets({ backBuffer }, false);
			ctx.SetRootSignature(rootSignature);
			ctx.SetPipelineState(resources.GetPipelineState(StrandRenderPSO));

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
	frameGraph.AddOutput(renderWindow.GetBackBuffer());

	ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), { 0.0f, 0.0f, 0.0f, 0.0f });
	ClearVertexBuffer(frameGraph, vertexBuffer);
	ClearDepthBuffer(frameGraph, depthBuffer, 1.0f);

	auto reserveVB = FlexKit::CreateVertexBufferReserveObject(vertexBuffer, framework.GetRenderSystem(), framework.core.GetTempMemory());
	auto reserveCB = FlexKit::CreateConstantBufferReserveObject(constantBuffer, framework.GetRenderSystem(), framework.core.GetTempMemory());

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
	renderWindow.Present(0);

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

	if (evt.InputSource == Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_ESC)
	{
		framework.quit = true;
		return true;
	}

	if (evt.InputSource == Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_S && evt.Action == Event::Release)
	{
		fmt::print("Reloading Shaders\n");

		framework.GetRenderSystem().QueuePSOLoad(StrandRenderPSO);
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
