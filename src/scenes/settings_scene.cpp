#include "scenes/settings_scene.h"

#include "ui_draw.h"

void SettingsScene::Update(float, const InputManager &input, SceneManager &scenes) {
  if (input.IsJustPressed(Button::B) || input.IsJustPressed(Button::Menu)) {
    scenes.Set(AppScene::Home);
  }
}

void SettingsScene::Render(AppContext &ctx) {
  ClearScreen(ctx.renderer, SDL_Color{18, 19, 25, 255});
  DrawText(ctx.renderer, 32, 24, "设置中心", 4, SDL_Color{235, 238, 245, 255});
  DrawPanel(ctx.renderer, SDL_Rect{38, 96, 644, 260}, SDL_Color{30, 33, 43, 255}, SDL_Color{76, 87, 112, 255});
  DrawText(ctx.renderer, 72, 126, "窗口模式：由 native_config.ini 控制", 2, SDL_Color{188, 198, 215, 255});
  DrawText(ctx.renderer, 72, 170, "输入映射：由 native_keymap.ini 覆盖", 2, SDL_Color{188, 198, 215, 255});
  DrawText(ctx.renderer, 72, 214, "目标设备：H700 / RG35XX 类 Linux 掌机", 2, SDL_Color{188, 198, 215, 255});
  DrawText(ctx.renderer, 72, 276, "说明：第一阶段只实现 App 空壳和场景系统", 2, SDL_Color{116, 200, 184, 255});
  DrawFooterHint(ctx.renderer, "B / Menu 返回首页");
}
