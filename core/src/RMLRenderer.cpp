#include "Containers.hpp"
#include "Events.hpp"
#include "Graphics.hpp"
#include "FrameGraph.hpp"
#include "MathUtilities.hpp"
#include "RMLRenderer.hpp"

#include <filesystem>
#include <fp16.h>
#include <span>
#include <stacktrace>

#undef GetNextSibling

#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>


namespace FlexKit
{	/************************************************************************************************/


	struct BeginResources
	{
		ResourceHandle					renderTarget;
	};


	uint64_t xorshift64(uint64_t& state)
	{
		uint64_t x = state;
		x ^= x << 13;
		x ^= x >> 7;
		x ^= x << 17;
		return state = x;
	}


	class RmlRenderer : public Rml::RenderInterface
	{
	public:
		explicit RmlRenderer(RenderSystem& IN_renderSystem);

		void Begin(FlexKit::Context& IN_ctx, BeginResources& resources, FlexKit::iAllocator& IN_allocator);

		void End();

		Rml::CompiledGeometryHandle	CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
		void						RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
		void						ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;


		void EnableScissorRegion(bool enable) override;

		void SetScissorRegion(Rml::Rectanglei region);
		void SetTransform(const Rml::Matrix4f* IN_transform) override;

		Rml::TextureHandle	LoadTexture		(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
		Rml::TextureHandle	GenerateTexture	(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
		void				ReleaseTexture	(Rml::TextureHandle texture) override;

		BeginResources*		pass			= nullptr;
		Context*			ctx				= nullptr;
		iAllocator*			allocator		= nullptr;
		ResourceHandle		renderTarget	= FlexKit::InvalidHandle;
		RenderSystem&		renderSystem;
		uint64_t			randState		= 1234;

		CopyContextHandle copyHandle	= FlexKit::InvalidHandle;

		struct TextureHandle
		{
			DescriptorRange	range;
			ResourceHandle	handle;

			operator DescriptorRange()	{ return range; }
			operator ResourceHandle()	{ return handle; }
		};


		struct GeometryHandle
		{
			ResourceHandle	vertexBuffer;
			ResourceHandle	indexBuffer;
			uint32_t		indexCount;
			uint32_t		vertexCount;
		};

		float4x4							transform;
		HashTable<TextureHandle, uint64_t>	textures;
		HashTable<GeometryHandle, uint64_t>	geometry;
	};


	/************************************************************************************************/


	class RmlSystemInterface : public Rml::SystemInterface
	{
		double GetElapsedTime() override;

		std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
	};


	/************************************************************************************************/


	class RmlUI
	{
	public:
		explicit RmlUI(FlexKit::RenderSystem& IN_renderSystem);

		~RmlUI();
		FlexKit::UpdateTask* Update(Rml::Context* ctx, struct EngineCore&, UpdateDispatcher&, double dT);

		void* Draw(Rml::Context* ctx, struct FlexKit::UpdateTask* update, struct EngineCore& core, RmlPassData& passData, double dT, class FrameGraph& frameGraph);

		void HandleEvent(Rml::Context* ctx, const FlexKit::Event& evt);

		Rml::Context*		context;  // Main context
		RenderSystem&		renderSystem;
		RmlRenderer			renderer;
		RmlSystemInterface	systemInterface;
	};


	/************************************************************************************************/


	RmlRenderer::RmlRenderer(FlexKit::RenderSystem& IN_renderSystem) :
		renderSystem	{ IN_renderSystem			},
		textures		{ IN_renderSystem.Memory	},
		geometry		{ IN_renderSystem.Memory	}
	{
		renderSystem.RegisterPSOLoader(RMLDrawPSO,
			[&](FlexKit::RenderSystem* renderSystem, FlexKit::iAllocator& allocator) -> FlexKit::LoadPipelineStateRes
			{
				FlexKit::PipelineBuilder builder{ allocator };
				builder.AddInputTopology(FlexKit::ETopology::EIT_TRIANGLE);
				builder.AddInputLayout(
					{	.inputs		= {
							FlexKit::EInputElement{
								.name				= "POSITION",
								.format				= FlexKit::DeviceFormat::R16G16_FLOAT,
								.alignedByteOffset	= 0,
							},
							FlexKit::EInputElement{
								.name				= "TEXCOORD",
								.format				= FlexKit::DeviceFormat::R16G16_FLOAT,
								.alignedByteOffset	= 4,
							},
							FlexKit::EInputElement{
								.name				= "COLOR",
								.format				= FlexKit::DeviceFormat::R8G8B8A8_UNORM,
								.alignedByteOffset	= 8,
							},
						},
						.count		= 3 });
				builder.AddVertexShader	("VMain", R"(assets/shaders/RMLUI/Vertex.hlsl)", { .enable16BitTypes = true });
				builder.AddPixelShader	("PMain", R"(assets/shaders/RMLUI/Pixel.hlsl)");
				builder.AddBlendState	(FlexKit::BlendState::Blend());
				builder.AddRasterizerState({
					.CullMode = FlexKit::ECullMode::NONE }); 
				builder.AddRenderTargetState({
					.targetCount	= 1,
					.targetFormats	= { FlexKit::DeviceFormat::R16G16B16A16_FLOAT }});
				

				auto res = builder.Build(*renderSystem);
				return res;
			});

		renderSystem.RegisterPSOLoader(RMLDraw2PSO,
			[&](FlexKit::RenderSystem* renderSystem, FlexKit::iAllocator& allocator) -> FlexKit::LoadPipelineStateRes
			{
				FlexKit::PipelineBuilder builder{ allocator };
				builder.AddInputTopology(FlexKit::ETopology::EIT_TRIANGLE);
				builder.AddInputLayout(
					{	.inputs		= {
							FlexKit::EInputElement{
								.name				= "POSITION",
								.format				= FlexKit::DeviceFormat::R16G16_FLOAT,
								.alignedByteOffset	= 0,
							},
							FlexKit::EInputElement{
								.name				= "TEXCOORD",
								.format				= FlexKit::DeviceFormat::R16G16_FLOAT,
								.alignedByteOffset	= 4,
							},
							FlexKit::EInputElement{
								.name				= "COLOR",
								.format				= FlexKit::DeviceFormat::R8G8B8A8_UNORM,
								.alignedByteOffset	= 8,
							},
						},
						.count		= 3 });
				builder.AddVertexShader	("VMain", R"(assets/shaders/RMLUI/Vertex.hlsl)", { .enable16BitTypes = true });
				builder.AddPixelShader	("TexturedPMain", R"(assets/shaders/RMLUI/Pixel.hlsl)");
				builder.AddBlendState	(FlexKit::BlendState::Blend());
				builder.AddRasterizerState({
					.CullMode = FlexKit::ECullMode::NONE }); 
				builder.AddRenderTargetState({
					.targetCount	= 1,
					.targetFormats	= { FlexKit::DeviceFormat::R16G16B16A16_FLOAT }});
				

				auto res = builder.Build(*renderSystem);
				return res;
			});

		renderSystem.QueuePSOLoad(RMLDrawPSO);
		renderSystem.QueuePSOLoad(RMLDraw2PSO);
	}


	/************************************************************************************************/


	void RmlRenderer::Begin(FlexKit::Context& IN_ctx, BeginResources& resources, FlexKit::iAllocator& IN_allocator)
	{
		ctx = &IN_ctx;
		ctx->SetRenderTargets({ resources.renderTarget }, false);
		ctx->SetScissorAndViewports({ resources.renderTarget });

		pass = &resources;

		renderTarget	= resources.renderTarget;
		copyHandle		= renderSystem.OpenUploadQueue();
		transform		= FlexKit::float4x4::Identity();
	}


	/************************************************************************************************/


	void RmlRenderer::End()
	{
		renderSystem.SubmitUploadQueues(&copyHandle);
		renderSystem.SyncDirectTo(renderSystem.SyncUploadTicket());

		ctx			= nullptr;
		pass		= nullptr;
		copyHandle	= FlexKit::InvalidHandle;

	}


	/************************************************************************************************/


	Rml::CompiledGeometryHandle	RmlRenderer::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
	{
		const uint32_t		vbSize = vertices.size() * sizeof(RMLVertex);
		const uint32_t		ibSize = indices.size() * sizeof(uint32_t);

		auto copyCtx		= renderSystem.GetImmediateCopyQueue();
		auto vertexBuffer	= renderSystem.CreateGPUResource(GPUResourceDesc::StructuredResource(vbSize));
		auto indexBuffer	= renderSystem.CreateGPUResource(GPUResourceDesc::StructuredResource(ibSize));

		auto& ctx = renderSystem._GetCopyContext(copyCtx);

		const auto vbReservation = ctx.Reserve(vbSize);
		const auto ibReservation = ctx.Reserve(ibSize);

		for(auto&& [idx, v] : enumerate(vertices))
		{
			RMLVertex vertex;
			vertex.color[0] = v.colour.red;
			vertex.color[1] = v.colour.green;
			vertex.color[2] = v.colour.blue;
			vertex.color[3] = v.colour.alpha;

			vertex.point[0] = fp16_ieee_from_fp32_value(v.position.x);
			vertex.point[1] = fp16_ieee_from_fp32_value(v.position.y);
			vertex.UV[0]	= fp16_ieee_from_fp32_value(v.tex_coord.x);
			vertex.UV[1]	= fp16_ieee_from_fp32_value(v.tex_coord.y);

			memcpy(vbReservation.buffer + sizeof(RMLVertex) * idx, &vertex, sizeof(vertex));
		}

		memcpy(ibReservation.buffer, indices.data(), ibSize);

		ctx.CopyBuffer(vertexBuffer,	0, vbReservation);
		ctx.CopyBuffer(indexBuffer,		0, ibReservation);

		GeometryHandle geometryEntry;
		geometryEntry.indexBuffer	= indexBuffer;
		geometryEntry.vertexBuffer	= vertexBuffer;
		geometryEntry.indexCount	= indices.size();
		geometryEntry.vertexCount	= vertices.size();

		const uint64_t geometryHandle = xorshift64(randState);

		geometry.insert(geometryHandle, geometryEntry);

		return geometryHandle;
	}


	/************************************************************************************************/


	void RmlRenderer::ReleaseGeometry(Rml::CompiledGeometryHandle geometryHandle)
	{
		auto geometryEntry = geometry.find(geometryHandle);
		if (!geometryEntry)
			return;

		renderSystem.ReleaseResource(geometryEntry->indexBuffer);
		renderSystem.ReleaseResource(geometryEntry->vertexBuffer);

		geometry.remove(geometryHandle);
	}


	/************************************************************************************************/


	void RmlRenderer::RenderGeometry(Rml::CompiledGeometryHandle geometryHandle, Rml::Vector2f translation, Rml::TextureHandle texture)
	{
		if (!ctx)
			return;

		auto geometryEntry = geometry.find(geometryHandle);
		if (!geometryEntry)
			return;

		auto wh = renderSystem.GetTextureWH(renderTarget);

		if (!texture)
			ctx->SetGraphicsPipelineState(RMLDrawPSO, *allocator);
		else if (auto resource = textures.find(texture); resource)
		{
			ctx->SetGraphicsPipelineState(RMLDraw2PSO, *allocator);
			ctx->SetGraphicsDescriptorTable(1u, *resource);
		}
		else
			return;

		ctx->SetRenderTargets({ pass->renderTarget }, false);
		ctx->SetScissorAndViewports({ pass->renderTarget });

		D3D12_VERTEX_BUFFER_VIEW vbView;
		vbView.BufferLocation	= ctx->renderSystem->GetDeviceResource(geometryEntry->vertexBuffer)->GetGPUVirtualAddress();
		vbView.SizeInBytes		= geometryEntry->vertexCount * sizeof(RMLVertex);
		vbView.StrideInBytes	= sizeof(RMLVertex);

		ctx->SetVertexBuffers2({ &vbView, 1 });
		ctx->SetIndexBuffer(geometryEntry->indexBuffer);

		auto temp = transform;
		temp(0, 3) = 0;
		temp(1, 3) = 0;

		ctx->SetGraphicsConstantValue(0, 16, &temp);
		ctx->SetGraphicsConstantValue(0, 2, &wh, 16);
		ctx->SetGraphicsConstantValue(0, 2, &translation, 18);
		ctx->DrawIndexed(geometryEntry->indexCount);
	}


	/************************************************************************************************/


	void RmlRenderer::EnableScissorRegion(bool enable)
	{
		if (!ctx)
			return;

		if (!enable)
		{
			auto WH = renderSystem.GetTextureWH(renderTarget);
			ctx->SetScissorRects(
				{
					D3D12_RECT{
						.left	= 0,
						.top	= 0,
						.right	= (LONG)WH[0],
						.bottom	= (LONG)WH[1]
					}
				});
		}
	}


	/************************************************************************************************/


	void RmlRenderer::SetScissorRegion(Rml::Rectanglei region)
	{
		if (!ctx)
			return;

		ctx->SetScissorRects(
			{
				D3D12_RECT{
					.left	= region.Left(),
					.top	= region.Top(),
					.right	= region.Right(),
					.bottom = region.Bottom()
				}
			});
	}


	/************************************************************************************************/


	void RmlRenderer::SetTransform(const Rml::Matrix4f* IN_transform)
	{
		if (IN_transform && ctx)
		{
			memcpy(&transform, IN_transform, sizeof(transform));
		}
		else
			transform = FlexKit::float4x4::Identity();
	}


	/************************************************************************************************/


	extern "C" { unsigned char* stbi_load(const char*, int*, int*, int*, int); };

	Rml::TextureHandle RmlRenderer::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
	{
		if(std::filesystem::exists(source))
		{
			std::filesystem::path fileDir{ source };
			auto extension = fileDir.extension();

			int w;
			int h;
			int channels;
			unsigned char* img = stbi_load(source.c_str(), &w, &h, &channels, 4);

			if (!img)
				return false;

			const size_t rowPitch	= FlexKit::AlignedSize(w * sizeof(FlexKit::RGBA), 256);
			const size_t bufferSize = rowPitch * h;

			FlexKit::TextureBuffer sourceBuffer{ { w, h }, (std::byte*)img, (size_t)FlexKit::Max(channels, 3)};

			FlexKit::TextureBuffer buffer{
				FlexKit::uint2{ (uint32_t)w, (uint32_t)h },
				(std::byte*)renderSystem.Memory->_aligned_malloc(rowPitch * h, 256),
				bufferSize,
				sizeof(FlexKit::RGBA),
				renderSystem.Memory
			};

			FlexKit::TextureBufferView<FlexKit::RGB>	inputView	{ sourceBuffer };
			FlexKit::TextureBufferView<FlexKit::RGBA>	inputView2	{ sourceBuffer };
			FlexKit::TextureBufferView<FlexKit::RGBA>	outputView	{ buffer, rowPitch };

			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					switch (channels)
					{
					case 1:
					{
						auto r = inputView[{ x, y }].Red;
						outputView[{x, y}] = {
							.Red	= std::byte(r),
							.Green	= std::byte(r),
							.Blue	= std::byte(r),
							.Alpha	= std::byte(0xff)
						};
					}	break;
					case 2:
					{
						auto r = inputView[{ x, y }].Red;
						auto g = inputView[{ x, y }].Green;
						outputView[{x, y}] = {
							.Red	= std::byte(r),
							.Green	= std::byte(g),
							.Blue	= std::byte(0x00),
							.Alpha	= std::byte(0xff)
						};
					}	break;
					case 3:
					{
						auto r = inputView[{ x, y }].Red;
						auto g = inputView[{ x, y }].Green;
						auto b = inputView[{ x, y }].Blue;
						outputView[{x, y}] = {
							.Red	= std::byte(r),
							.Green	= std::byte(g),
							.Blue	= std::byte(b),
							.Alpha	= std::byte(0xff)
						};
					}	break;
					case 4:
					{
						outputView[{x, y}] = inputView2[{x, y}];
					}	break;
					}
				}
			}

			texture_dimensions = { w, h };

			free(img);

			auto copy		= renderSystem.OpenUploadQueue();
			auto resource	= FlexKit::LoadTexture(&buffer, copy, renderSystem, renderSystem.Memory);

			renderSystem.SubmitUploadQueues(&copy);

			const auto res = renderSystem._AllocateDescriptorRange(1);
			if (!res.has_value())
				return false;

			PushTextureToDescHeap(renderSystem, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, resource, res.value());

			uint64_t textureHandle = std::hash<uint64_t>{}(resource);
			textures.insert(textureHandle, { res.value(), resource });

			return textureHandle;
		}

