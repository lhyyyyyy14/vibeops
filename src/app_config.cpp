#include "app_config.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>

namespace {
std::string Trim(std::string value) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

bool ParseBool(const std::string &value, bool fallback) {
  std::string lower = value;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") return true;
  if (lower == "0" || lower == "false" || lower == "no" || lower == "off") return false;
  return fallback;
}
}  // namespace

bool EnvFlagEnabled(const char *name) {
  const char *value = std::getenv(name);
  return value && *value && std::string(value) != "0";
}

AppConfig LoadAppConfig(const std::string &path) {
  AppConfig config{};
  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    line = Trim(line);
    if (line.empty() || line[0] == '#' || line[0] == ';') continue;
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string key = Trim(line.substr(0, eq));
    const std::string value = Trim(line.substr(eq + 1));
    if (key == "fullscreen") config.fullscreen = ParseBool(value, config.fullscreen);
    if (key == "windowed") config.windowed = ParseBool(value, config.windowed);
    if (key == "input_profile") config.input_profile = value;
  }
  if (EnvFlagEnabled("GBINTERNOVEL_FULLSCREEN")) {
    config.fullscreen = true;
    config.windowed = false;
  }
  if (EnvFlagEnabled("GBINTERNOVEL_WINDOWED")) {
    config.fullscreen = false;
    config.windowed = true;
  }
  return config;
}
