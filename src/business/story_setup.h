#pragma once

#include <string>
#include <vector>

struct StorySetupOption {
  std::string id;
  std::string label;
};

struct StorySetup {
  std::vector<StorySetupOption> keywords;
  std::vector<StorySetupOption> styles;
};

const std::vector<StorySetupOption> &AvailableStoryKeywords();
const std::vector<StorySetupOption> &AvailableStoryStyles();
std::string JoinSetupIds(const std::vector<StorySetupOption> &items);
std::string JoinSetupLabels(const std::vector<StorySetupOption> &items);
std::string StorySetupSummary(const StorySetup &setup);
