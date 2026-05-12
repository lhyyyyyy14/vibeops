#include "scenes/story_setup_scene.h"

#include "ui_draw.h"

#include <algorithm>

void StorySetupScene::OnEnter() {
  page_ = Page::Keywords;
  selected_ = 0;
  keyword_selected_.fill(false);
  style_selected_.fill(false);
  keywords_done_ = false;
  styles_done_ = false;
  status_ = "第一步：选择 1-5 个关键词，然后确认完成。";
}

void StorySetupScene::Update(float, const InputManager &input, SceneManager &scenes) {
  if (input.IsJustPressed(Button::B)) {
    scenes.Set(AppScene::Home);
    return;
  }
  if (input.IsJustPressed(Button::Left)) {
    if (page_ == Page::Styles) {
      page_ = Page::Keywords;
      selected_ = CurrentSize() - 1;
      status_ = "已返回关键词选择。";
    } else if (page_ == Page::Confirm) {
      page_ = Page::Styles;
      selected_ = CurrentSize() - 1;
      status_ = "已返回风格选择。";
    }
  }
  if (input.IsJustPressed(Button::Right)) {
    if (page_ == Page::Keywords && keywords_done_) {
      page_ = Page::Styles;
      selected_ = 0;
      status_ = "第二步：选择 1-3 个风格，然后确认完成。";
    } else if (page_ == Page::Styles && styles_done_) {
      page_ = Page::Confirm;
      selected_ = 0;
      status_ = StorySetupSummary(BuildSetup());
    }
  }
  if (input.IsJustPressed(Button::Up)) selected_ = selected_ <= 0 ? CurrentSize() - 1 : selected_ - 1;
  if (input.IsJustPressed(Button::Down)) selected_ = (selected_ + 1) % CurrentSize();
  if (input.IsJustPressed(Button::A)) ConfirmCurrent(scenes);
}

void StorySetupScene::Render(AppContext &ctx) {
  DrawAppShell(ctx.renderer, "故事设定", "SETUP / " + CurrentTitle(), "READY");
  DrawTabs(ctx.renderer, 48, 76, {"关键词", "风格", "确认"}, page_ == Page::Keywords ? 0 : page_ == Page::Styles ? 1 : 2);

  DrawSectionPanel(ctx.renderer, SDL_Rect{42, 112, 440, 294},
                   CurrentTitle() + " " + std::to_string(CurrentCount()) + "/" + std::to_string(CurrentMax()));
  DrawSectionPanel(ctx.renderer, SDL_Rect{506, 112, 172, 294}, "摘要");

  if (page_ == Page::Confirm) {
    DrawTextWrapped(ctx.renderer, 68, 164, 388, 22, StorySetupSummary(BuildSetup()), 2, UiInk(), 5);
    DrawMenuItem(ctx.renderer, SDL_Rect{92, 314, 326, 44}, "完成并开始", "GO", selected_ == 0);
  } else {
    const auto &items = page_ == Page::Keywords ? AvailableStoryKeywords() : AvailableStoryStyles();
    int y = 152;
    const int row_h = page_ == Page::Keywords ? 21 : 28;
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
      const bool is_selected = i == selected_;
      const bool is_checked = page_ == Page::Keywords ? keyword_selected_[static_cast<std::size_t>(i)]
                                                       : style_selected_[static_cast<std::size_t>(i)];
      DrawPanel(ctx.renderer, SDL_Rect{66, y, 386, row_h - 2}, is_selected ? SDL_Color{232, 241, 255, 255} : UiPaperAlt(),
                is_selected ? UiAccent() : UiLine());
      DrawText(ctx.renderer, 78, y + 2, is_selected ? ">" : " ", 2, UiAccent());
      DrawText(ctx.renderer, 104, y + 2, is_checked ? "[x]" : "[ ]", 2, is_checked ? UiGreen() : UiMuted());
      DrawTextWrapped(ctx.renderer, 154, y + 2, 260, 18, items[i].label, 2, is_selected ? UiInk() : UiMuted(), 1);
      y += row_h;
    }
    const bool done_selected = selected_ == static_cast<int>(items.size());
    const std::string done_label = page_ == Page::Keywords ? "完成关键词选择" : "完成风格选择";
    DrawMenuItem(ctx.renderer, SDL_Rect{66, 364, 386, 28}, done_label, "NEXT", done_selected);
  }

  DrawText(ctx.renderer, 526, 158, "步骤", 2, UiAccent());
  DrawText(ctx.renderer, 526, 188, keywords_done_ || page_ != Page::Keywords ? "[x] 关键词" : "[ ] 关键词", 2,
           page_ == Page::Keywords ? UiAccent() : UiMuted());
  DrawText(ctx.renderer, 526, 218, styles_done_ || page_ == Page::Confirm ? "[x] 风格" : "[ ] 风格", 2,
           page_ == Page::Styles ? UiAccent() : UiMuted());
  DrawText(ctx.renderer, 526, 248, page_ == Page::Confirm ? "[x] 确认" : "[ ] 确认", 2,
           page_ == Page::Confirm ? UiAccent() : UiMuted());
  DrawTextWrapped(ctx.renderer, 526, 302, 128, 20, status_, 2, UiMuted(), 4);

  DrawFooterButtons(ctx.renderer, {{"UP/DN", "MOVE"}, {"A", "OK"}, {"L/R", "STEP"}, {"B", "BACK"}});
}

