#pragma once
#include <RenderSystemInterface.hpp>
#include <Events.hpp>

using InputSubscriber   = FlexKit::EventNotifier<>::Subscriber&;
using InputEvent        = FlexKit::Event;
using IRenderWindow     = FlexKit::IRenderWindow;

IRenderWindow*  CreateAndroidRenderWindow(FlexKit::IRenderSystem& rendersystem, struct android_app* pApp);

void            RenderWindowProcessMessages(FlexKit::IRenderWindow*);
void            Subscribe(FlexKit::IRenderWindow* iwindow, InputSubscriber& subscriber);
void            NotifyEnd(FlexKit::IRenderWindow* iwindow);
