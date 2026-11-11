#include "../ui/layer_alphas.h"

#include <cmath>
#include <cstdio>

namespace {
int failures = 0;

void Check(bool ok, const char* label) {
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", label);
    failures += !ok;
}

bool Close(float a, float b) {
    return std::fabs(a - b) < 1e-4f;
}
}

int main() {
    using pulse::ui::ComputeLayerAlphas;
    using pulse::ui::WallpaperImageOpacity;

    bool unmasked = true;
    bool opacity_tracks_slider = true;
    for (int dark = 0; dark <= 1; ++dark) {
        for (int transparency = 0; transparency <= 100; ++transparency) {
            opacity_tracks_slider =
                opacity_tracks_slider &&
                Close(WallpaperImageOpacity(transparency),
                      static_cast<float>(transparency) / 100.0f);
            for (int blur : {0, 14}) {
                const auto a = ComputeLayerAlphas(true, true, true, dark != 0, transparency, blur);
                unmasked = unmasked && Close(a.title, 0.0f) && Close(a.sheet, 0.0f) &&
                           Close(a.card, 0.0f);
            }
        }
    }
    Check(unmasked, "image backgrounds have no title, sheet, or card scrim at any setting");
    Check(opacity_tracks_slider && Close(WallpaperImageOpacity(-1), 0.0f) &&
              Close(WallpaperImageOpacity(101), 1.0f),
          "image opacity follows the 0-100 slider and clamps invalid values");

    return failures ? 1 : 0;
}
