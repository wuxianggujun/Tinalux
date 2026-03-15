#pragma once

namespace tinalux::ui {

struct Spacing {
    float xs = 4.0f;
    float sm = 8.0f;
    float md = 16.0f;
    float lg = 24.0f;
    float xl = 32.0f;
    float xxl = 48.0f;

    float radiusXs = 2.0f;
    float radiusSm = 4.0f;
    float radiusMd = 8.0f;
    float radiusLg = 12.0f;
    float radiusXl = 16.0f;

    static Spacing defaultSpacing();
};

}  // namespace tinalux::ui
