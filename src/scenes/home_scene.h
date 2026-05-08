#pragma once

#include "scene.h"

class HomeScene : public Scene {
public:
  void Update(float dt, const InputManager &input, SceneManager &scenes) override;
  void Render(AppContext &ctx) override;

private:
  int selected_ = 0;
};
