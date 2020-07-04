#include "pch.h"
#include "SceneViewport.h"
#if __has_include("SceneViewport.g.cpp")
#include "SceneViewport.g.cpp"
#endif

#include "App.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dwrite.lib")

using namespace winrt::Windows::UI::Xaml;

namespace winrt::EditorWinUI::implementation
{
    UWPViewport::UWPViewport(const FlexKit::uint2 WH, winrt::Windows::UI::Xaml::Controls::SwapChainPanel swapChain, FlexKit::RenderSystem* IN_renderSystem) :
        renderSystem{ IN_renderSystem }
    {
        const auto width    = WH[0];
        const auto height   = WH[1];

        DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.BufferCount		= 3;
        SwapChainDesc.Width             = width;
        SwapChainDesc.Height            = height;
        SwapChainDesc.Format            = DXGI_FORMAT_R16G16B16A16_FLOAT;

		SwapChainDesc.BufferUsage		= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		SwapChainDesc.SampleDesc.Count	= 1;
        SwapChainDesc.Scaling           = DXGI_SCALING_STRETCH;
		SwapChainDesc.Flags             = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        auto swapChain_ = swapChain.as<IUnknown>()->QueryInterface(IID_PPV_ARGS(&sisNative));

        IDXGISwapChain1* dxgiSwapChain1 = nullptr;
        auto res    = renderSystem->pGIFactory->CreateSwapChainForComposition(renderSystem->GraphicsQueue, &SwapChainDesc, nullptr, &dxgiSwapChain1);

        DXGIswapChain = nullptr;
        FK_ASSERT(SUCCEEDED(dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&DXGIswapChain))));

        ID3D12Resource* buffer[3];
		for (size_t I = 0; I < SwapChainDesc.BufferCount; ++I)
		{
            DXGIswapChain->GetBuffer( I, __uuidof(ID3D12Resource), (void**)&buffer[I]);
			if (!buffer[I]) {
				FK_ASSERT(buffer[I], "Failed to Create Back Buffer!");
                return;
			}
		}

        backBuffer = renderSystem->CreateGPUResource(
            FlexKit::GPUResourceDesc::BackBuffered(
                { (uint32_t)width, (uint32_t)height },
                FlexKit::DeviceFormat::R16G16B16A16_FLOAT,
                buffer, 3));

        FK_ASSERT(SUCCEEDED(sisNative->SetSwapChain(dxgiSwapChain1)));

        renderSystem->SetDebugName(backBuffer, "Viewport");
        renderSystem->Textures.SetBufferedIdx(backBuffer, DXGIswapChain->GetCurrentBackBufferIndex());
    }


    bool UWPViewport::Present(const uint32_t syncInternal, const uint32_t flags)
    {
        DXGIswapChain->Present(syncInternal, flags);
        renderSystem->Textures.SetBufferedIdx(backBuffer, DXGIswapChain->GetCurrentBackBufferIndex());
        renderSystem->_PresentWindow(this);

        return true;
    }


    FlexKit::uint2 UWPViewport::GetWH() const
    {
        return renderSystem->GetTextureWH(backBuffer);
    }

    void UWPViewport::Resize(const FlexKit::uint2 WH)
    {
        if (renderSystem->GetTextureWH(backBuffer) == WH)
            return;

        auto res = renderSystem->GetDeviceResource(backBuffer);
        res->AddRef();
        while (res->Release());

		renderSystem->WaitforGPU();
        sisNative->SetSwapChain(nullptr);

		const auto HR = DXGIswapChain->ResizeBuffers(3, WH[0], WH[1], DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		if (FAILED(HR))
		{
			FK_ASSERT(0, "Failed to resize back buffer!");
		}

		//recreateBackBuffer
		ID3D12Resource* buffer[3];

		for (size_t I = 0; I < 3; ++I)
		{
            DXGIswapChain->GetBuffer(I, __uuidof(ID3D12Resource), (void**)&buffer[I]);
			if (!buffer[I])
				FK_ASSERT(buffer[I], "Failed to Create Back Buffer!");

		}

        sisNative->SetSwapChain(DXGIswapChain);
        
        renderSystem->_UpdateSubResources(backBuffer, buffer, 3);
		renderSystem->SetDebugName(backBuffer, "Viewport");
		renderSystem->Textures.SetBufferedIdx(backBuffer, DXGIswapChain->GetCurrentBackBufferIndex());
    }

    class TestBase : public FlexKit::FrameworkState
    {
    public:
        TestBase(FlexKit::GameFramework& IN_framework, FlexKit::IRenderWindow* IN_window) :
            renderWindow    { IN_window     },
            FrameworkState  { IN_framework  }{}


        void Draw(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph) final
        {
            auto backBuffer = renderWindow->GetBackBuffer();
            frameGraph.AddRenderTarget(backBuffer);
            FlexKit::ClearBackBuffer(frameGraph, backBuffer, FlexKit::float4{ (rand() % 128) / 256.0f, (rand() % 256) / 256.0f, (rand() % 128) / 256.0f, 1 });
            FlexKit::PresentBackBuffer(frameGraph, backBuffer);
        }

        void PostDrawUpdate(FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT) final
        {
            renderWindow->Present(1, 0);
        }

        FlexKit::IRenderWindow* renderWindow;
    };


    SceneViewport::SceneViewport()
    {
        InitializeComponent();


        const auto width   = swapChain().Width();
        const auto height  = swapChain().Height();

        if (width * height > 0)
        {
            auto& engine = Application::Current().as<App>().get()->fkEngine;

            viewport = UWPViewport{
                    FlexKit::uint2{ (uint32_t)width, (uint32_t)height },
                    swapChain(),
                    engine.GetCore().RenderSystem };

            engine.PushState<TestBase>(&viewport);
            engine.DrawOneFrame(0.0f);
            engine.PopState();
        }
    }


    SceneViewport::~SceneViewport()
    {
        viewport.Release();
    }
}


void winrt::EditorWinUI::implementation::SceneViewport::swapChain_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::SizeChangedEventArgs const& e)
{
    const auto size = e.NewSize();
    const auto bufferSize = viewport.GetWH();

    if (size.Height * size.Width == 0)
        return;

    auto& engine = Application::Current().as<App>().get()->fkEngine;

    if (!viewport)
    {
        viewport = UWPViewport{
                { (uint32_t)size.Width, (uint32_t)size.Height },
                swapChain(),
                engine.GetCore().RenderSystem };
    }
    else
        viewport.Resize({ (uint32_t)size.Width, (uint32_t)size.Height });

    engine.PushState<TestBase>(&viewport);
    engine.DrawOneFrame(0.0f);
    engine.PopState();
}


void winrt::EditorWinUI::implementation::SceneViewport::Grid_LostFocus(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e)
{
    if (viewport)
        viewport.Release();
}


void winrt::EditorWinUI::implementation::SceneViewport::ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args)
{
    auto& engine = Application::Current().as<App>().get()->fkEngine;

    engine.PushState<TestBase>(&viewport);
    engine.DrawOneFrame(0.0f);
    engine.PopState();
}
