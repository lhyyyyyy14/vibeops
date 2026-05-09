#pragma once

#include "business/choice.h"
#include "business/story_setup.h"

#include <array>
#include <mutex>
#include <string>
#include <vector>

struct HistorySession {
  std::string id;
  std::string created_at;
  std::string updated_at;
  std::string model;
  std::string world;
  std::string keyword_labels;
  std::string style_labels;
  int turn_count = 0;
};

struct HistoryTurn {
  int id = 0;
  int turn_index = 0;
  std::string created_at;
  std::string story;
  std::array<Choice, 4> choices{};
  std::string selected_label;
  std::string selected_text;
  std::string prompt;
  std::string raw_response;
  std::string error;
  int latency_ms = 0;
};

struct HistoryEdge {
  int from_turn_id = 0;
  int to_turn_id = 0;
  std::string choice_label;
  std::string choice_text;
};

// Thin SQLite boundary for all persisted story data. Scenes and StorySession
// call this API instead of embedding SQL so schema changes stay localized.
class HistoryStore {
public:
  HistoryStore();
  explicit HistoryStore(const std::string &db_path);
  ~HistoryStore();

  HistoryStore(const HistoryStore &) = delete;
  HistoryStore &operator=(const HistoryStore &) = delete;

  bool Ok() const;
  std::string Error() const;

  // Write path used by StorySession during live play.
  std::string StartSession(const std::string &model, const std::string &world);
  bool InsertStorySetup(const std::string &session_id, const StorySetup &setup);
  bool UpdateStorySetupInitialTurn(const std::string &session_id, int turn_id);
  int InsertTurn(const std::string &session_id, int turn_index, const std::string &story,
                 const std::array<Choice, 4> &choices, const std::string &prompt,
                 const std::string &raw_response, const std::string &error, int latency_ms);
  bool UpdateTurnSelection(int turn_id, const Choice &choice);
  bool InsertEdge(const std::string &session_id, int from_turn_id, int to_turn_id, const Choice &choice);

  // Read path used by HistoryScene.
  std::vector<HistorySession> ListSessions(int limit);
  std::vector<HistoryTurn> LoadTurns(const std::string &session_id);
  std::vector<HistoryEdge> LoadEdges(const std::string &session_id);

private:
  bool Open(const std::string &db_path);
  bool InitSchema();
  bool Exec(const std::string &sql);
  void SetError(const std::string &message);
  void TouchSession(const std::string &session_id);

  void *db_ = nullptr;
  std::string error_;
  mutable std::mutex mutex_;
};

std::string DefaultHistoryDbPath();
