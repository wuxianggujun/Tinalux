# Android平台多界面架构方案

## 概述

本文档分析Tinalux UI框架在Android平台上实现多界面的架构方案,对比不同实现方式的优缺点,并提供推荐方案。

## 当前架构分析

### 核心组件

1. **Application** (`tinalux::app::Application`)
   - 管理单个Window实例
   - 持有rootWidget和overlayWidget
   - 负责事件循环和渲染管线

2. **Window** (`tinalux::platform::Window`)
   - 平台抽象层,封装窗口系统
   - 当前实现:GLFWWindow(桌面平台)
   - 提供OpenGL上下文和事件回调

3. **UIContext** (`tinalux::app::UIContext`)
   - UI渲染和事件分发核心
   - 管理Widget树、焦点、动画等
   - 与Window解耦,可独立使用

## Android平台多界面实现方案

### 方案一:每个Activity一个独立渲染上下文 ⭐ 推荐

#### 架构设计

```
Activity1                    Activity2
    |                            |
    ├─ GLSurfaceView            ├─ GLSurfaceView
    ├─ AndroidWindow            ├─ AndroidWindow
    ├─ Application              ├─ Application
    │   ├─ UIContext            │   ├─ UIContext
    │   └─ rootWidget           │   └─ rootWidget
    └─ Native层                 └─ Native层
```

#### 实现要点

```cpp
// 1. 创建AndroidWindow实现
class AndroidWindow : public tinalux::platform::Window {
public:
    AndroidWindow(ANativeWindow* nativeWindow, JNIEnv* env, jobject activity);
    
    // 实现Window接口
    bool shouldClose() const override;
    void pollEvents() override;
    void swapBuffers() override;
    
    // Android特定
    void onSurfaceCreated();
    void onSurfaceChanged(int width, int height);
    void onSurfaceDestroyed();
    
private:
    ANativeWindow* nativeWindow_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
};

// 2. 每个Activity持有独立的Application实例
class TinaluxActivity {
private:
    std::unique_ptr<tinalux::app::Application> app_;
    std::unique_ptr<AndroidWindow> window_;
    
public:
    void onCreate() {
        // 创建Window
        window_ = std::make_unique<AndroidWindow>(
            ANativeWindow_fromSurface(env, surface),
            env,
            activity
        );
        
        // 创建Application
        app_ = std::make_unique<tinalux::app::Application>();
        app_->init(window_.get());
        
        // 设置UI
        app_->setRootWidget(createUI());
    }
    
    void onDestroy() {
        app_->shutdown();
        app_.reset();
        window_.reset();
    }
};
```

#### 优点

- ✅ **架构清晰**:每个Activity完全独立,符合Android生命周期
- ✅ **内存隔离**:Activity销毁时自动释放资源
- ✅ **状态管理简单**:不需要在界面间共享状态
- ✅ **线程安全**:每个Activity在自己的渲染线程
- ✅ **符合现有设计**:与桌面版架构一致

#### 缺点

- ⚠️ 界面切换时需要重新创建渲染上下文(可优化)
- ⚠️ 共享资源(字体、图片)需要单独管理

---

### 方案二:单Activity + 多UIContext

#### 架构设计

```
Activity
    |
    ├─ GLSurfaceView
    ├─ AndroidWindow
    ├─ Application
    │   ├─ NavigationManager
    │   │   ├─ UIContext (Screen1)
    │   │   ├─ UIContext (Screen2)
    │   │   └─ UIContext (Screen3)
    │   └─ 当前活动的UIContext
    └─ Native层
```

#### 实现要点

```cpp
// 导航管理器
class NavigationManager {
public:
    void pushScreen(std::shared_ptr<ui::Widget> screen) {
        auto context = std::make_unique<UIContext>();
        context->setRootWidget(screen);
        stack_.push_back(std::move(context));
        currentContext_ = stack_.back().get();
    }
    
    void popScreen() {
        if (stack_.size() > 1) {
            stack_.pop_back();
            currentContext_ = stack_.back().get();
        }
    }
    
    UIContext* currentContext() { return currentContext_; }
    
private:
    std::vector<std::unique_ptr<UIContext>> stack_;
    UIContext* currentContext_ = nullptr;
};

// Application修改
class Application {
public:
    void setNavigationManager(std::unique_ptr<NavigationManager> nav) {
        navigationManager_ = std::move(nav);
    }
    
    bool renderFrame() {
        if (auto* context = navigationManager_->currentContext()) {
            return context->render(canvas_, fbWidth_, fbHeight_);
        }
        return false;
    }
    
private:
    std::unique_ptr<NavigationManager> navigationManager_;
};
```

#### 优点

- ✅ **界面切换快速**:不需要重建OpenGL上下文
- ✅ **资源共享方便**:所有界面共享同一个ResourceLoader
- ✅ **导航栈管理**:方便实现前进/后退

#### 缺点

- ❌ **复杂度高**:需要大量修改Application核心代码
- ❌ **状态管理复杂**:多个UIContext的生命周期管理
- ❌ **内存占用**:所有界面的UIContext都保留在内存
- ❌ **与现有架构不符**:Application设计为单UIContext

