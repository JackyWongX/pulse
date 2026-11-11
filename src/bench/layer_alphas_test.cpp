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

    const auto light90 = ComputeLayerAlphas(true, true, true, false, 90, 0);
    const auto light99 = ComputeLayerAlphas(true, true, true, false, 99, 0);
    const auto light100 = ComputeLayerAlphas(true, true, true, false, 100, 0);
    const auto blurred100 = ComputeLayerAlphas(true, true, true, false, 100, 14);
    const auto dark90 = ComputeLayerAlphas(true, true, true, true, 90, 0);
    const auto dark100 = ComputeLayerAlphas(true, true, true, true, 100, 0);

    Check(Close(light90.title, 0.312f) && Close(light90.sheet, 0.352f) &&
              Close(light90.card, 0.168f) && Close(dark90.title, 0.31f),
          "the former 90 percent appearance is preserved");
    Check(light99.title > 0.0f && light99.sheet > 0.0f && light99.card > 0.0f &&
              light99.title < light90.title && light99.sheet < light90.sheet &&
              light99.card < light90.card && dark100.title == 0.0f,
          "image scrims fade out over the final ten percent in both themes");
    Check(light100.title == 0.0f && light100.sheet == 0.0f && light100.card == 0.0f &&
              blurred100.title == 0.0f && blurred100.sheet == 0.0f && blurred100.card == 0.0f,
          "100 percent shows the wallpaper without a title, sheet, or card scrim");

    return failures ? 1 : 0;
}
