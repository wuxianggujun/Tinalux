#include "tinalux/rendering/rendering.h"

#include "include/core/SkGraphics.h"

namespace tinalux::rendering {

void initSkia()
{
    SkGraphics::Init();
}

void shutdownSkia()
{
    SkGraphics::PurgeAllCaches();
}

void flushFrame(GrDirectContext* ctx)
{
    if (ctx == nullptr) {
        return;
    }

    ctx->flushAndSubmit();
}

}  // namespace tinalux::rendering
