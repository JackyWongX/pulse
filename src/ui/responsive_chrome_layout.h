#pragma once
#include <d2d1.h>
#include <algorithm>
#include <array>

namespace pulse::ui {
enum class NavigationTier { Full, Compact, Narrow, Merged };
struct ResponsiveNavigation {
    std::array<D2D1_RECT_F, 4> navigation{};
    D2D1_RECT_F address{}, search{}, address_text{}, rows_toggle{};
    NavigationTier tier = NavigationTier::Full;
    bool refresh_inside = false, merged = false;
    int breadcrumb_limit = 99;
};
inline ResponsiveNavigation MakeResponsiveNavigation(float width, float left, float top,
    float margin, float scale, bool can_forward, float search_min_dip, bool show_rows_toggle = false) {
    ResponsiveNavigation out;
    const float available = (width - left) / scale;
    out.tier = available >= 760 ? NavigationTier::Full : available >= 600 ? NavigationTier::Compact :
        available >= 460 ? NavigationTier::Narrow : NavigationTier::Merged;
    out.refresh_inside = out.tier != NavigationTier::Full;
    out.merged = out.tier == NavigationTier::Merged;
    out.breadcrumb_limit = out.merged ? 1 : out.tier == NavigationTier::Narrow ? 2 : 99;
    const bool hidden = search_min_dip < 0, editing = search_min_dip >= 500;
    const float step = (available < 500 ? 26.0f : 34.0f) * scale;
    const float toggle_width = show_rows_toggle ? 32.0f * scale : 0.0f;
    const float toggle_top = top + 6.0f * scale, toggle_bottom = top + 38.0f * scale;
    auto place_toggle = [&](float right_edge) {
        if (toggle_width > 0.0f)
            out.rows_toggle = {right_edge - toggle_width, toggle_top, right_edge, toggle_bottom};
    };
    float x = left + margin;
    for (int i = 0; i < 3; ++i) {
        if (i == 1 && !can_forward && available < 600) continue;
        out.navigation[i] = {x, top + 6 * scale, x + step - 4 * scale, top + 38 * scale};
        x += step;
    }
    const float field_left = x + margin, right = width - margin;
    const float y = top + 4 * scale, bottom = top + 40 * scale;
    out.address = {field_left, y, right, bottom};
    out.search = {right, y, right, bottom};
    if (out.merged && editing && !hidden) {
        out.search = out.address;
        if (toggle_width > 0.0f) {
            place_toggle(right);
            out.search.right = out.rows_toggle.left - margin;
        }
        out.address.right = out.address.left;
        out.address_text = out.address;
        return out;
    }
    if (!hidden && !out.merged) {
        float search_width = out.tier == NavigationTier::Narrow ? 112 * scale :
            out.tier == NavigationTier::Compact ? std::clamp(available * 0.27f, 180.0f, 260.0f) * scale :
            std::clamp(available * 0.27f, 140.0f, 360.0f) * scale;
        if (search_min_dip > 0)
            search_width = (std::max)(search_width, (std::min)(search_min_dip * scale,
                right - field_left - 120 * scale - margin - (out.refresh_inside ? 0 : step)));
        out.search.left = right - search_width;
        out.address.right = out.search.left - margin;
    } else if (!hidden) {
        out.search.left = right - 32 * scale;
    }
    if (out.merged) {
        out.address.right = right;
        if (toggle_width > 0.0f)
            place_toggle(!hidden ? out.search.left - margin : right);
        const float refresh_right = toggle_width > 0.0f
            ? out.rows_toggle.left - margin : (!hidden ? out.search.left : out.address.right);
        out.navigation[3] = {refresh_right - 32 * scale, top + 6 * scale, refresh_right, top + 38 * scale};
        out.address_text = out.address;
        out.address_text.right = (std::max)(out.address_text.left, out.navigation[3].left - 4 * scale);
        return out;
    }

    const float field_right = !hidden ? out.search.left - margin : right;
    if (out.refresh_inside) {
        if (toggle_width > 0.0f) {
            place_toggle(field_right);
            out.address.right = out.rows_toggle.left - margin;
        } else {
            out.address.right = field_right;
        }
        out.navigation[3] = {out.address.right - 32 * scale, top + 6 * scale,
                             out.address.right, top + 38 * scale};
    } else {
        out.address.right = field_right;
        out.navigation[3] = {out.address.right - step, top + 6 * scale,
                             out.address.right - 4 * scale, top + 38 * scale};
        if (toggle_width > 0.0f) {
            place_toggle(out.navigation[3].left - margin);
            out.address.right = out.rows_toggle.left - margin;
        } else {
            out.address.right = out.navigation[3].left - margin;
        }
    }
    out.address_text = out.address;
    if (out.refresh_inside) out.address_text.right = out.navigation[3].left - 4 * scale;
    out.address_text.right = (std::max)(out.address_text.left, out.address_text.right);
    return out;
}
}
