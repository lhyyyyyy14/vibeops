#include "scenes/boot_scene.h"

#include "ui_draw.h"

void BootScene::OnEnter() { elapsed_ = 0.0f; }

void BootScene::Update(float dt, const InputManager &input, SceneManager &scenes) {
  elapsed_ += dt;
  if (elapsed_ >= 1.0f || input.IsJustPressed(Button::A) || input.IsJustPressed(Button::Start)) {
    scenes.Set(AppScene::Home);
  }
}

void BootScene::Render(AppContext &ctx) {
  ClearScreen(ctx.renderer, SDL_Color{13, 16, 23, 255});
  DrawText(ctx.renderer, 96, 130, "GB Internovel", 5, SDL_Color{232, 238, 248, 255});
  DrawText(ctx.renderer, 132, 206, "复古掌机互动小说壳", 3, SDL_Color{116, 200, 184, 255});
  DrawText(ctx.renderer, 132, 260, "按 回车 / 空格 / Z 进入首页", 3, SDL_Color{188, 198, 215, 255});
  DrawFooterHint(ctx.renderer, "启动页：SDL2 + C++17 骨架");
}
