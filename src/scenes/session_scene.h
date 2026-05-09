#pragma once

#include "business/story_session.h"
#include "scene.h"

class SessionScene : public Scene {
public:
  void Update(float dt, const InputManager &input, SceneManager &scenes) override;
  void Render(AppContext &ctx) override;

private:
  StorySession session_{};
  int selected_choice_ = 0;
};
