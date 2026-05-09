#pragma once

#include "business/story_setup.h"
#include "scene.h"

#include <array>
#include <string>

class StorySetupScene : public Scene {
public:
  void OnEnter() override;
  void Update(float dt, const InputManager &input, SceneManager &scenes) override;
  void Render(AppContext &ctx) override;

private:
  // Three-step flow: choose keywords, choose styles, then confirm start.
  // The "done" rows are part of keyboard/controller navigation.
  enum class Page {
    Keywords,
    Styles,
    Confirm,
  };

  void ToggleCurrent();
  void ConfirmCurrent(SceneManager &scenes);
  StorySetup BuildSetup() const;
  int CurrentCount() const;
  int CurrentSize() const;
  int CurrentMax() const;
  std::string CurrentTitle() const;

  Page page_ = Page::Keywords;
  int selected_ = 0;
  std::array<bool, 10> keyword_selected_{};
  std::array<bool, 7> style_selected_{};
  bool keywords_done_ = false;
  bool styles_done_ = false;
  std::string status_;
};