---

### 方案三:Fragment + 共享Canvas

#### 架构设计

```
Activity
    |
    ├─ GLSurfaceView (共享)
    ├─ AndroidWindow
    ├─ Application
    │   └─ UIContext
    │       └─ rootWidget (Container)
    │           ├─ Fragment1Widget
    │           ├─ Fragment2Widget
    │           └─ Fragment3Widget (当前显示)
    └─ FragmentManager
```

#### 实现要点

```cpp
// 不需要多个UIContext,只是切换rootWidget
class ScreenManager {
public:
    void showScreen(const std::string& name) {
        if (auto it = screens_.find(name); it != screens_.end()) {
            app_->setRootWidget(it->second);
        }
    }
    
    void registerScreen(const std::string& name, 
                       std::shared_ptr<ui::Widget> screen) {
        screens_[name] = screen;
    }
    
private:
    tinalux::app::Application* app_;
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> screens_;
};
```

#### 优点

- ✅ **实现简单**:只需要切换rootWidget
- ✅ **性能好**:无需重建任何上下文
- ✅ **代码改动小**:不需要修改核心架构

#### 缺点

- ❌ **不符合Android规范**:没有使用Activity/Fragment机制
- ❌ **生命周期管理困难**:无法利用Android的生命周期
- ❌ **返回键处理复杂**:需要自己实现导航栈

---

## 推荐方案详解

### 采用方案一:每个Activity一个Application实例

#### 为什么选择这个方案?

1. **符合Android最佳实践**
   - 每个Activity代表一个独立的界面
   - 利用Android的生命周期管理
   - 支持系统返回键、任务切换等

2. **与现有架构完美契合**
   - Application设计就是管理单个窗口
   - UIContext是独立的渲染上下文
   - 不需要修改核心代码

3. **资源管理清晰**
   - Activity销毁时自动清理
   - 避免内存泄漏
   - 支持配置变更(旋转等)

#### 实现路线图

##### 阶段1:创建AndroidWindow

```cpp
// include/tinalux/platform/android/AndroidWindow.h
#pragma once

#include "tinalux/platform/Window.h"
#include <android/native_window.h>
#include <EGL/egl.h>
#include <jni.h>

namespace tinalux::platform {

class AndroidWindow : public Window {
public:
    AndroidWindow(ANativeWindow* nativeWindow, 
                  JNIEnv* env, 
                  jobject activity);
    ~AndroidWindow() override;
    
    // Window接口实现
    bool shouldClose() const override;
    void pollEvents() override;
    void waitEventsTimeout(double timeoutSeconds) override;
    void swapBuffers() override;
    void requestClose() override;
    
    int width() const override;
    int height() const override;
    int framebufferWidth() const override;
    int framebufferHeight() const override;
    float dpiScale() const override;
    
    void setClipboardText(const std::string& text) override;
    std::string clipboardText() const override;
    void setTextInputCursorRect(const std::optional<core::Rect>& rect) override;
    
    void setEventCallback(EventCallback callback) override;
    GLGetProcFn glGetProcAddress() const override;
    
    // Android生命周期
    void onSurfaceCreated();
    void onSurfaceChanged(int width, int height);
    void onSurfaceDestroyed();
    
    // Android事件
    void onTouchEvent(int action, float x, float y);
    void onKeyEvent(int action, int keyCode);
    
private:
    void initEGL();
    void destroyEGL();
    
    ANativeWindow* nativeWindow_;
    JNIEnv* env_;
    jobject activity_;
    
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLConfig config_;
    
    int width_;
    int height_;
    float dpiScale_;
    bool shouldClose_;
    
    EventCallback eventCallback_;
};

} // namespace tinalux::platform
```

##### 阶段2:JNI绑定层

```kotlin
// Android Kotlin层
class TinaluxActivity : AppCompatActivity() {
    private lateinit var surfaceView: GLSurfaceView
    private var nativeHandle: Long = 0
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        surfaceView = GLSurfaceView(this).apply {
            setEGLContextClientVersion(3)
            setRenderer(TinaluxRenderer())
        }
        setContentView(surfaceView)
        
        // 初始化native层
        nativeHandle = nativeCreate(surface)
    }
    
    override fun onDestroy() {
        super.onDestroy()
        nativeDestroy(nativeHandle)
    }
    
    override fun onTouchEvent(event: MotionEvent): Boolean {
        nativeTouchEvent(nativeHandle, event.action, event.x, event.y)
        return true
    }
    
    private external fun nativeCreate(surface: Surface): Long
    private external fun nativeDestroy(handle: Long)
    private external fun nativeTouchEvent(handle: Long, action: Int, x: Float, y: Float)
    
    companion object {
        init {
            System.loadLibrary("tinalux")
        }
    }
}
```

