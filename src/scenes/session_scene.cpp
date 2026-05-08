#include "scenes/session_scene.h"

#include "ui_draw.h"

void SessionScene::Update(float, const InputManager &input, SceneManager &scenes) {
  session_.EnsureStarted();
  if (input.IsJustPressed(Button::Menu) || input.IsJustPressed(Button::Select)) {
    scenes.Set(AppScene::Home);
    return;
  }
  if (input.IsJustPressed(Button::A)) session_.Choose('A');
  if (input.IsJustPressed(Button::B)) session_.Choose('B');
  if (input.IsJustPressed(Button::X)) session_.Choose('X');
  if (input.IsJustPressed(Button::Y)) session_.Choose('Y');
}

void SessionScene::Render(AppContext &ctx) {
  ClearScreen(ctx.renderer, SDL_Color{14, 20, 27, 255});
  DrawText(ctx.renderer, 28, 20, "互动小说会话", 4, SDL_Color{235, 238, 245, 255});
  DrawPanel(ctx.renderer, SDL_Rect{34, 82, 652, 142}, SDL_Color{27, 36, 48, 255}, SDL_Color{73, 90, 116, 255});
  DrawTextWrapped(ctx.renderer, 56, 104, 610, 28, session_.Story(), 2, SDL_Color{208, 216, 230, 255});
  DrawText(ctx.renderer, 56, 172, session_.Loading() ? "正在连接 DeepSeek API..." : session_.LastAction(), 2,
           SDL_Color{116, 200, 184, 255});
  if (!session_.Error().empty()) {
    DrawTextWrapped(ctx.renderer, 56, 202, 610, 24, session_.Error(), 2, SDL_Color{240, 120, 120, 255});
  }

  const auto choices = session_.Choices();
  int y = 252;
  for (const Choice &choice : choices) {
    std::string line;
    line.push_back(choice.label);
    line += ": ";
    line += choice.text;
    DrawText(ctx.renderer, 60, y, line, 2, SDL_Color{210, 218, 230, 255});
    y += 38;
  }
  DrawFooterHint(ctx.renderer, "A/B/X/Y 选择分支  Select/Menu 返回");
}
