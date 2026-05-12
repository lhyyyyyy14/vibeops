#include "scenes/history_scene.h"

#include "ui_draw.h"

#include <algorithm>

void HistoryScene::OnEnter() {
  view_ = View::Sessions;
  RefreshSessions();
}

void HistoryScene::Update(float, const InputManager &input, SceneManager &scenes) {
  if (view_ == View::Sessions) {
    if (input.IsJustPressed(Button::B)) {
      scenes.Set(AppScene::Home);
      return;
    }
    if (input.IsJustPressed(Button::Up)) selected_session_ = ClampIndex(selected_session_ - 1, sessions_.size());
    if (input.IsJustPressed(Button::Down)) selected_session_ = ClampIndex(selected_session_ + 1, sessions_.size());
    if (input.IsJustPressed(Button::A) && !sessions_.empty()) {
      LoadSelectedSession();
      view_ = View::Turns;
    }
    return;
  }

  if (view_ == View::Turns) {
    if (input.IsJustPressed(Button::B) || input.IsJustPressed(Button::Left)) {
      view_ = View::Sessions;
      return;
    }
    if (input.IsJustPressed(Button::Up)) selected_turn_ = ClampIndex(selected_turn_ - 1, turns_.size());
    if (input.IsJustPressed(Button::Down)) selected_turn_ = ClampIndex(selected_turn_ + 1, turns_.size());
    if ((input.IsJustPressed(Button::A) || input.IsJustPressed(Button::Right)) && !turns_.empty()) {
      view_ = View::Detail;
    }
    return;
  }

  if (input.IsJustPressed(Button::B) || input.IsJustPressed(Button::Left)) {
    view_ = View::Turns;
    return;
  }
  if (input.IsJustPressed(Button::L1) || input.IsJustPressed(Button::Up)) {
    selected_turn_ = ClampIndex(selected_turn_ - 1, turns_.size());
  }
  if (input.IsJustPressed(Button::R1) || input.IsJustPressed(Button::Down)) {
    selected_turn_ = ClampIndex(selected_turn_ + 1, turns_.size());
  }
}