```cpp
// JNI实现
extern "C" {

JNIEXPORT jlong JNICALL
Java_com_tinalux_TinaluxActivity_nativeCreate(
    JNIEnv* env, jobject thiz, jobject surface) {
    
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    
    auto androidWindow = std::make_unique<AndroidWindow>(window, env, thiz);
    auto app = std::make_unique<Application>();
    
    WindowConfig config;
    config.width = androidWindow->width();
    config.height = androidWindow->height();
    
    app->init(config);
    app->setRootWidget(createMainUI());
    
    // 返回Application指针
    return reinterpret_cast<jlong>(app.release());
}

JNIEXPORT void JNICALL
Java_com_tinalux_TinaluxActivity_nativeDestroy(
    JNIEnv* env, jobject thiz, jlong handle) {
    
    auto* app = reinterpret_cast<Application*>(handle);
    app->shutdown();
    delete app;
}

JNIEXPORT void JNICALL
Java_com_tinalux_TinaluxActivity_nativeTouchEvent(
    JNIEnv* env, jobject thiz, jlong handle, 
    jint action, jfloat x, jfloat y) {
    
    auto* app = reinterpret_cast<Application*>(handle);
    // 转换为Tinalux事件并分发
    // ...
}

} // extern "C"
```

##### 阶段3:多Activity示例

```kotlin
// MainActivity.kt
class MainActivity : TinaluxActivity() {
    override fun createUI(): Long {
        return nativeCreateMainUI()
    }
    
    fun navigateToSettings() {
        startActivity(Intent(this, SettingsActivity::class.java))
    }
}

// SettingsActivity.kt
class SettingsActivity : TinaluxActivity() {
    override fun createUI(): Long {
        return nativeCreateSettingsUI()
    }
}
```

```cpp
// C++层UI创建
extern "C" {

JNIEXPORT jlong JNICALL
Java_com_tinalux_MainActivity_nativeCreateMainUI(JNIEnv* env, jobject thiz) {
    auto root = std::make_shared<ui::Container>();
    
    auto button = std::make_shared<ui::Button>("Go to Settings");
    button->setOnClick([env, thiz]() {
        // 调用Java方法跳转
        jclass cls = env->GetObjectClass(thiz);
        jmethodID mid = env->GetMethodID(cls, "navigateToSettings", "()V");
        env->CallVoidMethod(thiz, mid);
    });
    
    root->addChild(button);
    return reinterpret_cast<jlong>(root.release());
}

JNIEXPORT jlong JNICALL
Java_com_tinalux_SettingsActivity_nativeCreateSettingsUI(
    JNIEnv* env, jobject thiz) {
    
    auto root = std::make_shared<ui::Container>();
    // 创建设置界面...
    return reinterpret_cast<jlong>(root.release());
}

} // extern "C"
```

#### 性能优化建议

1. **EGL上下文共享**
```cpp
// 在多个Activity间共享纹理等资源
class EGLContextManager {
public:
    static EGLContext getSharedContext() {
        static EGLContext shared = createSharedContext();
        return shared;
    }
};

// AndroidWindow创建时使用共享上下文
context_ = eglCreateContext(
    display_, 
    config_, 
    EGLContextManager::getSharedContext(),  // 共享上下文
    contextAttribs
);
```

2. **资源预加载**
```cpp
// 在Application启动时预加载常用资源
class ResourceCache {
public:
    static void preload() {
        // 预加载字体
        ResourceLoader::instance().loadFont("default.ttf");
        // 预加载常用图片
        ResourceLoader::instance().loadImage("icon.png");
    }
};
```

3. **Activity转场优化**
```kotlin
// 使用共享元素转场
val options = ActivityOptions.makeSceneTransitionAnimation(
    this,
    sharedElement,
    "transition_name"
)
startActivity(intent, options.toBundle())
```

## 对比总结

| 特性 | 方案一(多Activity) | 方案二(多UIContext) | 方案三(切换Widget) |
|------|-------------------|--------------------|--------------------|
| 架构复杂度 | ⭐⭐ 低 | ⭐⭐⭐⭐⭐ 高 | ⭐ 很低 |
| 符合Android规范 | ✅ 完全符合 | ⚠️ 部分符合 | ❌ 不符合 |
| 内存管理 | ✅ 自动管理 | ⚠️ 需手动管理 | ⚠️ 需手动管理 |
| 界面切换性能 | ⚠️ 中等 | ✅ 快 | ✅ 很快 |
| 代码改动量 | ⭐⭐ 小 | ⭐⭐⭐⭐⭐ 大 | ⭐ 很小 |
| 维护成本 | ⭐⭐ 低 | ⭐⭐⭐⭐ 高 | ⭐⭐⭐ 中 |
| 推荐度 | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |

## 结论

**强烈推荐使用方案一:每个Activity一个Application实例**

理由:
1. 符合Android平台最佳实践
2. 与Tinalux现有架构完美契合
3. 代码改动量小,风险低
4. 资源管理清晰,不易出错
5. 支持Android所有特性(返回键、任务切换、生命周期等)

如果有特殊需求(如需要非常快速的界面切换),可以考虑在方案一的基础上:
- 使用EGL上下文共享优化资源加载
- 使用Fragment在单个Activity内管理子界面
- 结合方案三在某些场景下切换Widget

## 下一步行动

1. 实现`AndroidWindow`类
2. 创建JNI绑定层
3. 编写示例Activity
4. 性能测试和优化
5. 编写Android平台移植文档
