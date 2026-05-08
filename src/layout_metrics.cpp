#include "layout_metrics.h"

namespace {
constexpr LayoutMetrics kLayout720x480{};
}

const LayoutMetrics &Layout() { return kLayout720x480; }
