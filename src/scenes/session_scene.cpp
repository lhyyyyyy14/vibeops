#include "scenes/session_scene.h"

#include "ui_draw.h"

namespace {
SDL_Color Rgb(Uint8 r, Uint8 g, Uint8 b) { return SDL_Color{r, g, b, 255}; }
}  // namespace

void SessionScene::Update(float, const InputManager &input, SceneManager &scenes) {
  if (scenes.HasPendingStorySetup()) {
    session_.StartNew(scenes.ConsumePendingStorySetup());
  }
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
  const SDL_Color bg = Rgb(12, 18, 25);
  const SDL_Color panel = Rgb(27, 36, 48);
  const SDL_Color panel_alt = Rgb(22, 30, 41);
  const SDL_Color border = Rgb(73, 90, 116);
  const SDL_Color title = Rgb(238, 242, 248);
  const SDL_Color body = Rgb(208, 216, 230);
  const SDL_Color muted = Rgb(148, 162, 184);
  const SDL_Color accent = Rgb(116, 200, 184);
  const SDL_Color warn = Rgb(240, 120, 120);

  ClearScreen(ctx.renderer, bg);
  DrawText(ctx.renderer, 34, 24, "互动小说会话", 4, title);
  DrawText(ctx.renderer, 520, 32, session_.Loading() ? "ONLINE" : "READY", 2, session_.Loading() ? accent : muted);

  DrawPanel(ctx.renderer, SDL_Rect{42, 76, 636, 156}, panel, border);
  DrawTextWrapped(ctx.renderer, 64, 100, 592, 26, session_.Story(), 2, body);

  DrawPanel(ctx.renderer, SDL_Rect{42, 246, 636, 38}, panel_alt, border);
  if (!session_.Error().empty()) {
    DrawTextWrapped(ctx.renderer, 60, 256, 600, 22, session_.Error(), 2, warn);
  } else {
    DrawText(ctx.renderer, 60, 256, session_.Loading() ? "正在连接 DeepSeek API..." : session_.LastAction(), 2,
             session_.Loading() ? accent : muted);
  }

  const auto choices = session_.Choices();
  int y = 300;
  for (const Choice &choice : choices) {
    DrawPanel(ctx.renderer, SDL_Rect{42, y - 8, 636, 32}, Rgb(16, 23, 32), Rgb(48, 62, 82));
    std::string label(1, choice.label);
    DrawText(ctx.renderer, 62, y, label, 2, accent);
    DrawTextWrapped(ctx.renderer, 96, y, 548, 22, choice.text, 2, body);
    y += 38;
  }

  DrawFooterHint(ctx.renderer, "A/B/X/Y 选择分支    Select/Menu 返回");
}
