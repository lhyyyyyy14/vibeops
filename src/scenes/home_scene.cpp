#include "scenes/home_scene.h"

#include "ui_draw.h"

void HomeScene::Update(float, const InputManager &input, SceneManager &scenes) {
  if (input.IsJustPressed(Button::Up)) selected_ = (selected_ + 2) % 3;
  if (input.IsJustPressed(Button::Down)) selected_ = (selected_ + 1) % 3;
  if (input.IsJustPressed(Button::A)) {
    if (selected_ == 0) scenes.Set(AppScene::StorySetup);
    if (selected_ == 1) scenes.Set(AppScene::History);
    if (selected_ == 2) scenes.Set(AppScene::Settings);
  }
}

void HomeScene::Render(AppContext &ctx) {
  DrawAppShell(ctx.renderer, "GB Internovel", "HOME", "READY");

  DrawSectionPanel(ctx.renderer, SDL_Rect{62, 92, 390, 284}, "主菜单");
  DrawMenuItem(ctx.renderer, SDL_Rect{92, 148, 312, 52}, "开始故事", "NEW", selected_ == 0);
  DrawMenuItem(ctx.renderer, SDL_Rect{92, 220, 312, 52}, "历史数据", "LOG", selected_ == 1);
  DrawMenuItem(ctx.renderer, SDL_Rect{92, 292, 312, 52}, "设置中心", "CFG", selected_ == 2);

  DrawSectionPanel(ctx.renderer, SDL_Rect{488, 92, 150, 284}, "状态");
  DrawText(ctx.renderer, 508, 142, "MODE", 2, UiAccent());
  DrawText(ctx.renderer, 508, 170, "AI NOVEL", 2, UiInk());
  DrawText(ctx.renderer, 508, 222, "INPUT", 2, UiAccent());
  DrawText(ctx.renderer, 508, 250, "D-PAD", 2, UiInk());
  DrawText(ctx.renderer, 508, 316, selected_ == 0 ? "NEW RUN" : selected_ == 1 ? "BROWSE" : "CONFIG", 2,
           UiGreen());

  DrawFooterButtons(ctx.renderer, {{"UP/DN", "MOVE"}, {"A", "OK"}, {"B", "BACK"}});
}
