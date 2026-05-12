#include "scenes/session_scene.h"

#include "ui_draw.h"

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
  DrawAppShell(ctx.renderer, "互动小说", "SESSION", session_.Loading() ? "ONLINE" : "READY");

  DrawTextBox(ctx.renderer, SDL_Rect{42, 84, 636, 174}, "故事", session_.Story(), 5);

  const std::string status = !session_.Error().empty()
                                 ? "错误: " + session_.Error()
                                 : session_.Loading() ? "正在连接 DeepSeek API..." : session_.LastAction();
  DrawPanel(ctx.renderer, SDL_Rect{42, 266, 636, 30}, UiPaperAlt(), UiLine());
  DrawTextWrapped(ctx.renderer, 56, 272, 608, 20, status, 2, session_.Error().empty() ? UiMuted() : UiRed(), 1);

  const auto choices = session_.Choices();
  int y = 306;
  int index = 0;
  for (const Choice &choice : choices) {
    const bool is_selected = index == selected_choice_;
    DrawChoiceRow(ctx.renderer, SDL_Rect{42, y, 636, 24}, choice.label,
                  choice.text.empty() ? "等待生成..." : choice.text, is_selected);
    y += 27;
    ++index;
  }

  DrawFooterButtons(ctx.renderer, {{"UP/DN", "CHOOSE"}, {"A", "SEND"}, {"B", "BACK"}});
}
