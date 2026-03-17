#include "tinalux/app/android/AndroidNativeBridge.h"

#include <new>

#include "tinalux/app/android/AndroidRuntime.h"
#include "tinalux/core/Log.h"
#include "tinalux/ui/DemoScene.h"
#include "tinalux/ui/Theme.h"

namespace {

tinalux::app::android::AndroidRuntime* runtimeFromHandle(void* runtimeHandle)
{
    return static_cast<tinalux::app::android::AndroidRuntime*>(runtimeHandle);
}

}  // namespace

void* tinaluxAndroidCreateRuntime(void)
{
    auto* runtime = new (std::nothrow) tinalux::app::android::AndroidRuntime();
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "Failed to allocate Android runtime instance");
    }
    return runtime;
}

void tinaluxAndroidDestroyRuntime(void* runtimeHandle)
{
    delete runtimeFromHandle(runtimeHandle);
}

bool tinaluxAndroidAttachWindow(
    void* runtimeHandle,
    void* nativeWindow,
    float dpiScale)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "AttachWindow ignored because runtime handle is null");
        return false;
    }

    return runtime->attachWindow(nativeWindow, dpiScale);
}

void tinaluxAndroidDetachWindow(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime != nullptr) {
        runtime->detachWindow();
    }
}

bool tinaluxAndroidRenderOnce(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "RenderOnce ignored because runtime handle is null");
        return false;
    }

    return runtime->renderOnce();
}

bool tinaluxAndroidInstallDemoScene(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "InstallDemoScene ignored because runtime handle is null");
        return false;
    }

    auto* application = runtime->application();
    if (application == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "InstallDemoScene requires an attached Android window");
        return false;
    }

    const tinalux::ui::Theme theme = tinalux::ui::Theme::dark();
    application->setTheme(theme);
    application->setRootWidget(
        tinalux::ui::createDemoScene(theme, application->animationSink()));
    return true;
}

bool tinaluxAndroidDispatchPointerMove(void* runtimeHandle, double x, double y)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchPointerMove ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchPointerMove(x, y);
}

bool tinaluxAndroidDispatchPointerDown(void* runtimeHandle, double x, double y)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchPointerDown ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchPointerDown(x, y);
}

bool tinaluxAndroidDispatchPointerUp(void* runtimeHandle, double x, double y)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchPointerUp ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchPointerUp(x, y);
}
