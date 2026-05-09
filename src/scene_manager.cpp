#include "scene_manager.h"

AppScene SceneManager::Current() const { return current_; }
AppScene SceneManager::Previous() const { return previous_; }

void SceneManager::Set(AppScene scene) {
  if (scene == current_) return;
  previous_ = current_;
  current_ = scene;
}

bool SceneManager::Is(AppScene scene) const { return current_ == scene; }

void SceneManager::SetPendingStorySetup(const StorySetup &setup) {
  pending_story_setup_ = setup;
  has_pending_story_setup_ = true;
}

bool SceneManager::HasPendingStorySetup() const { return has_pending_story_setup_; }

StorySetup SceneManager::ConsumePendingStorySetup() {
  has_pending_story_setup_ = false;
  return pending_story_setup_;
}
