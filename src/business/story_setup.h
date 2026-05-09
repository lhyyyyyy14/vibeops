#pragma once

#include <string>
#include <vector>

struct StorySetupOption {
  std::string id;
  std::string label;
};

// A per-session seed chosen before the first AI turn. It is intentionally
// small and serializable so SceneManager can pass it across scenes cheaply.
struct StorySetup {
  std::vector<StorySetupOption> keywords;
  std::vector<StorySetupOption> styles;
};

// Built-in v1 catalogs. Keep ids stable because they are persisted in SQLite.
const std::vector<StorySetupOption> &AvailableStoryKeywords();
const std::vector<StorySetupOption> &AvailableStoryStyles();
std::string JoinSetupIds(const std::vector<StorySetupOption> &items);
std::string JoinSetupLabels(const std::vector<StorySetupOption> &items);
std::string StorySetupSummary(const StorySetup &setup);
