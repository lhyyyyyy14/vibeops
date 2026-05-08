#pragma once

enum class AppScene {
  Boot,
  Home,
  Settings,
  Session,
};

class SceneManager {
public:
  AppScene Current() const;
  AppScene Previous() const;
  void Set(AppScene scene);
  bool Is(AppScene scene) const;

private:
  AppScene current_ = AppScene::Boot;
  AppScene previous_ = AppScene::Boot;
};
