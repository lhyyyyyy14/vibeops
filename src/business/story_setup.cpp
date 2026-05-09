#include "business/story_setup.h"

#include <sstream>

namespace {
std::string JoinField(const std::vector<StorySetupOption> &items, bool ids) {
  std::ostringstream out;
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (i > 0) out << ", ";
    out << (ids ? items[i].id : items[i].label);
  }
  return out.str();
}
}  // namespace

const std::vector<StorySetupOption> &AvailableStoryKeywords() {
  static const std::vector<StorySetupOption> keywords{
      {"wasteland_radio", "废土电台"},
      {"machine_temple", "机械神庙"},
      {"lost_fleet", "失踪舰队"},
      {"dream_archive", "梦境档案"},
      {"underground_bazaar", "地下集市"},
      {"time_rift", "时间裂缝"},
      {"ancient_ai", "古代 AI"},
      {"fog_lighthouse", "雾中灯塔"},
      {"forbidden_manuscript", "禁忌手稿"},
      {"orbital_colony", "轨道殖民地"},
  };
  return keywords;
}

const std::vector<StorySetupOption> &AvailableStoryStyles() {
  static const std::vector<StorySetupOption> styles{
      {"mystery", "悬疑"},
      {"sci_fi", "科幻"},
      {"magic", "魔法"},
      {"dark_comedy", "黑色幽默"},
      {"epic", "史诗"},
      {"warm", "温情"},
      {"horror", "恐怖"},
  };
  return styles;
}

std::string JoinSetupIds(const std::vector<StorySetupOption> &items) { return JoinField(items, true); }

std::string JoinSetupLabels(const std::vector<StorySetupOption> &items) { return JoinField(items, false); }

std::string StorySetupSummary(const StorySetup &setup) {
  return "关键词: " + JoinSetupLabels(setup.keywords) + " | 风格: " + JoinSetupLabels(setup.styles);
}
