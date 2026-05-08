#include "scenes/home_scene.h"

#include "ui_draw.h"

void HomeScene::Update(float, const InputManager &input, SceneManager &scenes) {
  if (input.IsJustPressed(Button::Up) || input.IsJustPressed(Button::Down)) selected_ = 1 - selected_;
  if (input.IsJustPressed(Button::Menu)) {
    scenes.Set(AppScene::Settings);
  }
  if (input.IsJustPressed(Button::A) || input.IsJustPressed(Button::Start)) {
    scenes.Set(selected_ == 0 ? AppScene::Session : AppScene::Settings);
  }
}

void HomeScene::Render(AppContext &ctx) {
  const LayoutMetrics &layout = *ctx.layout;
  ClearScreen(ctx.renderer, SDL_Color{17, 22, 31, 255});
  DrawText(ctx.renderer, layout.safe_margin_x, 22, "GB Internovel 首页", 4, SDL_Color{235, 238, 245, 255});
  DrawPanel(ctx.renderer, SDL_Rect{40, 108, 640, 230}, SDL_Color{28, 35, 48, 255},
            SDL_Color{72, 86, 112, 255});

  const SDL_Color active{116, 200, 184, 255};
  const SDL_Color idle{188, 198, 215, 255};
  DrawText(ctx.renderer, 92, 144, selected_ == 0 ? "> 会话演示" : "  会话演示", 4, selected_ == 0 ? active : idle);
  DrawText(ctx.renderer, 92, 210, selected_ == 1 ? "> 设置中心" : "  设置中心", 4, selected_ == 1 ? active : idle);
  DrawText(ctx.renderer, 92, 286, "当前功能：场景切换、掌机按键、Mock 业务层", 2, idle);
  DrawFooterHint(ctx.renderer, "方向键选择  回车/空格/Z 进入  Esc 设置");
}
