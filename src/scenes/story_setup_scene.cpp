#include "scenes/story_setup_scene.h"

#include "ui_draw.h"

#include <algorithm>

namespace {
SDL_Color Rgb(Uint8 r, Uint8 g, Uint8 b) { return SDL_Color{r, g, b, 255}; }
}  // namespace

void StorySetupScene::OnEnter() {
  page_ = Page::Keywords;
  selected_ = 0;
  keyword_selected_.fill(false);
  style_selected_.fill(false);
  keywords_done_ = false;
  styles_done_ = false;
  status_ = "第一步：选择 1-5 个关键词，然后移动到完成按钮确认。";
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
  const SDL_Color bg = Rgb(13, 18, 26);
  const SDL_Color panel = Rgb(25, 34, 46);
  const SDL_Color border = Rgb(70, 86, 112);
  const SDL_Color title = Rgb(238, 242, 248);
  const SDL_Color idle = Rgb(190, 200, 216);
  const SDL_Color muted = Rgb(142, 156, 178);
  const SDL_Color active = Rgb(116, 200, 184);
  const SDL_Color checked = Rgb(250, 210, 120);

  ClearScreen(ctx.renderer, bg);
  DrawText(ctx.renderer, 34, 24, "故事设定", 4, title);
  DrawText(ctx.renderer, 430, 32, CurrentTitle(), 2, active);

  DrawPanel(ctx.renderer, SDL_Rect{36, 76, 648, 314}, panel, border);
  DrawText(ctx.renderer, 58, 96,
           CurrentTitle() + "  " + std::to_string(CurrentCount()) + "/" + std::to_string(CurrentMax()), 2, active);

  if (page_ == Page::Confirm) {
    DrawTextWrapped(ctx.renderer, 66, 140, 590, 24, StorySetupSummary(BuildSetup()), 2, idle);
    DrawText(ctx.renderer, 66, 252, selected_ == 0 ? "> 完成并开始" : "  完成并开始", 3, active);
  } else {
    const auto &items = page_ == Page::Keywords ? AvailableStoryKeywords() : AvailableStoryStyles();
    int y = 132;
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
      const bool is_selected = i == selected_;
      const bool is_checked = page_ == Page::Keywords ? keyword_selected_[static_cast<std::size_t>(i)]
                                                       : style_selected_[static_cast<std::size_t>(i)];
      std::string line = std::string(is_selected ? "> " : "  ") + (is_checked ? "[x] " : "[ ] ") + items[i].label;
      DrawText(ctx.renderer, 66, y, line, 2, is_checked ? checked : is_selected ? active : idle);
      y += 25;
    }
    const bool done_selected = selected_ == static_cast<int>(items.size());
    const std::string done_label = page_ == Page::Keywords ? "完成关键词选择" : "完成风格选择";
    DrawText(ctx.renderer, 66, 352, std::string(done_selected ? "> " : "  ") + done_label, 2,
             done_selected ? active : muted);
  }

  DrawTextWrapped(ctx.renderer, 58, 400, 600, 22, status_, 2, muted);
  DrawFooterHint(ctx.renderer, "上下移动  Enter/A 确认  左右返回/前进  Esc/B 取消");
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
      status_ = "第二步：选择 1-3 个风格，然后移动到完成按钮确认。";
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
