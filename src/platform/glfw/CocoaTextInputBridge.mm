#include "CocoaTextInputBridge.h"

#if defined(__APPLE__)

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include "GLFWWindow.h"
#include "tinalux/core/Log.h"

namespace tinalux::platform {

namespace {

using FirstRectForCharacterRangeFn = NSRect (*)(id, SEL, NSRange, NSRangePointer);
using InsertTextFn = void (*)(id, SEL, id, NSRange);
using SelectedRangeFn = NSRange (*)(id, SEL);
using SetMarkedTextFn = void (*)(id, SEL, id, NSRange, NSRange);
using UnmarkTextFn = void (*)(id, SEL);

FirstRectForCharacterRangeFn gOriginalFirstRectForCharacterRange = nullptr;
InsertTextFn gOriginalInsertText = nullptr;
SelectedRangeFn gOriginalSelectedRange = nullptr;
SetMarkedTextFn gOriginalSetMarkedText = nullptr;
UnmarkTextFn gOriginalUnmarkText = nullptr;
void* kBridgeAssociationKey = &kBridgeAssociationKey;

std::size_t utf8ByteOffsetForNSStringPrefix(NSString* text, NSUInteger utf16Length)
{
    if (text == nil) {
        return 0;
    }

    const NSUInteger clampedLength = std::min<NSUInteger>(utf16Length, [text length]);
    NSString* prefix = [text substringToIndex:clampedLength];
    return static_cast<std::size_t>([prefix lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
}

std::string utf8FromNSString(id text)
{
    NSString* string = nil;
    if ([text isKindOfClass:[NSAttributedString class]]) {
        string = [(NSAttributedString*)text string];
    } else if ([text isKindOfClass:[NSString class]]) {
        string = (NSString*)text;
    }

    if (string == nil) {
        return {};
    }

    const char* utf8 = [string UTF8String];
    return utf8 != nullptr ? std::string(utf8) : std::string {};
}

NSString* stringFromMarkedTextObject(id text)
{
    if ([text isKindOfClass:[NSAttributedString class]]) {
        return [(NSAttributedString*)text string];
    }
    if ([text isKindOfClass:[NSString class]]) {
        return (NSString*)text;
    }
    return nil;
}

class CocoaTextInputBridgeImpl final : public CocoaTextInputBridge {
public:
    CocoaTextInputBridgeImpl(GLFWwindow* window, GLFWWindow& owner, NSWindow* cocoaWindow, NSView* contentView)
        : window_(window)
        , owner_(owner)
        , cocoaWindow_(cocoaWindow)
        , contentView_(contentView)
    {
        installHooks([contentView_ class]);
        objc_setAssociatedObject(contentView_, kBridgeAssociationKey, this, OBJC_ASSOCIATION_ASSIGN);
    }

    ~CocoaTextInputBridgeImpl() override
    {
        if (contentView_ != nil) {
            objc_setAssociatedObject(contentView_, kBridgeAssociationKey, nil, OBJC_ASSOCIATION_ASSIGN);
        }
    }

    void setActive(bool active, const std::optional<core::Rect>& cursorRect, float dpiScale) override
    {
        active_ = active;
        cursorRect_ = cursorRect;
        dpiScale_ = dpiScale;

        if (contentView_ == nil) {
            return;
        }

        NSTextInputContext* inputContext = [contentView_ inputContext];
        if (inputContext == nil) {
            return;
        }

        if (active_) {
            if (cocoaWindow_ != nil && [cocoaWindow_ firstResponder] != contentView_) {
                [cocoaWindow_ makeFirstResponder:contentView_];
            }
            [inputContext activate];
            [inputContext invalidateCharacterCoordinates];
            return;
        }

        if (markedTextActive_) {
            owner_.dispatchPlatformCompositionEnd();
            markedTextActive_ = false;
        }
        selectedRange_ = NSMakeRange(NSNotFound, 0);

        [inputContext discardMarkedText];
        [inputContext deactivate];
    }

    void setCursorRect(const std::optional<core::Rect>& cursorRect, float dpiScale) override
    {
        cursorRect_ = cursorRect;
        dpiScale_ = dpiScale;

        if (!active_ || contentView_ == nil) {
            return;
        }

        NSTextInputContext* inputContext = [contentView_ inputContext];
        if (inputContext != nil) {
            [inputContext invalidateCharacterCoordinates];
        }
    }

    std::optional<NSRect> firstRectForCharacterRange(NSRange range, NSRangePointer actualRange) const
    {
        if (!active_ || !cursorRect_.has_value() || contentView_ == nil || cocoaWindow_ == nil) {
            return std::nullopt;
        }

        if (actualRange != nullptr) {
            *actualRange = range;
        }

        const CGFloat scale = dpiScale_ > 0.0f ? static_cast<CGFloat>(dpiScale_) : 1.0;
        const NSRect bounds = [contentView_ bounds];
        const CGFloat x = static_cast<CGFloat>(cursorRect_->left() / scale);
        const CGFloat y = bounds.size.height - static_cast<CGFloat>(cursorRect_->bottom() / scale);
        const CGFloat width = std::max<CGFloat>(1.0, static_cast<CGFloat>(cursorRect_->width() / scale));
        const CGFloat height = std::max<CGFloat>(1.0, static_cast<CGFloat>(cursorRect_->height() / scale));

        const NSRect localRect = NSMakeRect(x, y, width, height);
        const NSRect windowRect = [contentView_ convertRect:localRect toView:nil];
        return [cocoaWindow_ convertRectToScreen:windowRect];
    }

    void handleSetMarkedText(id text, NSRange selectedRange)
    {
        if (!active_) {
            return;
        }

        NSString* string = stringFromMarkedTextObject(text);
        if (string == nil) {
            return;
        }

        const std::string utf8 = utf8FromNSString(string);
        if (!markedTextActive_) {
            owner_.dispatchPlatformCompositionStart();
            markedTextActive_ = true;
        }

        std::optional<std::size_t> caretUtf8Offset = std::nullopt;
        std::optional<std::size_t> targetStartUtf8 = std::nullopt;
        std::optional<std::size_t> targetEndUtf8 = std::nullopt;
        selectedRange_ = NSMakeRange(NSNotFound, 0);

        if (selectedRange.location != NSNotFound) {
            const NSUInteger selectionStart = std::min<NSUInteger>(selectedRange.location, [string length]);
            const NSUInteger selectionEnd = std::min<NSUInteger>(selectionStart + selectedRange.length, [string length]);
            const std::size_t startUtf8 = utf8ByteOffsetForNSStringPrefix(string, selectionStart);
            const std::size_t endUtf8 = utf8ByteOffsetForNSStringPrefix(string, selectionEnd);
            selectedRange_ = NSMakeRange(selectionStart, selectionEnd - selectionStart);
            caretUtf8Offset = startUtf8;
            if (selectionEnd > selectionStart) {
                targetStartUtf8 = startUtf8;
                targetEndUtf8 = endUtf8;
            }
        } else {
            selectedRange_ = NSMakeRange([string length], 0);
            caretUtf8Offset = utf8.size();
        }

        owner_.dispatchPlatformCompositionUpdate(
            utf8,
            std::move(caretUtf8Offset),
            std::move(targetStartUtf8),
            std::move(targetEndUtf8));

        NSTextInputContext* inputContext = [contentView_ inputContext];
        if (inputContext != nil) {
            [inputContext invalidateCharacterCoordinates];
        }
    }

    void handleInsertText()
    {
        if (!markedTextActive_) {
            return;
        }

        owner_.dispatchPlatformCompositionEnd();
        markedTextActive_ = false;
        selectedRange_ = NSMakeRange(NSNotFound, 0);

        NSTextInputContext* inputContext = [contentView_ inputContext];
        if (inputContext != nil) {
            [inputContext invalidateCharacterCoordinates];
        }
    }

    void handleUnmarkText()
    {
        if (!markedTextActive_) {
            selectedRange_ = NSMakeRange(NSNotFound, 0);
            return;
        }

        owner_.dispatchPlatformCompositionEnd();
        markedTextActive_ = false;
        selectedRange_ = NSMakeRange(NSNotFound, 0);
    }

    std::optional<NSRange> selectedRange() const
    {
        if (!active_ || !markedTextActive_) {
            return std::nullopt;
        }

        return selectedRange_;
    }

private:
    static CocoaTextInputBridgeImpl* bridgeFromView(id view)
    {
        return reinterpret_cast<CocoaTextInputBridgeImpl*>(
            objc_getAssociatedObject(view, kBridgeAssociationKey));
    }

    static void installHooks(Class contentViewClass)
    {
        static bool installed = false;
        if (installed || contentViewClass == Nil) {
            return;
        }

        Method firstRectMethod = class_getInstanceMethod(
            contentViewClass,
            @selector(firstRectForCharacterRange:actualRange:));
        if (firstRectMethod != nullptr) {
            gOriginalFirstRectForCharacterRange =
                reinterpret_cast<FirstRectForCharacterRangeFn>(method_getImplementation(firstRectMethod));
            method_setImplementation(firstRectMethod, reinterpret_cast<IMP>(&swizzledFirstRectForCharacterRange));
        }

        Method insertTextMethod = class_getInstanceMethod(
            contentViewClass,
            @selector(insertText:replacementRange:));
        if (insertTextMethod != nullptr) {
            gOriginalInsertText =
                reinterpret_cast<InsertTextFn>(method_getImplementation(insertTextMethod));
            method_setImplementation(insertTextMethod, reinterpret_cast<IMP>(&swizzledInsertText));
        }

        Method selectedRangeMethod = class_getInstanceMethod(contentViewClass, @selector(selectedRange));
        if (selectedRangeMethod != nullptr) {
            gOriginalSelectedRange =
                reinterpret_cast<SelectedRangeFn>(method_getImplementation(selectedRangeMethod));
            method_setImplementation(selectedRangeMethod, reinterpret_cast<IMP>(&swizzledSelectedRange));
        }

        Method setMarkedTextMethod = class_getInstanceMethod(
            contentViewClass,
            @selector(setMarkedText:selectedRange:replacementRange:));
        if (setMarkedTextMethod != nullptr) {
            gOriginalSetMarkedText =
                reinterpret_cast<SetMarkedTextFn>(method_getImplementation(setMarkedTextMethod));
            method_setImplementation(setMarkedTextMethod, reinterpret_cast<IMP>(&swizzledSetMarkedText));
        }

        Method unmarkTextMethod = class_getInstanceMethod(contentViewClass, @selector(unmarkText));
        if (unmarkTextMethod != nullptr) {
            gOriginalUnmarkText =
                reinterpret_cast<UnmarkTextFn>(method_getImplementation(unmarkTextMethod));
            method_setImplementation(unmarkTextMethod, reinterpret_cast<IMP>(&swizzledUnmarkText));
        }

        installed = true;
    }

    static NSRect swizzledFirstRectForCharacterRange(
        id self,
        SEL _cmd,
        NSRange range,
        NSRangePointer actualRange)
    {
        if (CocoaTextInputBridgeImpl* bridge = bridgeFromView(self); bridge != nullptr) {
            if (const auto rect = bridge->firstRectForCharacterRange(range, actualRange)) {
                return *rect;
            }
        }

        if (gOriginalFirstRectForCharacterRange != nullptr) {
            return gOriginalFirstRectForCharacterRange(self, _cmd, range, actualRange);
        }

        return NSMakeRect(0.0, 0.0, 0.0, 0.0);
    }

    static void swizzledInsertText(id self, SEL _cmd, id string, NSRange replacementRange)
    {
        if (gOriginalInsertText != nullptr) {
            gOriginalInsertText(self, _cmd, string, replacementRange);
        }

        if (CocoaTextInputBridgeImpl* bridge = bridgeFromView(self); bridge != nullptr) {
            bridge->handleInsertText();
        }
    }

    static NSRange swizzledSelectedRange(id self, SEL _cmd)
    {
        if (CocoaTextInputBridgeImpl* bridge = bridgeFromView(self); bridge != nullptr) {
            if (const auto range = bridge->selectedRange()) {
                return *range;
            }
        }

        if (gOriginalSelectedRange != nullptr) {
            return gOriginalSelectedRange(self, _cmd);
        }

        return NSMakeRange(NSNotFound, 0);
    }

    static void swizzledSetMarkedText(
        id self,
        SEL _cmd,
        id string,
        NSRange selectedRange,
        NSRange replacementRange)
    {
        if (gOriginalSetMarkedText != nullptr) {
            gOriginalSetMarkedText(self, _cmd, string, selectedRange, replacementRange);
        }

        if (CocoaTextInputBridgeImpl* bridge = bridgeFromView(self); bridge != nullptr) {
            bridge->handleSetMarkedText(string, selectedRange);
        }
    }

    static void swizzledUnmarkText(id self, SEL _cmd)
    {
        if (gOriginalUnmarkText != nullptr) {
            gOriginalUnmarkText(self, _cmd);
        }

        if (CocoaTextInputBridgeImpl* bridge = bridgeFromView(self); bridge != nullptr) {
            bridge->handleUnmarkText();
        }
    }

    GLFWwindow* window_ = nullptr;
    GLFWWindow& owner_;
    NSWindow* cocoaWindow_ = nil;
    NSView* contentView_ = nil;
    bool active_ = false;
    bool markedTextActive_ = false;
    float dpiScale_ = 1.0f;
    NSRange selectedRange_ = NSMakeRange(NSNotFound, 0);
    std::optional<core::Rect> cursorRect_;
};

}  // namespace

CocoaTextInputBridge::~CocoaTextInputBridge() = default;

std::unique_ptr<CocoaTextInputBridge> CocoaTextInputBridge::create(GLFWwindow* window, GLFWWindow& owner)
{
    if (window == nullptr) {
        return nullptr;
    }

    NSWindow* cocoaWindow = glfwGetCocoaWindow(window);
    if (cocoaWindow == nil) {
        core::logErrorCat("platform", "Failed to get Cocoa window from GLFW");
        return nullptr;
    }

    NSView* contentView = [cocoaWindow contentView];
    if (contentView == nil) {
        core::logErrorCat("platform", "Failed to get Cocoa content view from GLFW window");
        return nullptr;
    }

    return std::make_unique<CocoaTextInputBridgeImpl>(window, owner, cocoaWindow, contentView);
}

}  // namespace tinalux::platform

#endif
