#include "app_config.h"
#include "app_context.h"
#include "input_manager.h"
#include "layout_metrics.h"
#include "scene_manager.h"
#include "scenes/boot_scene.h"
#include "scenes/history_scene.h"
#include "scenes/home_scene.h"
#include "scenes/session_scene.h"
#include "scenes/settings_scene.h"
#include "scenes/story_setup_scene.h"
#include "ui_draw.h"

#include <SDL.h>

#include <iostream>
#include <memory>

namespace {
Scene *SceneFor(AppScene scene, BootScene &boot, HomeScene &home, StorySetupScene &story_setup, HistoryScene &history,
                SettingsScene &settings, SessionScene &session) {
  switch (scene) {
  case AppScene::Boot: return &boot;
  case AppScene::Home: return &home;
  case AppScene::StorySetup: return &story_setup;
  case AppScene::History: return &history;
  case AppScene::Settings: return &settings;
  case AppScene::Session: return &session;
  }
  return &home;
}

void OpenGameControllers() {
  const int joysticks = SDL_NumJoysticks();
  for (int i = 0; i < joysticks; ++i) {
    if (SDL_IsGameController(i)) {
      SDL_GameControllerOpen(i);
    } else {
      SDL_JoystickOpen(i);
    }
  }
}
}  // namespace

int main(int, char **) {
  AppConfig config = LoadAppConfig("native_config.ini");
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
    return 1;
  }

  const LayoutMetrics &layout = Layout();
  Uint32 window_flags = SDL_WINDOW_SHOWN;
  if (config.fullscreen && !config.windowed) window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  SDL_Window *window = SDL_CreateWindow("GB Internovel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        layout.screen_w, layout.screen_h, window_flags);
  if (!window) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    SDL_Quit();
    return 2;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (!renderer) {
    std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 3;
  }
  SDL_RenderSetLogicalSize(renderer, layout.screen_w, layout.screen_h);
  InitUiText();

  OpenGameControllers();
  InputManager input;
  input.LoadOverrides("native_keymap.ini");
  AppContext ctx{window, renderer, &layout, config};

  SceneManager scenes;
  BootScene boot;
  HomeScene home;
  StorySetupScene story_setup;
  HistoryScene history;
  SettingsScene settings;
  SessionScene session;
  Scene *active = SceneFor(scenes.Current(), boot, home, story_setup, history, settings, session);
  active->OnEnter();

  bool running = true;
  Uint32 last_ticks = SDL_GetTicks();
  while (running) {
    const Uint32 now = SDL_GetTicks();
    const float dt = static_cast<float>(now - last_ticks) / 1000.0f;
    last_ticks = now;

    input.BeginFrame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
      input.HandleEvent(event);
    }

    const AppScene before = scenes.Current();
    active = SceneFor(before, boot, home, story_setup, history, settings, session);
    active->Update(dt, input, scenes);
    if (scenes.Current() != before) {
      active = SceneFor(scenes.Current(), boot, home, story_setup, history, settings, session);
      active->OnEnter();
    }
    active->Render(ctx);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  ShutdownUiText();
  SDL_Quit();
  return 0;
}
