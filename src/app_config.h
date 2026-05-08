#pragma once

#include <string>

struct AppConfig {
  bool fullscreen = false;
  bool windowed = true;
  std::string input_profile = "desktop";
};

AppConfig LoadAppConfig(const std::string &path);
bool EnvFlagEnabled(const char *name);
