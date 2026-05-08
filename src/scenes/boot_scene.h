#pragma once

#include "scene.h"

class BootScene : public Scene {
public:
  void OnEnter() override;
  void Update(float dt, const InputManager &input, SceneManager &scenes) override;
  void Render(AppContext &ctx) override;

private:
  float elapsed_ = 0.0f;
};
