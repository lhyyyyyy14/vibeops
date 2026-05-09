#include "scenes/session_scene.h"

#include "ui_draw.h"

namespace {
SDL_Color Rgb(Uint8 r, Uint8 g, Uint8 b) { return SDL_Color{r, g, b, 255}; }
}  // namespace

void SessionScene::Update(float, const InputManager &input, SceneManager &scenes) {
  if (scenes.HasPendingStorySetup()) {
    session_.StartNew(scenes.ConsumePendingStorySetup());
    selected_choice_ = 0;
  }
  if (input.IsJustPressed(Button::B)) {
    scenes.Set(AppScene::Home);
    return;
  }
  if (input.IsJustPressed(Button::Up)) selected_choice_ = selected_choice_ <= 0 ? 3 : selected_choice_ - 1;
  if (input.IsJustPressed(Button::Down)) selected_choice_ = (selected_choice_ + 1) % 4;
  if (input.IsJustPressed(Button::A)) {
    const auto choices = session_.Choices();
    session_.Choose(choices[static_cast<std::size_t>(selected_choice_)].label);
  }
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
  int index = 0;
  for (const Choice &choice : choices) {
    const bool is_selected = index == selected_choice_;
    DrawPanel(ctx.renderer, SDL_Rect{42, y - 8, 636, 32}, is_selected ? Rgb(28, 45, 56) : Rgb(16, 23, 32),
              is_selected ? accent : Rgb(48, 62, 82));
    std::string label(1, choice.label);
    DrawText(ctx.renderer, 62, y, is_selected ? "> " + label : "  " + label, 2, accent);
    DrawTextWrapped(ctx.renderer, 96, y, 548, 22, choice.text, 2, body);
    y += 38;
    ++index;
  }

  DrawFooterHint(ctx.renderer, "上下选择  Enter/A 确认    Esc/B 返回");
}
