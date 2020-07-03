#pragma once

#include "SceneViewport.g.h"

namespace winrt::EditorWinUI::implementation
{
    class UWPViewport : public FlexKit::IRenderWindow
    {
    public:
        UWPViewport() = default;
        UWPViewport(winrt::Windows::UI::Xaml::Controls::SwapChainPanel, FlexKit::RenderSystem* renderSystem);

        ~UWPViewport()
        {
            Release();
        }

        void Release()
        {
            if(renderSystem)
                renderSystem->ReleaseTexture(backBuffer);

            if(DXGIswapChain)
                DXGIswapChain->Release();

            renderSystem    = nullptr;
            backBuffer      = FlexKit::InvalidHandle_t;
            DXGIswapChain   = nullptr;
        }

        FlexKit::ResourceHandle GetBackBuffer() const { return backBuffer; }
        bool                    Present(const uint32_t syncInternal, const uint32_t flags) final;
        FlexKit::uint2          GetWH() const final;
        void                    Resize(const FlexKit::uint2 WH) final;

        IDXGISwapChain4*        _GetSwapChain() const { return DXGIswapChain;}

        IDXGISwapChain4*        DXGIswapChain   = nullptr;
        FlexKit::ResourceHandle backBuffer      = FlexKit::InvalidHandle_t;
        FlexKit::RenderSystem*  renderSystem    = nullptr;
    };

    struct SceneViewport : SceneViewportT<SceneViewport>
    {
        SceneViewport();
        ~SceneViewport();

        void Present        (int interval = 0, int flags = 0);
        void ClickHandler   (Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

        UWPViewport viewport;
    };
}

namespace winrt::EditorWinUI::factory_implementation
{
    struct SceneViewport : SceneViewportT<SceneViewport, implementation::SceneViewport>
    {
    };
}
