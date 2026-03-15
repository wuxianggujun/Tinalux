#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../../src/ui/TextPrimitives.h"

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
    using namespace tinalux::ui;

    const TextMetrics emptyMetrics = measureTextMetrics("", 16.0f);
    expect(emptyMetrics.width >= 0.0f, "empty text width should stay non-negative");
    expect(emptyMetrics.height >= 0.0f, "empty text height should stay non-negative");
    expect(emptyMetrics.baseline >= 0.0f, "baseline should stay non-negative");

    const TextMetrics asciiMetrics = measureTextMetrics("Build", 18.0f);
    const TextMetrics utf8Metrics = measureTextMetrics("你好，Tinalux", 18.0f);
    expect(asciiMetrics.width > 0.0f, "ASCII text should produce positive width");
    expect(utf8Metrics.width > 0.0f, "UTF-8 text should produce positive width");

    const TextMetrics cachedAgain = measureTextMetrics("Build", 18.0f);
    expect(
        std::abs(asciiMetrics.width - cachedAgain.width) < 0.001f
            && std::abs(asciiMetrics.height - cachedAgain.height) < 0.001f
            && std::abs(asciiMetrics.baseline - cachedAgain.baseline) < 0.001f
            && std::abs(asciiMetrics.drawX - cachedAgain.drawX) < 0.001f,
        "repeated text measurement should stay stable");

    return 0;
}
