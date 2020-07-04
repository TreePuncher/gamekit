#include "pch.h"
#include "MainPage.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

using namespace winrt::Windows::UI::Xaml;

namespace winrt::EditorWinUI::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
    }

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}


void winrt::EditorWinUI::implementation::MainPage::AddSceneViewPort(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e)
{
    auto newTab = Microsoft::UI::Xaml::Controls::TabViewItem();
    auto header = winrt::Windows::UI::Xaml::Controls::TextBlock();
    header.Text(L"Scene Viewport");

    newTab.Header(header);
    newTab.Content(SceneViewport());

    tabs().TabItems().Append(newTab);
}

void winrt::EditorWinUI::implementation::MainPage::AddResourceView(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e)
{

}

