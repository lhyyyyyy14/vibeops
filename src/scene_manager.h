#pragma once

#include "business/story_setup.h"

enum class AppScene {
  Boot,
  Home,
  StorySetup,
  History,
  Settings,
  Session,
};

class SceneManager {
public:
  AppScene Current() const;
  AppScene Previous() const;
  void Set(AppScene scene);
  bool Is(AppScene scene) const;
  void SetPendingStorySetup(const StorySetup &setup);
  bool HasPendingStorySetup() const;
  StorySetup ConsumePendingStorySetup();

private:
  AppScene current_ = AppScene::Boot;
  AppScene previous_ = AppScene::Boot;
  StorySetup pending_story_setup_{};
  bool has_pending_story_setup_ = false;
};
