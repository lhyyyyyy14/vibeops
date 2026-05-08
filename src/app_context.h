#pragma once

#include "app_config.h"
#include "layout_metrics.h"

#include <SDL.h>

struct AppContext {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  const LayoutMetrics *layout = nullptr;
  AppConfig config{};

  int ScreenWidth() const { return layout ? layout->screen_w : 720; }
  int ScreenHeight() const { return layout ? layout->screen_h : 480; }
};
