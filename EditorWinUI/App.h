#pragma once
#include "App.xaml.g.h"
#include "buildsettings.h"

namespace winrt::EditorWinUI::implementation
{
    struct App : AppT<App>
    {
        App();
        ~App();

        void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const&);
        void OnSuspending(IInspectable const&, Windows::ApplicationModel::SuspendingEventArgs const&);
        void OnNavigationFailed(IInspectable const&, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs const&);

        FlexKit::FKApplication fkEngine;
    };
}
