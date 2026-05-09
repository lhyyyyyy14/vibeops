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
  const LayoutMetrics &layout = *ctx.layout;
  ClearScreen(ctx.renderer, SDL_Color{17, 22, 31, 255});
  DrawText(ctx.renderer, layout.safe_margin_x, 22, "GB Internovel Home", 4, SDL_Color{235, 238, 245, 255});
  DrawPanel(ctx.renderer, SDL_Rect{40, 108, 640, 230}, SDL_Color{28, 35, 48, 255},
            SDL_Color{72, 86, 112, 255});

  const SDL_Color active{116, 200, 184, 255};
  const SDL_Color idle{188, 198, 215, 255};
  DrawText(ctx.renderer, 92, 140, selected_ == 0 ? "> Story Session" : "  Story Session", 4,
           selected_ == 0 ? active : idle);
  DrawText(ctx.renderer, 92, 202, selected_ == 1 ? "> History Data" : "  History Data", 4,
           selected_ == 1 ? active : idle);
  DrawText(ctx.renderer, 92, 264, selected_ == 2 ? "> Settings" : "  Settings", 4, selected_ == 2 ? active : idle);
  DrawFooterHint(ctx.renderer, "上下移动  Enter/A 确认  Esc/B 取消");
}