		return false;
	}


	/************************************************************************************************/


	Rml::TextureHandle RmlRenderer::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
	{
		if (!ctx)
		{
			auto resource = renderSystem.CreateGPUResource(
				FlexKit::GPUResourceDesc::ShaderResource({ source_dimensions.x, source_dimensions.y }, FlexKit::DeviceFormat::R8G8B8A8_UNORM));

			size_t bufferSize	= source_dimensions.x * source_dimensions.y * 4;
			auto uploadSpace	= ctx->ReserveDirectUploadSpace(bufferSize);

			if (uploadSpace)
				return false;

			memcpy(uploadSpace.buffer, source.data(), source.size());
			ctx->AddCopyResourceBarrier(resource, FlexKit::DASCommon, FlexKit::DASCopyDest);
			ctx->CopyTextureRegion(resource, 0, { 0, 0, 0 }, uploadSpace);
			ctx->AddCopyResourceBarrier(resource, FlexKit::DASCopyDest, FlexKit::DASCommon);

			const auto res = renderSystem._AllocateDescriptorRange(1);
			if (!res.has_value())
				return false;

			PushTextureToDescHeap(renderSystem, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, resource, res.value());

			uint64_t texture_handle = std::hash<uint64_t>{}(resource);
			textures.insert(texture_handle, { res.value(), resource });

			return texture_handle;
		}
		else
		{
			auto resource = renderSystem.CreateGPUResource(
				FlexKit::GPUResourceDesc::ShaderResource({ source_dimensions.x, source_dimensions.y }, FlexKit::DeviceFormat::R8G8B8A8_UNORM));

			size_t bufferSize = source_dimensions.x * source_dimensions.y * 4;

			auto& copyContext	= renderSystem._GetCopyContext(copyHandle);

			auto uploadSpace = copyContext.Reserve(bufferSize);
			memcpy(uploadSpace.buffer, source.data(), source.size());
			ctx->CopyTextureRegion(resource, 0, { 0, 0, 0 }, uploadSpace);

			const auto res = renderSystem._AllocateDescriptorRange(1);
			if (!res.has_value())
				return false;

			PushTextureToDescHeap(renderSystem, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, resource, res.value());

			uint64_t texture_handle = xorshift64(randState);
			textures.insert(texture_handle, { res.value(), resource });

			return texture_handle;
		}

		return 0u;
	}


