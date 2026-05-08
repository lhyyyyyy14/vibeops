#include "scene_manager.h"

AppScene SceneManager::Current() const { return current_; }
AppScene SceneManager::Previous() const { return previous_; }

void SceneManager::Set(AppScene scene) {
  if (scene == current_) return;
  previous_ = current_;
  current_ = scene;
}

bool SceneManager::Is(AppScene scene) const { return current_ == scene; }
