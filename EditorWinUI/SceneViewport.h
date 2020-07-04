#pragma once

#include "SceneViewport.g.h"
#include "windows.ui.xaml.media.dxinterop.h"

namespace winrt::EditorWinUI::implementation
{
    class UWPViewport : public FlexKit::IRenderWindow
    {
    public:
        UWPViewport() = default;
        UWPViewport(const FlexKit::uint2 WH, winrt::Windows::UI::Xaml::Controls::SwapChainPanel, FlexKit::RenderSystem* renderSystem);

        ~UWPViewport()
        {
            Release();
        }

        void Release()
        {
            if (sisNative)
                sisNative->SetSwapChain(nullptr);

            if(renderSystem)
                renderSystem->ReleaseTexture(backBuffer);

            if(DXGIswapChain)
                DXGIswapChain->Release();

            sisNative       = nullptr;
            renderSystem    = nullptr;
            backBuffer      = FlexKit::InvalidHandle_t;
            DXGIswapChain   = nullptr;
        }

        operator bool() const { return (DXGIswapChain != nullptr); }

        FlexKit::ResourceHandle GetBackBuffer() const { return backBuffer; }
        bool                    Present(const uint32_t syncInternal, const uint32_t flags) final;
        FlexKit::uint2          GetWH() const final;
        void                    Resize(const FlexKit::uint2 WH) final;

        IDXGISwapChain4*        _GetSwapChain() const { return DXGIswapChain;}

        IDXGISwapChain4*        DXGIswapChain   = nullptr;
        ISwapChainPanelNative*  sisNative       = nullptr;

        FlexKit::ResourceHandle backBuffer      = FlexKit::InvalidHandle_t;
        FlexKit::RenderSystem*  renderSystem    = nullptr;
    };

    class ViewportSceneState : public FlexKit::FrameworkState
    {
    public:
        ViewportSceneState(FlexKit::GameFramework& framework) : FlexKit::FrameworkState{ framework } {}

        FlexKit::GraphicScene scene;
    };

    struct SceneViewport : SceneViewportT<SceneViewport>
    {
        SceneViewport();
        ~SceneViewport();

        void Present        (int interval = 0, int flags = 0);
        void ClickHandler   (Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

        UWPViewport         viewport;
        ViewportSceneState* viewPortState;

        void swapChain_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::SizeChangedEventArgs const& e);
        void Grid_LostFocus(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::EditorWinUI::factory_implementation
{
    struct SceneViewport : SceneViewportT<SceneViewport, implementation::SceneViewport>
    {
    };
}
