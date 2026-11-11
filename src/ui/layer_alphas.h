#pragma once
// The opacities of the main window's background layers over a wallpaper or
// a window material (ui_renderer.cpp paints them). Standalone so the
// self test can check the numbers.
#include <algorithm>

namespace pulse::ui {

// Opacity of the three window layers: title strip, the sheet shared by the
// active tab/toolbar/sidebar/status bar, and the pane/details cards. The
// active tab and the sheet must use the same value so they read as one.
struct LayerAlphas {
    float title = 1.0f, sheet = 1.0f, card = 1.0f;
    // DWM material: a pane card replaces what is under it (copy blend)
    // with this opacity, so the sidebar's setting does not show through
    // it; < 0 when the card blends over the sheet with `card` instead.
    float pane = -1.0f;
    // The colour of that card: the mix the three stacked layers give at
    // this interface transparency (weights of title / sheet / card).
    float pane_mix[3] = {0.0f, 0.0f, 1.0f};
};
// Piecewise-linear lookup at t = 0, .25, .5, .75, 1.
inline float LerpStops(float t, const float (&v)[5]) noexcept {
    t = std::clamp(t, 0.0f, 1.0f) * 4.0f;
    const int i = (std::min)(3, static_cast<int>(t));
    return v[i] + (v[i + 1] - v[i]) * (t - static_cast<float>(i));
}
// Acrylic/Mica transparency: 0..100 slider; 25/50/75 reproduce the former
// subtle/balanced/vivid surface tables exactly.
// frame: 侧边栏与标题栏 0..100 (AppPrefs::frame_transparency), only for
// Acrylic / Mica: 10 gives what the three layers gave before it existed.
inline float WallpaperImageOpacity(int transparency) noexcept {
    return static_cast<float>(std::clamp(transparency, 0, 100)) / 100.0f;
}

inline LayerAlphas ComputeLayerAlphas(bool image_mode, bool backdrop_drawn,
                                      bool backdrop_enabled, bool dark,
                                      int transparency, [[maybe_unused]] int wallpaper_blur,
                                      int frame = 10) noexcept {
    LayerAlphas a;
    const float t = static_cast<float>(std::clamp(transparency, 0, 100)) / 100.0f;
    if (image_mode) {
        // Decode failure stays opaque rather than exposing the desktop.
        if (!backdrop_drawn) return a;
        return {0.0f, 0.0f, 0.0f};
    }
    if (backdrop_drawn) a.title = 0.0f;
    else if (backdrop_enabled) a.title = dark ? 0.72f : 0.78f;
    if (backdrop_enabled) {
        // Acrylic / Mica: the slider scales the default (t = .5) opacities.
        static constexpr float kScale[5] = {1.40f, 1.20f, 1.0f, 0.80f, 0.60f};
        const auto stacked = [&](float at, float& sheet, float& card) {
            const float k = LerpStops(at, kScale);
            sheet = (std::min)(1.0f, (dark ? 0.55f : 0.60f) * k);
            card = (std::min)(1.0f, (dark ? 0.72f : 0.78f) * k);
            return 1.0f - (1.0f - a.title) * (1.0f - sheet) * (1.0f - card);
        };
        const float t0 = a.title;
        float s0 = 1.0f, c0 = 1.0f;
        // The content: what the layers gave up to 60, then on to 80% at 100.
        float pane = stacked(t, s0, c0);
        if (t > 0.6f) {
            float s60 = 1.0f, c60 = 1.0f;
            const float p60 = stacked(0.6f, s60, c60);
            pane = p60 + (0.80f - p60) * (t - 0.6f) / 0.4f;
        }
        a.pane_mix[0] = (1.0f - c0) * (1.0f - s0) * t0;
        a.pane_mix[1] = (1.0f - c0) * s0;
        a.pane_mix[2] = c0;
        // The frame (title strip over the whole window, sheet below it):
        // 10 is the old look, 0 solid; above 10 the strip clears to the
        // plain material and the sheet thins to 30%, so the active tab,
        // which is the sheet's colour, still stands out.
        const float base = 1.0f - (1.0f - t0) * (1.0f - s0);
        const float floor_sheet = (std::min)(0.30f, base);
        const int v = std::clamp(frame, 0, 100);
        float side = base;
        if (v <= 10) {
            // Both layers go solid together, as with no window effect, so the
            // active tab (the sheet's colour) still stands out at 0.
            const float f = static_cast<float>(v) / 10.0f;
            a.title = 1.0f + (t0 - 1.0f) * f;
            a.sheet = 1.0f + (s0 - 1.0f) * f;
            side = 1.0f - (1.0f - a.title) * (1.0f - a.sheet);
        } else {
            const float f = static_cast<float>(v - 10) / 90.0f;
            side = base - (base - floor_sheet) * f;
            a.title = t0 * (1.0f - f);
            a.sheet = std::clamp(1.0f - (1.0f - side) / (1.0f - a.title), 0.0f, 1.0f);
        }
        // Cards that stay on the sheet (Settings, the AI panel) keep the
        // content's opacity where the sheet below them allows it; below 10
        // they go solid with the sheet rather than vanish into it.
        const auto top_up = [&](float under) {
            return under >= 1.0f ? 1.0f : std::clamp(1.0f - (1.0f - pane) / (1.0f - under), 0.0f, 1.0f);
        };
        a.card = v <= 10 ? 1.0f + (top_up(base) - 1.0f) * static_cast<float>(v) / 10.0f : top_up(side);
        // Pane cards cut through when DWM draws the material (nothing of
        // Pulse's own is under them to lose).
        if (!backdrop_drawn) a.pane = pane;
    }
    return a;
}

} // namespace pulse::ui