	/************************************************************************************************/


	void RmlRenderer::ReleaseTexture(Rml::TextureHandle texture)
	{
		if (auto res = textures.find(texture); res)
		{
			renderSystem._ReleaseDescriptorRange(res->range, renderSystem.GetCurrentCounter());
			renderSystem.ReleaseResource(res->handle);
		}
	}


	/************************************************************************************************/


	double RmlSystemInterface::GetElapsedTime()
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto duration = now - begin;

		using durationUnit = std::chrono::duration<double>;
		auto d = std::chrono::duration_cast<durationUnit>(duration).count();
		return d;
	}


	/************************************************************************************************/


	std::string CreateRuntimeErrorMessage(const char* message)
	{
		std::string errorMessage = message;
		errorMessage += "\n";

		std::stacktrace trace = std::stacktrace::current();
		for (auto& stack : std::span{ trace.begin() + 1, trace.end() })
			errorMessage += std::format("{} : {} : {}\n", stack.description(), stack.source_file(), stack.source_line());

		return errorMessage;
	}


	/************************************************************************************************/


	RmlUI::RmlUI(FlexKit::RenderSystem& IN_renderSystem) :
		renderSystem	{ IN_renderSystem },
		renderer		{ IN_renderSystem }
	{
		Rml::SetRenderInterface(&renderer);
		Rml::SetSystemInterface(&systemInterface);
		Rml::Initialise();

		context = Rml::CreateContext("main", Rml::Vector2i(1920, 1080));

		if (!context)
			throw std::runtime_error{ CreateRuntimeErrorMessage("Failed to create RmlUI Context") };

		if(!Rml::LoadFontFace(R"(assets/fonts/RobotoCondensed-Regular.ttf)"))
			throw std::runtime_error{ CreateRuntimeErrorMessage("Failed to load default font") };

		Rml::Debugger::Initialise(context);
		Rml::Debugger::SetVisible(true);
	}


	/************************************************************************************************/

	
	RmlUI::~RmlUI()
	{
		Rml::Debugger::Shutdown();
		Rml::RemoveContext("main");
		Rml::Shutdown();
	}


	/************************************************************************************************/


	FlexKit::UpdateTask* RmlUI::Update(Rml::Context* uiCtx, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT)
	{
		uiCtx->Update();
		return nullptr;
	}


	/************************************************************************************************/


	void* RmlUI::Draw(Rml::Context* uiCtx, FlexKit::UpdateTask* update, FlexKit::EngineCore& core, RmlPassData& passData, double dT, FlexKit::FrameGraph& frameGraph)
	{
		auto& node = frameGraph.AddNode<BeginResources> (
			BeginResources{
				.renderTarget	= passData.renderTarget,
			},
			[&](FrameGraphNodeBuilder& builder, BeginResources& data)
			{
				builder.RenderTarget(passData.renderTarget);
			},
			[this, uiCtx](auto&& data,  auto& resources, FlexKit::Context& ctx, auto& allocator)
			{
				ctx.BeginEvent_DEBUG("RML");
				renderer.Begin(ctx, data, allocator);
				uiCtx->Render();
				renderer.End();
				ctx.EndEvent_DEBUG();
			});

		return &node;
	}


	/************************************************************************************************/


	void RmlUI::HandleEvent(Rml::Context* uiCtx, const FlexKit::Event& evt)
	{
		switch (evt.InputSource)
		{
		case FlexKit::Event::Mouse:
		{
			switch (evt.Action)
			{
			case FlexKit::Event::Moved:
				uiCtx->ProcessMouseMove((int)evt.mData1.mINT[0], (int)evt.mData1.mINT[1], 0);
				break;
			case FlexKit::Event::Pressed:
			{
				if (evt.mData1.mKC[0] == FlexKit::KC_MOUSELEFT)
					uiCtx->ProcessMouseButtonDown(0, 0);

				if (evt.mData1.mKC[0] == FlexKit::KC_MOUSERIGHT)
					uiCtx->ProcessMouseButtonDown(1, 0);
			}	break;
			case FlexKit::Event::Release:
			{
				if (evt.mData1.mKC[0] == FlexKit::KC_MOUSELEFT)
					uiCtx->ProcessMouseButtonUp(0, 0);

				if (evt.mData1.mKC[0] == FlexKit::KC_MOUSERIGHT)
					uiCtx->ProcessMouseButtonUp(1, 0);
			}	break;
			}
		}
		}
	}


	/************************************************************************************************/


	RmlIntegrator::RmlIntegrator(RenderSystem& renderSystem, iAllocator& IN_allocator)
	{
		impl = &IN_allocator.allocate<RmlUI>(renderSystem);

		allocator = IN_allocator;
	}


	/************************************************************************************************/


	RmlIntegrator::~RmlIntegrator()
	{
		if (impl)
			allocator->release(*impl);
		
		impl		= nullptr;
		allocator	= nullptr;
	}


	/************************************************************************************************/


	class UpdateTask* RmlIntegrator::Update(class EngineCore& core, class UpdateDispatcher& dispatcher, double dT)
	{
		if (impl)
			return impl->Update(impl->context, core, dispatcher, dT);
		return nullptr;
	}


	/************************************************************************************************/


	void* RmlIntegrator::Draw(class UpdateTask* update, class EngineCore& core, RmlPassData& passData, double dT, class FrameGraph& frameGraph)
	{
		if (impl)
			return impl->Draw(impl->context, update, core, passData, dT, frameGraph);

		return nullptr;
	}


	/************************************************************************************************/


	UpdateTask* RmlIntegrator::Update(Rml::Context* uiCtx, EngineCore& core, UpdateDispatcher& dispatcher, double dT)
	{
		if (impl)
			return impl->Update(uiCtx, core, dispatcher, dT);
		return nullptr;
	}


	/************************************************************************************************/


	void* RmlIntegrator::Draw(Rml::Context* uiCtx, UpdateTask* update, EngineCore& core, RmlPassData& passData, double dT, FrameGraph& frameGraph)
	{
		if (impl)
			return impl->Draw(uiCtx, update, core, passData, dT, frameGraph);

		return nullptr;
	}


	/************************************************************************************************/


	void RmlIntegrator::HandleEvent(const FlexKit::Event& evt)
	{
		if (impl)
			impl->HandleEvent(impl->context, evt);
	}


	/************************************************************************************************/


	void RmlIntegrator::HandleEvent(Rml::Context* uiCtx, const FlexKit::Event& evt)
	{
		if (impl)
			impl->HandleEvent(uiCtx, evt);
	}


	/************************************************************************************************/


	Rml::Context* RmlIntegrator::GetMainContext()
	{
		return impl->context;
	}


	/************************************************************************************************/


	Rml::Context* RmlIntegrator::CreateContext(const char* id, const uint2& WH)
	{
		auto context = Rml::CreateContext(id, Rml::Vector2i(WH[0], WH[1]));

		if (!context)
			throw std::runtime_error{ CreateRuntimeErrorMessage("Failed to create RmlUI Context") };

		return context;
	}


}	/************************************************************************************************/


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
