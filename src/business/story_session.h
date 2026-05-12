#pragma once

#include "business/ai_client.h"
#include "business/choice.h"
#include "business/history_store.h"

#include <array>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class StorySession {
public:
  StorySession();
  ~StorySession();

  void StartNew(const StorySetup &setup);
  void EnsureStarted();
  std::string Story() const;
  std::array<Choice, 4> Choices() const;
  std::string LastAction() const;
  bool Loading() const;
  std::string Error() const;
  void Choose(char label);

private:
  void StartRequest(const Choice *choice);
  void ApplyResult(const AiTurnResult &result, const Choice *choice);

  AiConfig config_{};
  HistoryStore history_{};
  StorySetup setup_{};
  std::string session_id_;
  std::string story_ = "这里是互动小说会话页。请选择关键词和风格，开始生成第一段故事。";
  std::array<Choice, 4> choices_{
      Choice{'A', "调查村口线索"},
      Choice{'B', "询问机械镇长"},
      Choice{'X', "检查蒸汽装置"},
      Choice{'Y', "潜入钟楼阴影"},
  };
  std::string last_action_ = "尚未选择分支";
  std::string error_;
  std::vector<std::string> recent_story_;
  Choice last_choice_{};
  Choice pending_edge_choice_{};
  int turn_index_ = 0;
  int current_turn_id_ = 0;
  int pending_edge_from_turn_id_ = 0;
  bool has_last_choice_ = false;
  bool has_pending_edge_ = false;
  bool started_ = false;
  bool loading_ = false;
  std::thread worker_;
  mutable std::mutex mutex_;
};
