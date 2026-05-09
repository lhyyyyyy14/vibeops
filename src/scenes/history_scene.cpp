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
  ClearScreen(ctx.renderer, SDL_Color{15, 19, 27, 255});
  const SDL_Color title{235, 238, 245, 255};
  const SDL_Color active{116, 200, 184, 255};
  const SDL_Color selected{250, 210, 120, 255};
  const SDL_Color idle{188, 198, 215, 255};
  const SDL_Color muted{132, 145, 166, 255};
  const SDL_Color error{240, 120, 120, 255};

  DrawText(ctx.renderer, 28, 20, "History Data", 4, title);

  if (!store_.Ok()) {
    DrawTextWrapped(ctx.renderer, 52, 96, 620, 24, "SQLite error: " + store_.Error(), 2, error);
    DrawFooterHint(ctx.renderer, "Esc/B Back");
    return;
  }

  if (view_ == View::Sessions) {
    DrawPanel(ctx.renderer, SDL_Rect{34, 78, 652, 320}, SDL_Color{27, 36, 48, 255}, SDL_Color{73, 90, 116, 255});
    if (sessions_.empty()) {
      DrawText(ctx.renderer, 60, 122, "No history yet.", 3, muted);
    }
    const int visible = 6;
    const int start = std::max(0, std::min(selected_session_ - 2, static_cast<int>(sessions_.size()) - visible));
    const int end = std::min(static_cast<int>(sessions_.size()), start + visible);
    int y = 104;
    for (int i = start; i < end; ++i) {
      const HistorySession &session = sessions_[static_cast<std::size_t>(i)];
      const bool is_selected = i == selected_session_;
      DrawText(ctx.renderer, 58, y,
               std::string(is_selected ? "> " : "  ") + session.updated_at + "  turns:" +
                   std::to_string(session.turn_count),
               2, is_selected ? active : idle);
      std::string setup_line = session.keyword_labels.empty() ? session.model : session.keyword_labels + " / " + session.style_labels;
      DrawText(ctx.renderer, 82, y + 24, setup_line, 2, muted);
      y += 48;
    }
    DrawFooterHint(ctx.renderer, "上下移动  Enter/A 打开  Esc/B 返回");
    return;
  }

  if (view_ == View::Turns) {
    DrawPanel(ctx.renderer, SDL_Rect{34, 78, 652, 320}, SDL_Color{27, 36, 48, 255}, SDL_Color{73, 90, 116, 255});
    if (!sessions_.empty()) {
      const HistorySession &session = sessions_[static_cast<std::size_t>(selected_session_)];
      if (!session.keyword_labels.empty()) {
        DrawText(ctx.renderer, 58, 82, session.keyword_labels + " / " + session.style_labels, 2, active);
      }
    }
    if (turns_.empty()) {
      DrawText(ctx.renderer, 60, 122, "No turns in this session.", 3, muted);
    }
    const int visible = 6;
    const int start = std::max(0, std::min(selected_turn_ - 3, static_cast<int>(turns_.size()) - visible));
    const int end = std::min(static_cast<int>(turns_.size()), start + visible);
    int y = 112;
    for (int i = start; i < end; ++i) {
      const HistoryTurn &turn = turns_[static_cast<std::size_t>(i)];
      const bool is_selected = i == selected_turn_;
      std::string line = std::string(is_selected ? "> " : "  ") + "Turn " + std::to_string(turn.turn_index);
      if (!turn.selected_label.empty()) line += " chose " + turn.selected_label;
      if (!turn.error.empty()) line += "  ERROR";
      DrawText(ctx.renderer, 58, y, line, 2, is_selected ? active : idle);
      DrawText(ctx.renderer, 82, y + 24, EdgeSummary(turn.id), 2, turn.error.empty() ? muted : error);
      y += 42;
    }
    DrawFooterHint(ctx.renderer, "上下移动  Enter/A 详情  Esc/B 返回");
    return;
  }

  if (turns_.empty()) {
    DrawFooterHint(ctx.renderer, "B Back");
    return;
  }
  const HistoryTurn &turn = turns_[static_cast<std::size_t>(selected_turn_)];
  DrawText(ctx.renderer, 420, 26, "Turn " + std::to_string(turn.turn_index), 3, active);
  DrawPanel(ctx.renderer, SDL_Rect{34, 72, 652, 154}, SDL_Color{27, 36, 48, 255}, SDL_Color{73, 90, 116, 255});
  DrawTextWrapped(ctx.renderer, 56, 92, 610, 24, turn.story.empty() ? "(empty story)" : turn.story, 2, idle);

  int y = 250;
  for (const Choice &choice : turn.choices) {
    const std::string label(1, choice.label);
    const bool chosen = label == turn.selected_label;
    DrawText(ctx.renderer, 60, y, std::string(chosen ? "> " : "  ") + label + ": " + choice.text, 2,
             chosen ? selected : idle);
    y += 36;
  }
  if (!turn.error.empty()) {
    DrawTextWrapped(ctx.renderer, 56, 394, 610, 22, "Error: " + turn.error, 2, error);
  } else {
    DrawText(ctx.renderer, 56, 394, "Latency: " + std::to_string(turn.latency_ms) + " ms  prompt/raw saved", 2, muted);
  }
  DrawFooterHint(ctx.renderer, "上下切换  Esc/B 返回");
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
