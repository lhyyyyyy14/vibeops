#pragma once

#include "scene.h"

class SettingsScene : public Scene {
public:
  void Update(float dt, const InputManager &input, SceneManager &scenes) override;
  void Render(AppContext &ctx) override;
};
