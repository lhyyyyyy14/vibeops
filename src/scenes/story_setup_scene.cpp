#include "scenes/story_setup_scene.h"

#include "business/story_setup.h"
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
  status_ = "选择 1-5 个关键词，再选择 1-3 个风格。";
}

void StorySetupScene::Update(float, const InputManager &input, SceneManager &scenes) {
  if (input.IsJustPressed(Button::B) || input.IsJustPressed(Button::Menu)) {
    scenes.Set(AppScene::Home);
    return;
  }
  if (input.IsJustPressed(Button::X) || input.IsJustPressed(Button::Y) || input.IsJustPressed(Button::Left) ||
      input.IsJustPressed(Button::Right)) {
    page_ = page_ == Page::Keywords ? Page::Styles : Page::Keywords;
    selected_ = std::min(selected_, CurrentSize() - 1);
    status_ = CurrentTitle();
  }
  if (input.IsJustPressed(Button::Up)) selected_ = selected_ <= 0 ? CurrentSize() - 1 : selected_ - 1;
  if (input.IsJustPressed(Button::Down)) selected_ = (selected_ + 1) % CurrentSize();
  if (input.IsJustPressed(Button::A)) ToggleCurrent();
  if (input.IsJustPressed(Button::Start)) {
    const int keyword_count = static_cast<int>(std::count(keyword_selected_.begin(), keyword_selected_.end(), true));
    const int style_count = static_cast<int>(std::count(style_selected_.begin(), style_selected_.end(), true));
    if (keyword_count <= 0 || style_count <= 0) {
      status_ = "至少选择 1 个关键词和 1 个风格。";
      return;
    }
    scenes.SetPendingStorySetup(BuildSetup());
    scenes.Set(AppScene::Session);
  }
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
  DrawText(ctx.renderer, 430, 32, page_ == Page::Keywords ? "KEYWORDS" : "STYLES", 2, active);

  DrawPanel(ctx.renderer, SDL_Rect{36, 76, 648, 314}, panel, border);
  DrawText(ctx.renderer, 58, 96,
           CurrentTitle() + "  " + std::to_string(CurrentCount()) + "/" + std::to_string(CurrentMax()), 2, active);

  const auto &items = page_ == Page::Keywords ? AvailableStoryKeywords() : AvailableStoryStyles();
  int y = 132;
  for (int i = 0; i < static_cast<int>(items.size()); ++i) {
    const bool is_selected = i == selected_;
    const bool is_checked =
        page_ == Page::Keywords ? keyword_selected_[static_cast<std::size_t>(i)] : style_selected_[static_cast<std::size_t>(i)];
    std::string line = std::string(is_selected ? "> " : "  ") + (is_checked ? "[x] " : "[ ] ") + items[i].label;
    DrawText(ctx.renderer, 66, y, line, 2, is_checked ? checked : is_selected ? active : idle);
    y += 28;
  }

  DrawTextWrapped(ctx.renderer, 58, 346, 600, 22, status_, 2, muted);
  DrawFooterHint(ctx.renderer, "A 勾选  X/Y 切换  Start 开始  B/Menu 返回");
}

void StorySetupScene::ToggleCurrent() {
  if (page_ == Page::Keywords) {
    bool &value = keyword_selected_[static_cast<std::size_t>(selected_)];
    if (!value && CurrentCount() >= CurrentMax()) {
      status_ = "关键词最多选择 5 个。";
      return;
    }
    value = !value;
  } else {
    bool &value = style_selected_[static_cast<std::size_t>(selected_)];
    if (!value && CurrentCount() >= CurrentMax()) {
      status_ = "风格最多选择 3 个。";
      return;
    }
    value = !value;
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
  return static_cast<int>(std::count(style_selected_.begin(), style_selected_.end(), true));
}

int StorySetupScene::CurrentSize() const {
  return static_cast<int>(page_ == Page::Keywords ? AvailableStoryKeywords().size() : AvailableStoryStyles().size());
}

int StorySetupScene::CurrentMax() const { return page_ == Page::Keywords ? 5 : 3; }

std::string StorySetupScene::CurrentTitle() const { return page_ == Page::Keywords ? "关键词" : "风格"; }