void StorySetupScene::ConfirmCurrent(SceneManager &scenes) {
  if (page_ == Page::Confirm) {
    scenes.SetPendingStorySetup(BuildSetup());
    scenes.Set(AppScene::Session);
    return;
  }

  const int item_count = page_ == Page::Keywords ? static_cast<int>(AvailableStoryKeywords().size())
                                                 : static_cast<int>(AvailableStoryStyles().size());
  if (selected_ == item_count) {
    if (CurrentCount() <= 0) {
      status_ = page_ == Page::Keywords ? "至少选择 1 个关键词。" : "至少选择 1 个风格。";
      return;
    }
    if (page_ == Page::Keywords) {
      keywords_done_ = true;
      page_ = Page::Styles;
      selected_ = 0;
      status_ = "第二步：选择 1-3 个风格，然后确认完成。";
    } else {
      styles_done_ = true;
      page_ = Page::Confirm;
      selected_ = 0;
      status_ = StorySetupSummary(BuildSetup());
    }
    return;
  }

  ToggleCurrent();
}

void StorySetupScene::ToggleCurrent() {
  if (page_ == Page::Keywords) {
    bool &value = keyword_selected_[static_cast<std::size_t>(selected_)];
    if (!value && CurrentCount() >= CurrentMax()) {
      status_ = "关键词最多选择 5 个。";
      return;
    }
    value = !value;
    keywords_done_ = false;
  } else if (page_ == Page::Styles) {
    bool &value = style_selected_[static_cast<std::size_t>(selected_)];
    if (!value && CurrentCount() >= CurrentMax()) {
      status_ = "风格最多选择 3 个。";
      return;
    }
    value = !value;
    styles_done_ = false;
  }
  status_ = StorySetupSummary(BuildSetup());
}

StorySetup StorySetupScene::BuildSetup() const {
  StorySetup setup;
  const auto &keywords = AvailableStoryKeywords();
  for (int i = 0; i < static_cast<int>(keywords.size()); ++i) {
    if (keyword_selected_[static_cast<std::size_t>(i)]) setup.keywords.push_back(keywords[static_cast<std::size_t>(i)]);
  }
  const auto &styles = AvailableStoryStyles();
  for (int i = 0; i < static_cast<int>(styles.size()); ++i) {
    if (style_selected_[static_cast<std::size_t>(i)]) setup.styles.push_back(styles[static_cast<std::size_t>(i)]);
  }
  return setup;
}

int StorySetupScene::CurrentCount() const {
  if (page_ == Page::Keywords) {
    return static_cast<int>(std::count(keyword_selected_.begin(), keyword_selected_.end(), true));
  }
  if (page_ == Page::Styles) {
    return static_cast<int>(std::count(style_selected_.begin(), style_selected_.end(), true));
  }
  return 1;
}

int StorySetupScene::CurrentSize() const {
  if (page_ == Page::Keywords) return static_cast<int>(AvailableStoryKeywords().size()) + 1;
  if (page_ == Page::Styles) return static_cast<int>(AvailableStoryStyles().size()) + 1;
  return 1;
}

int StorySetupScene::CurrentMax() const {
  if (page_ == Page::Keywords) return 5;
  if (page_ == Page::Styles) return 3;
  return 1;
}

std::string StorySetupScene::CurrentTitle() const {
  if (page_ == Page::Keywords) return "关键词";
  if (page_ == Page::Styles) return "风格";
  return "确认";
}
