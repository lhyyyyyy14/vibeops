#pragma once

#include "business/history_store.h"
#include "scene.h"

#include <string>
#include <vector>

class HistoryScene : public Scene {
public:
  void OnEnter() override;
  void Update(float dt, const InputManager &input, SceneManager &scenes) override;
  void Render(AppContext &ctx) override;

private:
  enum class View {
    Sessions,
    Turns,
    Detail,
  };

  void RefreshSessions();
  void LoadSelectedSession();
  std::string EdgeSummary(int turn_id) const;
  int TurnIndexForId(int turn_id) const;
  static int ClampIndex(int value, int size);

  HistoryStore store_{};
  View view_ = View::Sessions;
  std::vector<HistorySession> sessions_;
  std::vector<HistoryTurn> turns_;
  std::vector<HistoryEdge> edges_;
  int selected_session_ = 0;
  int selected_turn_ = 0;
};
