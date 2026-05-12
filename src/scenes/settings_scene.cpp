#include "scenes/settings_scene.h"

#include "ui_draw.h"

void SettingsScene::Update(float, const InputManager &input, SceneManager &scenes) {
  if (input.IsJustPressed(Button::B)) {
    scenes.Set(AppScene::Home);
  }
}

void SettingsScene::Render(AppContext &ctx) {
  DrawAppShell(ctx.renderer, "设置中心", "CONFIG", "READY");

  DrawSectionPanel(ctx.renderer, SDL_Rect{58, 96, 418, 280}, "运行文件");
  DrawMenuItem(ctx.renderer, SDL_Rect{88, 148, 338, 42}, "窗口模式", "INI", false);
  DrawText(ctx.renderer, 118, 198, "native_config.ini", 2, UiMuted());
  DrawMenuItem(ctx.renderer, SDL_Rect{88, 230, 338, 42}, "输入映射", "INI", false);
  DrawText(ctx.renderer, 118, 280, "native_keymap.ini", 2, UiMuted());
  DrawMenuItem(ctx.renderer, SDL_Rect{88, 312, 338, 42}, "API 配置", "JSON", false);

  DrawSectionPanel(ctx.renderer, SDL_Rect{504, 96, 136, 280}, "设备");
  DrawTextWrapped(ctx.renderer, 522, 142, 100, 21, "H700 / RG35XX 类 Linux 掌机", 2, UiInk(), 3);
  DrawText(ctx.renderer, 522, 236, "720x480", 2, UiAccent());
  DrawText(ctx.renderer, 522, 282, "SDL2", 2, UiMuted());

  DrawFooterButtons(ctx.renderer, {{"B", "BACK"}});
}
