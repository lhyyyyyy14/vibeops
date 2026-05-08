#pragma once

#include "app_context.h"
#include "input_manager.h"
#include "scene_manager.h"

class Scene {
public:
  virtual ~Scene() = default;
  virtual void OnEnter() {}
  virtual void Update(float dt, const InputManager &input, SceneManager &scenes) = 0;
  virtual void Render(AppContext &ctx) = 0;
};
