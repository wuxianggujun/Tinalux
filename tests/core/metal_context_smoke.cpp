#include <cstdlib>
#include <iostream>

#include "tinalux/rendering/rendering.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

}  // namespace

int main()
{
    using namespace tinalux::rendering;

#if defined(__APPLE__)
    RenderContext metalContext = createContext(ContextConfig { .backend = Backend::Metal });
    expect(metalContext, "Metal smoke should create a Metal direct context");
    expect(metalContext.backend() == Backend::Metal, "Metal smoke should report Metal backend");

    RenderContext autoContext = createContext(ContextConfig { .backend = Backend::Auto });
    expect(autoContext, "Auto backend smoke should create a fallback context on Apple platforms");
    expect(
        autoContext.backend() == Backend::Metal,
        "Auto backend smoke should prioritize Metal on Apple platforms");
#else
    RenderContext metalContext = createContext(ContextConfig { .backend = Backend::Metal });
    expect(!metalContext, "Metal smoke should fail cleanly on non-Apple platforms");

    RenderContext autoContext = createContext(ContextConfig { .backend = Backend::Auto });
    expect(
        !autoContext || autoContext.backend() != Backend::Metal,
        "Auto backend smoke should never report Metal on non-Apple platforms");
#endif

    return 0;
}
