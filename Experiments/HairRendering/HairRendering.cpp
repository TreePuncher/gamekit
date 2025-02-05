
#include "HairRendering.hpp"
#include <Win32Graphics.hpp>
#include <FrameGraph.hpp>
#include <fmt/printf.h>
#include <fmt/format.h>
#include <cstdio>
#include <fp16.h>
#include <scn/scan.h>
#include <ranges>
#include <imgui.h>
#include <stacktrace>
#include <filesystem>
#include <stb_image.h>
#include <RmlUi/Core.h>


/************************************************************************************************/


using namespace FlexKit;

constexpr FlexKit::PSOHandle MBOITRender1				= FlexKit::PSOHandle{ GetCRCGUID(MBOITRender1)	};
constexpr FlexKit::PSOHandle MBOITRender2				= FlexKit::PSOHandle{ GetCRCGUID(MBOITRender2)	};
constexpr FlexKit::PSOHandle MBOITBlend					= FlexKit::PSOHandle{ GetCRCGUID(MBOITBlend)	};

constexpr FlexKit::PSOHandle ApplyForces				= FlexKit::PSOHandle{ GetCRCGUID(ApplyForces) };
constexpr FlexKit::PSOHandle ApplyShapeConstraints		= FlexKit::PSOHandle{ GetCRCGUID(ApplyShapeConstraints) };
constexpr FlexKit::PSOHandle ApplyEdgeLengthConstraint	= FlexKit::PSOHandle{ GetCRCGUID(ApplyEdgeLengthConstraint) };

