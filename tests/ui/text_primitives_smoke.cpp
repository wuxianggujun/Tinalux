#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../../src/rendering/FontCache.h"
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

    tinalux::rendering::clearCachedFonts();
    clearTextMetricsCache();
    expect(tinalux::rendering::cachedFontCount() == 0, "font cache should start empty after clear");
    expect(textMetricsCacheStats().entryCount == 0, "text metrics cache should start empty after clear");

    const TextMetrics emptyMetrics = measureTextMetrics("", 16.0f);
    expect(emptyMetrics.width >= 0.0f, "empty text width should stay non-negative");
    expect(emptyMetrics.height >= 0.0f, "empty text height should stay non-negative");
    expect(emptyMetrics.baseline >= 0.0f, "baseline should stay non-negative");
    expect(tinalux::rendering::cachedFontCount() == 1, "measuring text should populate one cached font");
    expect(textMetricsCacheStats().missCount == 1, "first text measurement should record a cache miss");

    const TextMetrics asciiMetrics = measureTextMetrics("Build", 18.0f);
    const TextMetrics utf8Metrics = measureTextMetrics("你好，Tinalux", 18.0f);
    expect(asciiMetrics.width > 0.0f, "ASCII text should produce positive width");
    expect(utf8Metrics.width > 0.0f, "UTF-8 text should produce positive width");
    expect(tinalux::rendering::cachedFontCount() == 2, "same font size should reuse cached font entry");

    const TextMetrics cachedAgain = measureTextMetrics("Build", 18.0f);
    expect(
        std::abs(asciiMetrics.width - cachedAgain.width) < 0.001f
            && std::abs(asciiMetrics.height - cachedAgain.height) < 0.001f
            && std::abs(asciiMetrics.baseline - cachedAgain.baseline) < 0.001f
            && std::abs(asciiMetrics.drawX - cachedAgain.drawX) < 0.001f,
        "repeated text measurement should stay stable");
    expect(tinalux::rendering::cachedFontCount() == 2, "repeated measurement should not add duplicate font entries");
    expect(textMetricsCacheStats().hitCount > 0, "repeated measurement should record a cache hit");

    clearTextMetricsCache();
    for (int index = 0; index < 512; ++index) {
        (void)measureTextMetrics("Entry " + std::to_string(index), 14.0f);
    }
    expect(textMetricsCacheStats().entryCount == 512, "text metrics cache should cap at 512 entries");

    const TextMetrics retainedMetrics = measureTextMetrics("Entry 0", 14.0f);
    const std::size_t hitsBeforeEviction = textMetricsCacheStats().hitCount;
    (void)measureTextMetrics("Entry overflow", 14.0f);
    expect(textMetricsCacheStats().entryCount == 512, "overflow insert should evict instead of growing cache");

    const TextMetrics retainedAgain = measureTextMetrics("Entry 0", 14.0f);
    expect(
        std::abs(retainedMetrics.width - retainedAgain.width) < 0.001f
            && std::abs(retainedMetrics.height - retainedAgain.height) < 0.001f,
        "recently reused cache entry should remain stable after eviction");
    expect(
        textMetricsCacheStats().hitCount > hitsBeforeEviction,
        "recently reused cache entry should survive LRU eviction");
    expect(textMetricsCacheStats().evictionCount > 0, "overflow insert should increment eviction count");

    return 0;
}
