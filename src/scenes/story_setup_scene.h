#pragma once

#include "scene.h"

#include <array>
#include <string>

class StorySetupScene : public Scene {
public:
  void OnEnter() override;
  void Update(float dt, const InputManager &input, SceneManager &scenes) override;
  void Render(AppContext &ctx) override;

private:
  enum class Page {
    Keywords,
    Styles,
  };

  void ToggleCurrent();
  StorySetup BuildSetup() const;
  int CurrentCount() const;
  int CurrentSize() const;
  int CurrentMax() const;
  std::string CurrentTitle() const;

  Page page_ = Page::Keywords;
  int selected_ = 0;
  std::array<bool, 10> keyword_selected_{};
  std::array<bool, 7> style_selected_{};
  std::string status_;
};