void HistoryScene::Render(AppContext &ctx) {
  DrawAppShell(ctx.renderer, "历史数据", "HISTORY", store_.Ok() ? "READY" : "ERROR");

  if (!store_.Ok()) {
    DrawTextBox(ctx.renderer, SDL_Rect{58, 108, 604, 178}, "SQLite", "SQLite error: " + store_.Error(), 5);
    DrawFooterButtons(ctx.renderer, {{"B", "BACK"}});
    return;
  }

  if (view_ == View::Sessions) {
    DrawSectionPanel(ctx.renderer, SDL_Rect{42, 84, 430, 322}, "存档列表");
    DrawSectionPanel(ctx.renderer, SDL_Rect{500, 84, 178, 322}, "详情");

    if (sessions_.empty()) {
      DrawText(ctx.renderer, 72, 152, "暂无历史记录", 3, UiMuted());
    }
    const int visible = 7;
    const int start = std::max(0, std::min(selected_session_ - 3, static_cast<int>(sessions_.size()) - visible));
    const int end = std::min(static_cast<int>(sessions_.size()), start + visible);
    int y = 128;
    for (int i = start; i < end; ++i) {
      const HistorySession &session = sessions_[static_cast<std::size_t>(i)];
      const bool is_selected = i == selected_session_;
      DrawMenuItem(ctx.renderer, SDL_Rect{66, y, 380, 34}, session.updated_at,
                   "T" + std::to_string(session.turn_count), is_selected);
      y += 38;
    }
    if (!sessions_.empty()) {
      const HistorySession &session = sessions_[static_cast<std::size_t>(selected_session_)];
      DrawText(ctx.renderer, 520, 136, "模型", 2, UiAccent());
      DrawTextWrapped(ctx.renderer, 520, 164, 132, 20, session.model, 2, UiInk(), 2);
      DrawText(ctx.renderer, 520, 224, "设定", 2, UiAccent());
      DrawTextWrapped(ctx.renderer, 520, 252, 132, 20,
                      session.keyword_labels.empty() ? "无关键词" : session.keyword_labels + " / " + session.style_labels,
                      2, UiMuted(), 5);
    }
    DrawFooterButtons(ctx.renderer, {{"UP/DN", "MOVE"}, {"A", "OPEN"}, {"B", "BACK"}});
    return;
  }

  if (view_ == View::Turns) {
    DrawSectionPanel(ctx.renderer, SDL_Rect{42, 84, 430, 322}, "回合列表");
    DrawSectionPanel(ctx.renderer, SDL_Rect{500, 84, 178, 322}, "本局");

    if (turns_.empty()) {
      DrawText(ctx.renderer, 72, 152, "本局暂无回合", 3, UiMuted());
    }
    const int visible = 7;
    const int start = std::max(0, std::min(selected_turn_ - 3, static_cast<int>(turns_.size()) - visible));
    const int end = std::min(static_cast<int>(turns_.size()), start + visible);
    int y = 128;
    for (int i = start; i < end; ++i) {
      const HistoryTurn &turn = turns_[static_cast<std::size_t>(i)];
      const bool is_selected = i == selected_turn_;
      std::string line = "Turn " + std::to_string(turn.turn_index);
      if (!turn.selected_label.empty()) line += " / " + turn.selected_label;
      if (!turn.error.empty()) line += " / ERR";
      DrawMenuItem(ctx.renderer, SDL_Rect{66, y, 380, 34}, line, "", is_selected);
      y += 38;
    }
    if (!turns_.empty()) {
      const HistoryTurn &turn = turns_[static_cast<std::size_t>(selected_turn_)];
      DrawText(ctx.renderer, 520, 136, turn.error.empty() ? "OK" : "ERROR", 2, turn.error.empty() ? UiGreen() : UiRed());
      DrawText(ctx.renderer, 520, 178, std::to_string(turn.latency_ms) + " ms", 2, UiMuted());
      DrawTextWrapped(ctx.renderer, 520, 232, 132, 20, EdgeSummary(turn.id), 2, UiMuted(), 3);
    }
    DrawFooterButtons(ctx.renderer, {{"UP/DN", "MOVE"}, {"A", "DETAIL"}, {"B", "BACK"}});
    return;
  }

  if (turns_.empty()) {
    DrawFooterButtons(ctx.renderer, {{"B", "BACK"}});
    return;
  }

  const HistoryTurn &turn = turns_[static_cast<std::size_t>(selected_turn_)];
  DrawTextBox(ctx.renderer, SDL_Rect{42, 84, 636, 150}, "Turn " + std::to_string(turn.turn_index),
              turn.story.empty() ? "(empty story)" : turn.story, 4);
  DrawPanel(ctx.renderer, SDL_Rect{42, 244, 636, 28}, UiPaperAlt(), UiLine());
  DrawTextWrapped(ctx.renderer, 56, 250, 608, 18,
                  turn.error.empty() ? "prompt/raw response 已保存" : "Error: " + turn.error, 2,
                  turn.error.empty() ? UiMuted() : UiRed(), 1);

  int y = 282;
  for (const Choice &choice : turn.choices) {
    const std::string label(1, choice.label);
    const bool chosen = label == turn.selected_label;
    DrawChoiceRow(ctx.renderer, SDL_Rect{42, y, 636, 28}, choice.label, choice.text, chosen);
    y += 31;
  }
  DrawFooterButtons(ctx.renderer, {{"UP/DN", "TURN"}, {"B", "BACK"}});
}

void HistoryScene::RefreshSessions() {
  sessions_ = store_.ListSessions(30);
  selected_session_ = ClampIndex(selected_session_, sessions_.size());
}

void HistoryScene::LoadSelectedSession() {
  if (sessions_.empty()) return;
  const HistorySession &session = sessions_[static_cast<std::size_t>(selected_session_)];
  turns_ = store_.LoadTurns(session.id);
  edges_ = store_.LoadEdges(session.id);
  selected_turn_ = ClampIndex(0, turns_.size());
}

std::string HistoryScene::EdgeSummary(int turn_id) const {
  for (const HistoryEdge &edge : edges_) {
    if (edge.from_turn_id == turn_id) {
      const int to_index = TurnIndexForId(edge.to_turn_id);
      return "Flow: " + std::to_string(TurnIndexForId(edge.from_turn_id)) + " --" + edge.choice_label + "--> " +
             (to_index > 0 ? std::to_string(to_index) : "?");
    }
  }
  return "Flow: end";
}

int HistoryScene::TurnIndexForId(int turn_id) const {
  for (const HistoryTurn &turn : turns_) {
    if (turn.id == turn_id) return turn.turn_index;
  }
  return 0;
}

int HistoryScene::ClampIndex(int value, int size) {
  if (size <= 0) return 0;
  if (value < 0) return 0;
  if (value >= size) return size - 1;
  return value;
}
