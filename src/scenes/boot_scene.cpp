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
  DrawAppShell(ctx.renderer, "GB Internovel", "PRESS START", "READY");

  DrawSectionPanel(ctx.renderer, SDL_Rect{96, 106, 528, 222}, "");
  DrawText(ctx.renderer, 156, 152, "GB Internovel", 5, UiDark());
  DrawText(ctx.renderer, 170, 226, "复古掌机互动小说", 3, UiAccent());
  DrawText(ctx.renderer, 180, 282, "按 Enter / Space / Z 进入首页", 2, UiMuted());

  DrawFooterButtons(ctx.renderer, {{"A", "START"}, {"B", "BACK"}, {"UP/DN", "MOVE"}});
}
