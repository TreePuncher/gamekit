#include <jni.h>
#include <android/log.h>
#include <vkBackend.hpp>
#include <Application.hpp>
#include <Logging.hpp>
#include <filesystem>

#include "AndroidIOHelpers.hpp"
#include "AndroidRenderWindow.hpp"
#include "native_app_glue/android_native_app_glue.h"
#include "game-activity/GameActivity.h"

struct UserApplication
{
    using FKApplication_ptr = std::unique_ptr<FlexKit::FKApplication>;

    FKApplication_ptr       app;
    FlexKit::EngineMemory*  memory          = nullptr;
    FlexKit::IRenderWindow* renderWindow    = nullptr;

    void Run()
    {
        if(app)
            app->Run();
    }

    void Wait()
    {
        if(app)
            app->Wait();
    }
};

extern void             SetupApplication(FlexKit::FKApplication& app, FlexKit::IRenderWindow* renderWindow, struct android_app *pApp);
FlexKit::IRenderWindow* CreateAndroidRenderWindow(FlexKit::IRenderSystem& rendersystem, android_app *pApp);


void AndroidLogHandler(void* User, const char* Str, uint64_t StrLen)
{
    __android_log_write(ANDROID_LOG_INFO, "FlexKit",  Str);
}

UserApplication* CreateApplication(android_app *pApp)
{
    auto* allocator = FlexKit::CreateEngineMemory();
    
    static FlexKit::LogCallback callback{
        .ID         = "Android", 
        .User       = nullptr, 
        .Callback   = AndroidLogHandler
    };


    __android_log_write(ANDROID_LOG_INFO, "FlexKit",  "Initialize Log!");

    FlexKit::AddLogCallback(&callback, 0);
    FK_LOG_INFO("TESTING LOG!");
    
    __android_log_write(ANDROID_LOG_INFO, "FlexKit",  "Beginning Engine Initialization!");

    auto app = std::make_unique<FlexKit::FKApplication>(allocator,
			FlexKit::CoreOptions{
				.GPUdebugMode		= true,
				.GPUValidation		= true,
				.GPUSyncQueues		= true,
				.CreateRenderSystem = FlexKit::CreateVK,
			}, FlexKit::FrameworkOptions{
				.integrateIMGUI		= false
			});

    __android_log_write(ANDROID_LOG_INFO, "FlexKit",  "Finished Engine Initialization!");

    auto renderWindow = CreateAndroidRenderWindow(app->GetRenderSystem(), pApp);

    auto userApp = new UserApplication{ 
        .app            = std::move(app), 
        .memory         = FlexKit::CreateEngineMemory(),
        .renderWindow   = renderWindow
     };

     return userApp;
}

void ReleaseApplication(UserApplication* userApp)
{
    userApp->renderWindow->Release();
    ReleaseEngineMemory(userApp->memory);
    userApp->app.reset();
}

void ProcessMessages(android_app *pApp)
{
    while (true) 
    {
        // 0 is non-blocking.
        int timeout = 0;
        int events;
        android_poll_source *pSource;
        int result = ALooper_pollOnce(timeout, nullptr, &events,
                                        reinterpret_cast<void**>(&pSource));
        switch (result) {
            case ALOOPER_POLL_TIMEOUT:
                [[clang::fallthrough]];
            case ALOOPER_POLL_WAKE:
                return;
            case ALOOPER_EVENT_ERROR:
                __android_log_write(ANDROID_LOG_INFO, "FlexKit", "ProcessMessage: ALooper_pollOnce returned an error");
                return;
            case ALOOPER_POLL_CALLBACK:
                break;
            default:
            {
                if (pSource)
                    pSource->process(pApp, pSource);
            }
        }
    }
}

extern "C" 
{

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */

    void handle_cmd(android_app *pApp, int32_t cmd) 
    {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
            {
                // A new window is created, associate a renderer with it. You may replace this with a
                // "game" class if that suits your needs. Remember to change all instances of userData
                // if you change the class here as a reinterpret_cast is dangerous this in the
                // android_main function and the APP_CMD_TERM_WINDOW handler case.
                __android_log_write(ANDROID_LOG_INFO, "FlexKit", "handle_cmd: Init!");
                
                auto userApp = CreateApplication(pApp);
                pApp->userData = userApp; 

                SetupApplication(
                    *userApp->app.get(), 
                    userApp->renderWindow, 
                    pApp);

            }   break;
            case APP_CMD_TERM_WINDOW:
                __android_log_write(ANDROID_LOG_INFO, "FlexKit", "handle_cmd: Terminate!");

                // The window is being destroyed. Use this to clean up your userData to avoid leaking
                // resources.
                //
                // We have to check if userData is assigned just in case this comes in really quickly
                if (pApp->userData)
                {   // Notify termination
                    UserApplication* userApp = reinterpret_cast<UserApplication*>(pApp->userData);
                    NotifyEnd(userApp->renderWindow);
                    userApp->Wait();

                    ReleaseApplication(userApp);
                }
                break;
            default:
                break;
        }
    }

/*!
 * Enable the motion events you want to handle; not handled events are
 * passed back to OS for further processing. For this example case,
 * only pointer and joystick devices are enabled.
 *
 * @param motionEvent the newly arrived GameActivityMotionEvent.
 * @return true if the event is from a pointer or joystick device,
 *         false for all other input devices.
 */
    bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) 
    {
        auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
        return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
                sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
    }

/*!
 * This the main entry point for a native activity
 */
    void android_main(struct android_app *pApp) 
    {
        using namespace std::filesystem;
        __android_log_write(ANDROID_LOG_INFO, "FlexKit", "android_main: initializing");

        // Register an event handler for Android events
        pApp->onAppCmd = handle_cmd;

        // Set input event filters (set it to NULL if the app wants to process all inputs).
        // Note that for key inputs, this example uses the default default_key_filter()
        // implemented in android_native_app_glue.c.
        android_app_set_motion_event_filter(pApp, motion_event_filter_func);

        while(!pApp->userData)
            ProcessMessages(pApp);

        __android_log_write(ANDROID_LOG_INFO, "FlexKit", "android_main: running");

        // This sets up a typical game/event loop. It will run until the app is destroyed.
        UserApplication* userApp = reinterpret_cast<UserApplication*>(pApp->userData);
        userApp->Run();

        __android_log_write(ANDROID_LOG_INFO, "FlexKit", "android_main: closing");
    }
}