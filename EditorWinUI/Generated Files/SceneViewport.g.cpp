// WARNING: Please don't edit this file. It was generated by C++/WinRT v2.0.200630.5

void* winrt_make_EditorWinUI_SceneViewport()
{
    return winrt::detach_abi(winrt::make<winrt::EditorWinUI::factory_implementation::SceneViewport>());
}
WINRT_EXPORT namespace winrt::EditorWinUI
{
    SceneViewport::SceneViewport() :
        SceneViewport(make<EditorWinUI::implementation::SceneViewport>())
    {
    }
}