#pragma once

#include "business/choice.h"
#include "business/story_setup.h"

#include <array>
#include <string>
#include <vector>

struct AiConfig {
  std::string api_url = "https://api.deepseek.com/chat/completions";
  std::string model = "deepseek-v4-flash";
  std::string api_key;
  std::string world = "中世纪猎巫题材，融合魔法与科技元素";
  double temperature = 0.7;
  int max_tokens = 900;
  int request_timeout = 30;
};

struct AiTurnRequest {
  std::vector<std::string> recent_story;
  StorySetup setup;
  Choice last_choice{};
  bool has_last_choice = false;
};

struct AiTurnResult {
  bool ok = false;
  std::string error;
  std::string story;
  std::array<Choice, 4> choices{};
  std::string prompt;
  std::string raw_response;
  int latency_ms = 0;
};

AiConfig LoadAiConfig(const std::string &path);
AiTurnResult GenerateStoryTurn(const AiConfig &config, const AiTurnRequest &request);