constexpr FlexKit::PSOHandle StrandRenderPSO			= FlexKit::PSOHandle{ GetCRCGUID(StrandRenderPSO) };


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
	renderSystem.UpdateResourceByUploadQueue(renderSystem.GetDeviceResource(style.hairBuffers[0]),	copyCtx, stylePoints.controlPoints.data(), stylePoints.controlPoints.ByteSize(), 1, DASCommon);
	renderSystem.UpdateResourceByUploadQueue(renderSystem.GetDeviceResource(style.styleBuffer),		copyCtx, stylePoints.controlPoints.data(), stylePoints.controlPoints.ByteSize(), 1, DASCommon);

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
		auto temp = line.size();
		if (line.size())
		{
			lineCount++;

			resetColumn = true;
			uint32_t columnCount = 0;

			auto trimWhiteSpace = [&](const auto a)
				{
					if (a.begin() < buffer.end() && a.end() < buffer.end() && *a.end() != '\0')
					{
						auto b = a.begin();
						auto e = a.end();
						bool endline = false;
						bool fileEnd = false;

						while (b < a.end() && (std::isspace(*b) || *b == ','))
						{
							if (*b == '\n') endline = true;
							if (*b == '\0') fileEnd = true;
							b++;
						}
						while (e > b && std::isspace(*e)) e--;

						auto temp = std::string_view{ b, e };
						if (temp.size() > 1024)
							DebugBreak();

						return temp;
					}
					else return std::string_view{};
				};

			for (const auto stringView : std::ranges::split_view(line, commaSeperator) | std::views::transform(trimWhiteSpace))
			{
				std::string line{ stringView };

				if (auto res = scn::scan<float, float, float>(stringView, "({} {} {})"); res.has_value())
				{

					const auto d	= resetColumn ? FlexKit::float3(0, 0, 0) : (float3{ floats.back().position[0], floats.back().position[1], floats.back().position[2] });
					const float l	= resetColumn ? 0.0f : (d - float3{ std::get<0>(res->values()), std::get<1>(res->values()), std::get<2>(res->values()) }).magnitude();

					ControlPoint controlPoint{
						.position	= { std::get<0>(res->values()), std::get<1>(res->values()), std::get<2>(res->values()) },
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


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateStrandRenderOpaquePSO(FlexKit::iAllocator& tempMemory)
{
	PipelineBuilder builder{ tempMemory };

	builder.AddVertexShader		("VMain",	R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });
	builder.AddGeometryShader	("GMain",	R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });
	builder.AddPixelShader		("PMain",	R"(assets\shaders\HairRendering\StrandRendering.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });

	builder.AddInputTopology(ETopology::EIT_POINT);
	builder.AddInputLayout({
		.inputs = { { "POSITION",	0, DeviceFormat::R32G32B32A32_FLOAT, 0, 0,	EInputClassification::PerVertex, 0 }, },
		.count	= 1
		});

	builder.AddRenderTargetState({
			.targetCount	= 1,
			.targetFormats	= {
				DeviceFormat::R16G16B16A16_FLOAT
			}
		});

	builder.AddDepthStencilState({
			.depthEnable	= true,
			.depthWriteMask	= EDepthWriteMask::All,
			.depthFunc		= EComparison::LESS,
		});

	builder.SetDebugName("DrawOpaque");

	return builder.Build(framework.core.RenderSystem);
}


LoadPipelineStateRes HairRenderingTest::CreateStrandRender1PSO(iAllocator& tempMemory)
{
	PipelineBuilder builder{ tempMemory };

	builder.AddVertexShader		("VMain",	R"(assets\shaders\HairRendering\StrandRenderingMBOITPass1.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });
	builder.AddGeometryShader	("GMain",	R"(assets\shaders\HairRendering\StrandRenderingMBOITPass1.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });
	builder.AddPixelShader		("PS_Draw",	R"(assets\shaders\HairRendering\StrandRenderingMBOITPass1.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });

	builder.AddInputTopology(ETopology::EIT_POINT);
	builder.AddInputLayout({
		.inputs = { { "POSITION",	0, DeviceFormat::R32G32B32A32_FLOAT, 0, 0,	EInputClassification::PerVertex, 0 }, },
		.count	= 1
		});

	builder.SetDebugName("DrawMBOIT1");

	return builder.Build(framework.core.RenderSystem);
}


/************************************************************************************************/


LoadPipelineStateRes HairRenderingTest::CreateStrandRender2PSO(iAllocator& tempMemory)
{
	PipelineBuilder builder{ tempMemory };

	builder.AddVertexShader		("VMain",	R"(assets\shaders\HairRendering\StrandRenderingMBOITPass2.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });
	builder.AddGeometryShader	("GMain",	R"(assets\shaders\HairRendering\StrandRenderingMBOITPass2.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });
	builder.AddPixelShader		("PS_Draw",	R"(assets\shaders\HairRendering\StrandRenderingMBOITPass2.hlsl)", { .enable16BitTypes = true, .hlsl2021 = true  });

	builder.AddInputTopology(ETopology::EIT_POINT);
	builder.AddInputLayout({
		.inputs = { { "POSITION",	0, DeviceFormat::R32G32B32A32_FLOAT, 0, 0,	EInputClassification::PerVertex, 0 }, },
		.count	= 1
		});

	builder.AddBlendState({
			.renderTarget = {{
				.blendEnable	= true,
				.srcBlend		= EBlend::ONE,
				.blendOp		= EBlendOP::ADD,
				.srcBlendAlpha	= EBlend::ONE,
			}}
		}
	);

	builder.AddRenderTargetState({
		.targetCount	= 1,
		.targetFormats	= {
			DeviceFormat::R16G16B16A16_FLOAT
		}});

	builder.SetDebugName("DrawMBOIT2");

	return builder.Build(framework.core.RenderSystem);
}


/************************************************************************************************/


FlexKit::LoadPipelineStateRes HairRenderingTest::CreateBlendState(iAllocator& tempMemory)
{
	PipelineBuilder builder{ tempMemory };

	builder.AddVertexShader		("VS_FullScreen",	R"(assets\shaders\HairRendering\StrandBlendMBOIT.hlsl)", { .enable16BitTypes = true });
	builder.AddPixelShader		("PS_Blend",		R"(assets\shaders\HairRendering\StrandBlendMBOIT.hlsl)", { .enable16BitTypes = true });

	builder.AddInputTopology(ETopology::EIT_TRIANGLE);

	builder.AddRasterizerState({
			.CullMode	= ECullMode::NONE
		});

	builder.AddRenderTargetState({
			.targetCount	= 1,
			.targetFormats	= { DeviceFormat::R16G16B16A16_FLOAT },
		});

	builder.SetDebugName("DrawBlend_MBOIT");

	return builder.Build(framework.core.RenderSystem);
}


/************************************************************************************************/


HairRenderingTest::HairRenderingTest(GameFramework& IN_framework, bool enableWorkGraph) :
	FrameworkState				{ IN_framework },
	vertexBuffer				{ IN_framework.GetRenderSystem().CreateVertexBuffer(16 * MEGABYTE, false) },
	constantBuffer				{ IN_framework.GetRenderSystem().CreateConstantBuffer(16 * MEGABYTE, false) },
	cameras						{ IN_framework.core.GetBlockMemory() },
	runOnceQueue				{ IN_framework.core.GetBlockMemory() },
	depthBuffer					{ IN_framework.GetRenderSystem().CreateDepthBuffer({ 1920, 1080 }, true) },
	UAVPool						{ IN_framework.GetRenderSystem(), 1024 * MEGABYTE, 64 * KILOBYTE, DeviceHeapFlags::UAVTextures | DeviceHeapFlags::UAVBuffer, IN_framework.core.GetBlockMemory() },
	RTPool						{ IN_framework.GetRenderSystem(), 1024 * MEGABYTE, 64 * KILOBYTE, DeviceHeapFlags::RenderTarget, IN_framework.core.GetBlockMemory() },
	ui							{ IN_framework.GetRenderSystem(), IN_framework.core.GetBlockMemory() }
{
	if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
		renderWindow = res;
	else
		throw std::runtime_error{ "Unable to create render window!" };

	EventNotifier<>::Subscriber sub;
	sub.Notify = &EventsWrapper;
	sub._ptr = &framework;

	renderWindow->Handler.Subscribe(sub);
	renderWindow->SetWindowTitle("Hair Rendering - WIP");

	framework.GetRenderSystem().RegisterPSOLoader(ApplyForces,					[this](auto, auto& allocator) { return CreateApplyForcesPSO(allocator); });
	framework.GetRenderSystem().RegisterPSOLoader(ApplyShapeConstraints,		[this](auto, auto& allocator) { return CreateApplyShapeConstraintsPSO(allocator); });
	framework.GetRenderSystem().RegisterPSOLoader(ApplyEdgeLengthConstraint,	[this](auto, auto& allocator) { return CreateApplyEdgeLengthConstraintPSO(allocator); });

	framework.GetRenderSystem().RegisterPSOLoader(StrandRenderPSO, [this](auto, auto& allocator) { return CreateStrandRenderOpaquePSO(allocator); });

	framework.GetRenderSystem().RegisterPSOLoader(MBOITRender1, [this](auto, auto& allocator) { return CreateStrandRender1PSO(allocator); });
	framework.GetRenderSystem().RegisterPSOLoader(MBOITRender2,	[this](auto, auto& allocator) { return CreateStrandRender2PSO(allocator); });
	framework.GetRenderSystem().RegisterPSOLoader(MBOITBlend,	[this](auto, auto& allocator) { return CreateBlendState(allocator); });

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

	if(enableWorkGraph)
		CreateWorkGraphObjects();

	auto uiContext = ui.GetMainContext();
	auto document = uiContext->LoadDocument(R"(assets\hello.rml)");
	document->Show();
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


void HairRenderingTest::CreateWorkGraphObjects()
{
	if (GetRenderSystem()->features.workGraph != FlexKit::AvailableFeatures::WorkGraphs_AVAILABLE)
		return;

	FlexKit::RootSignatureBuilder	signatureBuilder{ framework.core.GetBlockMemory() };
	FlexKit::RootSignature*			rootSig = signatureBuilder.Build(GetRenderSystem(), framework.core.GetTempMemory());


	D3D12_GLOBAL_ROOT_SIGNATURE signature =
	{
		.pGlobalRootSignature = *rootSig
	};


	auto shaderLibrary = GetRenderSystem()->LoadShader(nullptr, "lib_6_8", R"(assets\shaders\HairRendering\workgraphs\TestWorkGroup.hlsl)");

	D3D12_DXIL_LIBRARY_DESC dxil_desc[] = {
		{
			.DXILLibrary = {
				.pShaderBytecode	= shaderLibrary.buffer,
				.BytecodeLength		= shaderLibrary.bufferSize,
			},
			.NumExports		= 0,
			.pExports		= nullptr,
		}
	};

	D3D12_WORK_GRAPH_DESC workGroupDesk[] = {
		{
			.ProgramName				= L"Main",
			.Flags						= D3D12_WORK_GRAPH_FLAGS::D3D12_WORK_GRAPH_FLAG_INCLUDE_ALL_AVAILABLE_NODES,
			.NumEntrypoints				= 0,
			.pEntrypoints				= 0,
			.NumExplicitlyDefinedNodes	= 0,
			.pExplicitlyDefinedNodes	= nullptr,
		}
	};
	
	D3D12_STATE_SUBOBJECT subObjects[] = {
		{
			.Type	= D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
			.pDesc	= &signature,
		},
		{
			.Type	= D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
			.pDesc	= dxil_desc,
		},
		{
			.Type	= D3D12_STATE_SUBOBJECT_TYPE_WORK_GRAPH,
			.pDesc	= workGroupDesk,
		},
	};
	
	D3D12_STATE_OBJECT_DESC descs = {
		.Type			= D3D12_STATE_OBJECT_TYPE_EXECUTABLE,
		.NumSubobjects	= sizeof(subObjects) / sizeof(subObjects[0]),
		.pSubobjects	= subObjects,
	};

	ID3D12StateObject* stateObject = nullptr;
	if (FAILED(GetRenderSystem()->pDevice14->CreateStateObject(&descs, IID_PPV_ARGS(&stateObject))))
		FK_LOG_ERROR("Failed to create State Object");
	else
	{
		ID3D12StateObjectProperties1*	properties			= nullptr;
		ID3D12WorkGraphProperties*		workGraphProperties = nullptr;
		auto HR1 = stateObject->QueryInterface<ID3D12StateObjectProperties1>(&properties);
		auto HR2 = stateObject->QueryInterface<ID3D12WorkGraphProperties>(&workGraphProperties);

		if (FAILED(HR1) || FAILED(HR2))
		{
			if(stateObject)
				stateObject->Release();
			if(properties)
				properties->Release();
			if (workGraphProperties)
				workGraphProperties->Release();

			rootSig->Release();

			mode = Mode::Default;
		}
		else
		{
			workGraphObjects.globalRootSignature	= rootSig;
			workGraphObjects.stateObject			= stateObject;
			workGraphObjects.properties				= properties;
			workGraphObjects.workGraphProperties	= workGraphProperties;

			workGraphObjects.main	= properties->GetProgramIdentifier(L"Main");
			workGraphObjects.mainID = workGraphProperties->GetWorkGraphIndex(L"Main");
			mode					= Mode::WorkGraph;
		}
	}
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
					data.hair1	= builder.UnorderedAccess(style.hairBuffers[0]);
					data.hair2	= builder.UnorderedAccess(style.hairBuffers[1]);
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
	renderWindow->UpdateCapturedMouseInput(dT);


	auto cameraNode = cameras.GetCamera(camera).Node;
	Yaw(cameraRig, pi / 8.0f * dT);
	SetCameraAspectRatio(camera, renderWindow->GetAspectRatio());
	cameras.MarkDirty(camera);
	
	auto& transformUpdate	= QueueTransformUpdateTask(dispatcher);
	auto& cameraUpdate		= cameras.QueueCameraUpdate(dispatcher);

	cameraUpdate.AddInput(transformUpdate);

	if(framework.debugUI)
		framework.debugUI->Update(*renderWindow, core, dispatcher, dT);

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

	if(auto uiUpdate = ui.Update(core, dispatcher, dT); uiUpdate)
		cameraUpdate.AddInput(*uiUpdate);

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

	auto& drawStrands = frameGraph.AddNode<RenderStrands>(
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

			builder.Requires(ApplyForces);
			builder.Requires(ApplyShapeConstraints);
			builder.Requires(ApplyEdgeLengthConstraint);
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
			ctx.SetComputePipelineState(ApplyForces, threadLocalAllocator);

			ctx.SetComputeConstantValue(0, 4, &shaderConstants);
			ctx.SetComputeShaderResourceView(1, resources.GetResource(data.styleBuffer));

			ctx.SetComputeShaderResourceView(2, resources.GetResource(data.sourceBuffer));
			ctx.SetComputeUnorderedAccessView(3, resources.GetResource(data.destinationTarget));
			ctx.Dispatch({ x, 1, 1 });

			// Simulate B
			ctx.SetComputePipelineState(ApplyShapeConstraints, threadLocalAllocator);
			ctx.SetComputeShaderResourceView(2, resources.NonPixelShaderResource(data.destinationTarget, ctx));
			ctx.SetComputeUnorderedAccessView(3, resources.UAV(data.sourceBuffer, ctx));
			ctx.Dispatch({ x, 1, 1 });

			// Simulate C
			ctx.SetComputePipelineState(ApplyEdgeLengthConstraint, threadLocalAllocator);
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
		FlexKit::FrameResourceHandle			renderTarget;
		FlexKit::FrameResourceHandle			strandBuffer;
		FlexKit::FrameResourceHandle			depthBuffer;
		FlexKit::ReserveVertexBufferFunction	reserveVB;
		FlexKit::ReserveConstantBufferFunction	reserveCB;
	};

	frameGraph.AddNode(
		RenderStrands{
			.reserveVB = reserveVB,
			.reserveCB = reserveCB 
		},
		[&](FrameGraphNodeBuilder& builder, RenderStrands& data)
		{
			builder.AddDataDependency(*update);
			builder.Requires(StrandRenderPSO);

			data.renderTarget	= builder.RenderTarget(renderWindow->GetBackBuffer());
			data.strandBuffer	= builder.NonPixelShaderResource(style.strandbuffer);
			data.depthBuffer	= builder.DepthTarget(depthBuffer);
		},
		[=, backBuffer = renderWindow->GetBackBuffer(), this](RenderStrands& data, const ResourceHandler& resources, Context& ctx, iAllocator& threadLocalAllocator)
		{
			ctx.SetScissorAndViewports({ backBuffer });
			ctx.SetRenderTargets({ backBuffer }, true, resources.GetResource(data.depthBuffer));
			ctx.SetGraphicsPipelineState(StrandRenderPSO, threadLocalAllocator);
			ctx.SetPipelineState(resources.GetPipelineState(StrandRenderPSO, threadLocalAllocator));

			const auto CameraValues = GetCameraConstants(camera);

			struct Constants
			{
				float4x4_GPU PV;
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



void HairRenderingTest::DrawStrandsOIT(
	FlexKit::UpdateTask*					update,
	FlexKit::EngineCore&					core,
	FlexKit::UpdateDispatcher&				dispatcher,
	const double							dT,
	FlexKit::FrameGraph&					frameGraph,
	FlexKit::ReserveVertexBufferFunction&	reserveVB,
	FlexKit::ReserveConstantBufferFunction&	reserveCB)
{
	
	struct RenderStrands
	{
		FlexKit::FrameResourceHandle			b0Buffer;
		FlexKit::FrameResourceHandle			momentBuffer;
		FlexKit::FrameResourceHandle			accumBuffer;

		FlexKit::FrameResourceHandle			renderTarget;
		FlexKit::FrameResourceHandle			strandBuffer;

		FlexKit::ReserveVertexBufferFunction	reserveVB;
		FlexKit::ReserveConstantBufferFunction	reserveCB;
	};

	struct MBOITSample
	{
		float sampleBuffer[4];
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
			builder.Requires(MBOITRender1);
			builder.Requires(MBOITRender2);
			builder.Requires(MBOITBlend);

			data.b0Buffer		= builder.AcquireVirtualResource(GPUResourceDesc::UAVTexture({ 1920, 1080 }, DeviceFormat::R32_FLOAT),				DASUAV, VirtualResourceScope::Temporary);
			data.momentBuffer	= builder.AcquireVirtualResource(GPUResourceDesc::UAVTexture({ 1920, 1080 }, DeviceFormat::R32G32B32A32_FLOAT),		DASUAV, VirtualResourceScope::Temporary);
			data.accumBuffer	= builder.AcquireVirtualResource(GPUResourceDesc::RenderTarget({ 1920, 1080 }, DeviceFormat::R16G16B16A16_FLOAT),	DASRenderTarget, VirtualResourceScope::Temporary);

			data.renderTarget	= builder.RenderTarget(renderWindow->GetBackBuffer());
			data.strandBuffer	= builder.NonPixelShaderResource(style.strandbuffer);
		},
		[=, backBuffer = renderWindow->GetBackBuffer(), this](RenderStrands& data, const ResourceHandler& resources, Context& ctx, iAllocator& threadLocalAllocator)
		{
			ctx.BeginEvent_DEBUG("Draw Strands");

			auto b0Buffer		= resources.GetResource(data.b0Buffer);
			auto momentBuffer	= resources.GetResource(data.momentBuffer);
			auto accumBuffer	= resources.GetResource(data.accumBuffer);
			auto strandBuffer	= resources.GetResource(data.strandBuffer);


			// Clear
			ctx.ClearUAVTextureFloat(b0Buffer, { 0.0f, 0.0f, 0.0f, 0.0f });
			ctx.ClearUAVTextureFloat(momentBuffer, { 0.0f, 0.0f, 0.0f, 0.0f });
			ctx.ClearRenderTarget(accumBuffer, { 0.0f, 0.0f, 0.0f, 0.0f });

			ctx.AddUAVBarrier(b0Buffer,		-1, DeviceLayout_UnorderedAccess,	Sync_Compute,	Sync_PixelShader);
			ctx.AddTextureBarrier(accumBuffer,	DASRenderTarget, DASRenderTarget, DeviceLayout_RenderTarget, DeviceLayout_RenderTarget,	Sync_RenderTarget,	Sync_RenderTarget);

			// Pass 1
			ctx.SetGraphicsPipelineState(MBOITRender1, threadLocalAllocator);
			
			const auto cameraValues = GetCameraConstants(camera);
			
			struct
			{
				float4x4_GPU	PV;
				float4			wrapping_zone_parameters	= { 0.0f, 0.0f, 0.0f, 0.0f };
				float			overestimation				= 0.0f;
				float			moment_bias					= 0.0f;
			} shaderConstants0
			{
				.PV	= cameraValues.PV,
			};

			ctx.SetInputPrimitive(INPUTPRIMITIVEPOINTLIST);
			ctx.SetGraphicsConstantValue(0, 17, &shaderConstants0);
			ctx.SetGraphicsShaderResourceView(1, strandBuffer);
			ctx.SetGraphicsDescriptorTable(2,
				DescriptorHeap{ ctx, ctx.CurrentGraphicsRootSig()->GetDescHeap(0), threadLocalAllocator }
					.SetUAVTexture(ctx, 0, b0Buffer)
					.SetUAVTexture(ctx, 1, momentBuffer));

			ctx.SetScissorAndViewports({ accumBuffer });
			ctx.Draw((style.strandLength - 1) * style.strandCount);

			// Pass 2
			ctx.SetGraphicsPipelineState(MBOITRender2, threadLocalAllocator);

			ctx.SetGraphicsConstantValue(0, 22, &shaderConstants0);
			ctx.SetGraphicsShaderResourceView(1, strandBuffer);

			ctx.SetGraphicsDescriptorTable(2,
				DescriptorHeap{ ctx, ctx.CurrentGraphicsRootSig()->GetDescHeap(0), threadLocalAllocator }
					.SetSRV(ctx, 0, resources.PixelShaderResource(data.b0Buffer,		ctx, Sync_PixelShader,	Sync_PixelShader))
					.SetSRV(ctx, 1, resources.PixelShaderResource(data.momentBuffer,	ctx, Sync_PixelShader,	Sync_PixelShader))
					.NullFill(ctx));

			ctx.SetScissorAndViewports({ accumBuffer });
			ctx.SetRenderTargets({ accumBuffer });
			ctx.Draw((style.strandLength - 1) * style.strandCount);

			// Blend
			ctx.SetGraphicsPipelineState(MBOITBlend, threadLocalAllocator);

			ctx.SetGraphicsDescriptorTable(1,
				DescriptorHeap{ ctx, ctx.CurrentGraphicsRootSig()->GetDescHeap(0), threadLocalAllocator }
					.SetSRV(ctx, 0, b0Buffer)
					.SetSRV(ctx, 1, momentBuffer)
					.SetSRV(ctx, 2, resources.PixelShaderResource(data.accumBuffer,		ctx, Sync_RenderTarget,	Sync_PixelShader)));

			ctx.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);
			ctx.SetRenderTargets({ backBuffer });
			ctx.Draw(3);

			ctx.EndEvent_DEBUG();
		});
}


/************************************************************************************************/


void HairRenderingTest::WorkGraph(
	FlexKit::UpdateTask*					update,
	FlexKit::EngineCore&					core,
	FlexKit::UpdateDispatcher&				dispatcher,
	const double							dT,
	FlexKit::FrameGraph&					frameGraph,
	FlexKit::ReserveVertexBufferFunction&	reserveVB,
	FlexKit::ReserveConstantBufferFunction&	reserveCB)
{
	struct DataStruct
	{
		FlexKit::ReserveConstantBufferFunction	reserveCB;

		FlexKit::FrameResourceHandle workGroupStorage;
		FlexKit::FrameResourceHandle renderTarget;
	};

	frameGraph.AddNode(
		DataStruct{
			.reserveCB = reserveCB,
		},
		[&](FrameGraphNodeBuilder& builder, DataStruct& data)
		{
			D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS memoryRequirements{};
			workGraphObjects.workGraphProperties->GetWorkGraphMemoryRequirements(workGraphObjects.mainID, &memoryRequirements);
			
			data.workGroupStorage	= builder.AcquireVirtualResource(FlexKit::GPUResourceDesc::UAVResource(memoryRequirements.MaxSizeInBytes), FlexKit::DeviceAccessState::DASUAV);
			data.renderTarget		= builder.RenderTarget(renderWindow->GetBackBuffer());
		},
		[=, this](DataStruct& data, const ResourceHandler& resources, Context& ctx, iAllocator& threadLocalAllocator)
		{
			auto CBBuffer = data.reserveCB(1024);
			
			struct TestData
			{
				uint4 XYXW;
			} test;
			
			ConstantBufferDataSet constants{test, CBBuffer};
			
			auto range = resources.GetDevicePointerRange(constants);

			D3D12_SET_PROGRAM_DESC programDesc;
			programDesc.Type									= D3D12_PROGRAM_TYPE_WORK_GRAPH;
			programDesc.WorkGraph.BackingMemory					= resources.GetDevicePointerRange(data.workGroupStorage);
			programDesc.WorkGraph.Flags							= D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
			programDesc.WorkGraph.ProgramIdentifier				= workGraphObjects.main;
			programDesc.WorkGraph.NodeLocalRootArgumentsTable	= { 0 };

			//programDesc.WorkGraph.NodeLocalRootArgumentsTable	= { range.range.StartAddress, sizeof(TestData), sizeof(TestData) };

			D3D12_DISPATCH_GRAPH_DESC graph;
			graph.Mode							= D3D12_DISPATCH_MODE::D3D12_DISPATCH_MODE_NODE_CPU_INPUT;
			graph.NodeCPUInput					= { };
			graph.NodeCPUInput.EntrypointIndex	= workGraphObjects.mainID;
			graph.NodeCPUInput.NumRecords		= 1;

			ctx.FlushBarriers();
			ctx.SetComputeRootSignature(workGraphObjects.globalRootSignature);
			ctx.DeviceContext->SetProgram(&programDesc);
			ctx.DeviceContext->DispatchGraph(&graph);

			programDesc.WorkGraph.Flags = D3D12_SET_WORK_GRAPH_FLAG_NONE;
			ctx.DeviceContext->DispatchGraph(&graph);
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
	frameGraph.AddMemoryPool(&UAVPool);
	frameGraph.AddMemoryPool(&RTPool);
	frameGraph.AddOutput(renderWindow->GetBackBuffer());

	ClearBackBuffer(frameGraph, renderWindow->GetBackBuffer(), { 0.0f, 0.0f, 0.0f, 0.0f });
	ClearVertexBuffer(frameGraph, vertexBuffer);
	ClearDepthBuffer(frameGraph, depthBuffer, 1.0f);

	auto reserveVB = CreateVertexBufferReserveObject(vertexBuffer, framework.GetRenderSystem(), framework.core.GetTempMemory());
	auto reserveCB = CreateConstantBufferReserveObject(constantBuffer, framework.GetRenderSystem(), framework.core.GetTempMemory());

	runOnceQueue.Process(dispatcher, frameGraph);

	switch(mode)
	{
	case Mode::Default:
	{
		if (!pause)
			Simulate(update, core, dispatcher, dT, frameGraph, reserveVB, reserveCB);

		DrawStrands(update, core, dispatcher, dT, frameGraph, reserveVB, reserveCB);
	}	break;
	case Mode::WorkGraph:
	{
		WorkGraph(update, core, dispatcher, dT, frameGraph, reserveVB, reserveCB);
	}	break;
	}

	if(framework.debugUI)
		framework.debugUI->DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderWindow->GetBackBuffer());

	RmlPassData passData{
		.constantBuffer = reserveCB,
		.vertexBuffer	= reserveVB,
		.renderTarget	= renderWindow->GetBackBuffer()
	};

	ui.Draw(update, core, passData, dT, frameGraph);

	PresentBackBuffer(frameGraph, *renderWindow);
	return nullptr;
}


/************************************************************************************************/


void HairRenderingTest::PostDrawUpdate(EngineCore& core, double dT)
{
	renderWindow->Present(core.vSync, 0);

	core.RenderSystem.ResetConstantBuffer(constantBuffer);
}


/************************************************************************************************/


bool HairRenderingTest::EventHandler(Event evt)
{
	if (evt.InputSource == Event::Keyboard && evt.Action == Event::Release && evt.mData1.mKC[0] == FlexKit::KC_SPACE)
	{
		pause = !pause;

		fmt::print("Simulation {}\n", (pause ? " Paused\n" : " Running\n"));
		return true;
	}
	else if (evt.InputSource == Event::Keyboard && evt.Action == Event::Release && evt.mData1.mKC[0] == FlexKit::KC_R)
	{
		if (auto import = ImportCSV(R"(assets\hair.csv)", framework.core.GetTempMemory()); import)
		{
			fmt::print("Reloading Style\n");

			auto& controlPoints = import.value();
			UploadHairStyle(style, controlPoints, framework.GetRenderSystem());
			return true;
		}
		else return false;
	}
	else if (evt.InputSource == Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_I && evt.Action == Event::Pressed)
	{
		debugOffset = (debugOffset != 3) ? debugOffset = FlexKit::clamp(0u, debugOffset += 1u, 3u) : 0u;
		return true;
	}
	else if (evt.InputSource == Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_ESC)
	{
		framework.quit = true;
		return true;
	}
	else if (evt.InputSource == Event::Keyboard && evt.mData1.mKC[0] == FlexKit::KC_S && evt.Action == Event::Release)
	{
		fmt::print("Reloading Shaders\n");

		framework.GetRenderSystem().QueuePSOLoad(MBOITRender1);
		framework.GetRenderSystem().QueuePSOLoad(MBOITRender2);

		framework.GetRenderSystem().QueuePSOLoad(ApplyForces);
		framework.GetRenderSystem().QueuePSOLoad(ApplyShapeConstraints);
		framework.GetRenderSystem().QueuePSOLoad(ApplyEdgeLengthConstraint);
		return true;
	}
	else if (evt.InputSource == Event::E_SystemEvent && evt.Action == FlexKit::Event::Exit)
	{
		framework.quit = true;
		return true;
	}
	else
	{
		ui.HandleEvent(evt);
		return (framework.debugUI) ? framework.debugUI->HandleInput(evt) : false;
	}
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
