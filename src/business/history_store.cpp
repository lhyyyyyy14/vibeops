#include "business/history_store.h"

#include <sqlite3.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>

namespace {
sqlite3 *AsDb(void *db) { return static_cast<sqlite3 *>(db); }

std::string ColumnText(sqlite3_stmt *stmt, int index) {
  const unsigned char *text = sqlite3_column_text(stmt, index);
  return text ? reinterpret_cast<const char *>(text) : "";
}

void BindText(sqlite3_stmt *stmt, int index, const std::string &value) {
  sqlite3_bind_text(stmt, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
}

std::string NowIso() {
  using namespace std::chrono;
  const std::time_t t = system_clock::to_time_t(system_clock::now());
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &t);
#else
  gmtime_r(&t, &tm);
#endif
  std::ostringstream out;
  out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}

std::string NewSessionId() {
  using namespace std::chrono;
  const auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<unsigned int> dist(0, 0xffff);
  std::ostringstream out;
  out << "session_" << now << "_" << std::hex << std::setw(4) << std::setfill('0') << dist(rng);
  return out.str();
}

std::array<Choice, 4> EmptyChoices() {
  return {Choice{'A', ""}, Choice{'B', ""}, Choice{'X', ""}, Choice{'Y', ""}};
}
}  // namespace

std::string DefaultHistoryDbPath() {
  std::filesystem::path base;
  if (const char *env = std::getenv("GBINTERNOVEL_DATA_DIR")) {
    if (*env) base = env;
  }
  if (base.empty()) base = "data";
  return (base / "gb_internovel.db").string();
}

HistoryStore::HistoryStore() { Open(DefaultHistoryDbPath()); }

HistoryStore::HistoryStore(const std::string &db_path) { Open(db_path); }

HistoryStore::~HistoryStore() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (db_) sqlite3_close(AsDb(db_));
  db_ = nullptr;
}

bool HistoryStore::Ok() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return db_ != nullptr && error_.empty();
}

std::string HistoryStore::Error() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return error_;
}

bool HistoryStore::Open(const std::string &db_path) {
  std::lock_guard<std::mutex> lock(mutex_);
  try {
    const std::filesystem::path path(db_path);
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
  } catch (const std::exception &exc) {
    SetError(std::string("history db directory failed: ") + exc.what());
    return false;
  }

  sqlite3 *db = nullptr;
  if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
    error_ = db ? sqlite3_errmsg(db) : "sqlite3_open failed";
    if (db) sqlite3_close(db);
    return false;
  }
  db_ = db;
  return InitSchema();
}

bool HistoryStore::InitSchema() {
  return Exec("PRAGMA foreign_keys = ON;") &&
         Exec("CREATE TABLE IF NOT EXISTS sessions ("
              "id TEXT PRIMARY KEY,"
              "created_at TEXT NOT NULL,"
              "updated_at TEXT NOT NULL,"
              "model TEXT,"
              "world TEXT"
              ");") &&
         Exec("CREATE TABLE IF NOT EXISTS turns ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "session_id TEXT NOT NULL,"
              "turn_index INTEGER NOT NULL,"
              "created_at TEXT NOT NULL,"
              "story TEXT NOT NULL,"
              "option_a TEXT NOT NULL,"
              "option_b TEXT NOT NULL,"
              "option_x TEXT NOT NULL,"
              "option_y TEXT NOT NULL,"
              "selected_label TEXT,"
              "selected_text TEXT,"
              "prompt TEXT,"
              "raw_response TEXT,"
              "error TEXT,"
              "latency_ms INTEGER NOT NULL DEFAULT 0,"
              "FOREIGN KEY(session_id) REFERENCES sessions(id)"
              ");") &&
         Exec("CREATE TABLE IF NOT EXISTS turn_edges ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "session_id TEXT NOT NULL,"
              "from_turn_id INTEGER NOT NULL,"
              "to_turn_id INTEGER NOT NULL,"
              "choice_label TEXT NOT NULL,"
              "choice_text TEXT NOT NULL,"
              "FOREIGN KEY(session_id) REFERENCES sessions(id),"
              "FOREIGN KEY(from_turn_id) REFERENCES turns(id),"
              "FOREIGN KEY(to_turn_id) REFERENCES turns(id)"
              ");") &&
         Exec("CREATE TABLE IF NOT EXISTS story_setups ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "session_id TEXT NOT NULL UNIQUE,"
              "created_at TEXT NOT NULL,"
              "keyword_ids TEXT NOT NULL,"
              "keyword_labels TEXT NOT NULL,"
              "style_ids TEXT NOT NULL,"
              "style_labels TEXT NOT NULL,"
              "initial_turn_id INTEGER,"
              "FOREIGN KEY(session_id) REFERENCES sessions(id),"
              "FOREIGN KEY(initial_turn_id) REFERENCES turns(id)"
              ");") &&
         Exec("CREATE INDEX IF NOT EXISTS idx_turns_session ON turns(session_id, turn_index, id);") &&
         Exec("CREATE INDEX IF NOT EXISTS idx_edges_session ON turn_edges(session_id, from_turn_id);");
}

bool HistoryStore::Exec(const std::string &sql) {
  char *message = nullptr;
  if (sqlite3_exec(AsDb(db_), sql.c_str(), nullptr, nullptr, &message) != SQLITE_OK) {
    error_ = message ? message : sqlite3_errmsg(AsDb(db_));
    sqlite3_free(message);
    return false;
  }
  return true;
}

void HistoryStore::SetError(const std::string &message) { error_ = message; }

std::string HistoryStore::StartSession(const std::string &model, const std::string &world) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!db_) return "";
  const std::string id = NewSessionId();
  const std::string now = NowIso();
  sqlite3_stmt *stmt = nullptr;
  const char *sql = "INSERT INTO sessions(id, created_at, updated_at, model, world) VALUES(?, ?, ?, ?, ?);";
  if (sqlite3_prepare_v2(AsDb(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
    SetError(sqlite3_errmsg(AsDb(db_)));
    return "";
  }
  BindText(stmt, 1, id);
  BindText(stmt, 2, now);
  BindText(stmt, 3, now);
  BindText(stmt, 4, model);
  BindText(stmt, 5, world);
  const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  if (!ok) SetError(sqlite3_errmsg(AsDb(db_)));
  sqlite3_finalize(stmt);
  return ok ? id : "";
}

bool HistoryStore::InsertStorySetup(const std::string &session_id, const StorySetup &setup) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!db_ || session_id.empty()) return false;
  sqlite3_stmt *stmt = nullptr;
  const char *sql =
      "INSERT OR REPLACE INTO story_setups(session_id, created_at, keyword_ids, keyword_labels, style_ids, "
      "style_labels, initial_turn_id) VALUES(?, ?, ?, ?, ?, ?, NULL);";
  if (sqlite3_prepare_v2(AsDb(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
    SetError(sqlite3_errmsg(AsDb(db_)));
    return false;
  }
  BindText(stmt, 1, session_id);
  BindText(stmt, 2, NowIso());
  BindText(stmt, 3, JoinSetupIds(setup.keywords));
  BindText(stmt, 4, JoinSetupLabels(setup.keywords));
  BindText(stmt, 5, JoinSetupIds(setup.styles));
  BindText(stmt, 6, JoinSetupLabels(setup.styles));
  const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  if (!ok) SetError(sqlite3_errmsg(AsDb(db_)));
  sqlite3_finalize(stmt);
  return ok;
}

bool HistoryStore::UpdateStorySetupInitialTurn(const std::string &session_id, int turn_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!db_ || session_id.empty() || turn_id <= 0) return false;
  sqlite3_stmt *stmt = nullptr;
  const char *sql = "UPDATE story_setups SET initial_turn_id = ? WHERE session_id = ?;";
  if (sqlite3_prepare_v2(AsDb(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
    SetError(sqlite3_errmsg(AsDb(db_)));
    return false;
  }
  sqlite3_bind_int(stmt, 1, turn_id);
  BindText(stmt, 2, session_id);
  const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  if (!ok) SetError(sqlite3_errmsg(AsDb(db_)));
  sqlite3_finalize(stmt);
  return ok;
}

int HistoryStore::InsertTurn(const std::string &session_id, int turn_index, const std::string &story,
                             const std::array<Choice, 4> &choices, const std::string &prompt,
                             const std::string &raw_response, const std::string &error, int latency_ms) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!db_ || session_id.empty()) return 0;
  sqlite3_stmt *stmt = nullptr;
  const char *sql =
      "INSERT INTO turns(session_id, turn_index, created_at, story, option_a, option_b, option_x, option_y, "
      "prompt, raw_response, error, latency_ms) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
  if (sqlite3_prepare_v2(AsDb(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
    SetError(sqlite3_errmsg(AsDb(db_)));
    return 0;
  }
  BindText(stmt, 1, session_id);
  sqlite3_bind_int(stmt, 2, turn_index);
  BindText(stmt, 3, NowIso());
  BindText(stmt, 4, story);
  BindText(stmt, 5, choices[0].text);
  BindText(stmt, 6, choices[1].text);
  BindText(stmt, 7, choices[2].text);
  BindText(stmt, 8, choices[3].text);
  BindText(stmt, 9, prompt);
  BindText(stmt, 10, raw_response);
  BindText(stmt, 11, error);
  sqlite3_bind_int(stmt, 12, latency_ms);
  const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  if (!ok) SetError(sqlite3_errmsg(AsDb(db_)));
  sqlite3_finalize(stmt);
  if (!ok) return 0;
  const int id = static_cast<int>(sqlite3_last_insert_rowid(AsDb(db_)));
  TouchSession(session_id);
  return id;
}

bool HistoryStore::UpdateTurnSelection(int turn_id, const Choice &choice) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!db_ || turn_id <= 0) return false;
  sqlite3_stmt *stmt = nullptr;
  const char *sql = "UPDATE turns SET selected_label = ?, selected_text = ? WHERE id = ?;";
  if (sqlite3_prepare_v2(AsDb(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
    SetError(sqlite3_errmsg(AsDb(db_)));
    return false;
  }
  std::string label(1, choice.label);
  BindText(stmt, 1, label);
  BindText(stmt, 2, choice.text);
  sqlite3_bind_int(stmt, 3, turn_id);
  const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  if (!ok) SetError(sqlite3_errmsg(AsDb(db_)));
  sqlite3_finalize(stmt);
  return ok;
}

bool HistoryStore::InsertEdge(const std::string &session_id, int from_turn_id, int to_turn_id, const Choice &choice) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!db_ || session_id.empty() || from_turn_id <= 0 || to_turn_id <= 0) return false;
  sqlite3_stmt *stmt = nullptr;
  const char *sql =
      "INSERT INTO turn_edges(session_id, from_turn_id, to_turn_id, choice_label, choice_text) VALUES(?, ?, ?, ?, ?);";
  if (sqlite3_prepare_v2(AsDb(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
    SetError(sqlite3_errmsg(AsDb(db_)));
    return false;
  }
  std::string label(1, choice.label);
  BindText(stmt, 1, session_id);
  sqlite3_bind_int(stmt, 2, from_turn_id);
  sqlite3_bind_int(stmt, 3, to_turn_id);
  BindText(stmt, 4, label);
  BindText(stmt, 5, choice.text);
  const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
  if (!ok) SetError(sqlite3_errmsg(AsDb(db_)));
  sqlite3_finalize(stmt);
  if (ok) TouchSession(session_id);
  return ok;
}

std::vector<HistorySession> HistoryStore::ListSessions(int limit) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<HistorySession> sessions;
  if (!db_) return sessions;
  sqlite3_stmt *stmt = nullptr;
  const char *sql =
      "SELECT s.id, s.created_at, s.updated_at, s.model, s.world, "
      "COALESCE(st.keyword_labels, ''), COALESCE(st.style_labels, ''), COUNT(t.id) "
      "FROM sessions s LEFT JOIN turns t ON t.session_id = s.id "
      "LEFT JOIN story_setups st ON st.session_id = s.id "
      "GROUP BY s.id ORDER BY s.updated_at DESC LIMIT ?;";
  if (sqlite3_prepare_v2(AsDb(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
    SetError(sqlite3_errmsg(AsDb(db_)));
    return sessions;
  }
  sqlite3_bind_int(stmt, 1, limit);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    HistorySession session;
    session.id = ColumnText(stmt, 0);
    session.created_at = ColumnText(stmt, 1);
    session.updated_at = ColumnText(stmt, 2);
    session.model = ColumnText(stmt, 3);
    session.world = ColumnText(stmt, 4);
    session.keyword_labels = ColumnText(stmt, 5);
    session.style_labels = ColumnText(stmt, 6);
    session.turn_count = sqlite3_column_int(stmt, 7);
    sessions.push_back(session);
  }
  sqlite3_finalize(stmt);
  return sessions;
}

std::vector<HistoryTurn> HistoryStore::LoadTurns(const std::string &session_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<HistoryTurn> turns;
  if (!db_ || session_id.empty()) return turns;
  sqlite3_stmt *stmt = nullptr;
  const char *sql =
      "SELECT id, turn_index, created_at, story, option_a, option_b, option_x, option_y, "
      "selected_label, selected_text, prompt, raw_response, error, latency_ms "
      "FROM turns WHERE session_id = ? ORDER BY turn_index ASC, id ASC;";
  if (sqlite3_prepare_v2(AsDb(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
    SetError(sqlite3_errmsg(AsDb(db_)));
    return turns;
  }
  BindText(stmt, 1, session_id);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    HistoryTurn turn;
    turn.choices = EmptyChoices();
    turn.id = sqlite3_column_int(stmt, 0);
    turn.turn_index = sqlite3_column_int(stmt, 1);
    turn.created_at = ColumnText(stmt, 2);
    turn.story = ColumnText(stmt, 3);
    turn.choices[0].text = ColumnText(stmt, 4);
    turn.choices[1].text = ColumnText(stmt, 5);
    turn.choices[2].text = ColumnText(stmt, 6);
    turn.choices[3].text = ColumnText(stmt, 7);
    turn.selected_label = ColumnText(stmt, 8);
    turn.selected_text = ColumnText(stmt, 9);
    turn.prompt = ColumnText(stmt, 10);
    turn.raw_response = ColumnText(stmt, 11);
    turn.error = ColumnText(stmt, 12);
    turn.latency_ms = sqlite3_column_int(stmt, 13);
    turns.push_back(turn);
  }
  sqlite3_finalize(stmt);
  return turns;
}

std::vector<HistoryEdge> HistoryStore::LoadEdges(const std::string &session_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<HistoryEdge> edges;
  if (!db_ || session_id.empty()) return edges;
  sqlite3_stmt *stmt = nullptr;
  const char *sql =
      "SELECT from_turn_id, to_turn_id, choice_label, choice_text FROM turn_edges "
      "WHERE session_id = ? ORDER BY id ASC;";
  if (sqlite3_prepare_v2(AsDb(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
    SetError(sqlite3_errmsg(AsDb(db_)));
    return edges;
  }
  BindText(stmt, 1, session_id);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    HistoryEdge edge;
    edge.from_turn_id = sqlite3_column_int(stmt, 0);
    edge.to_turn_id = sqlite3_column_int(stmt, 1);
    edge.choice_label = ColumnText(stmt, 2);
    edge.choice_text = ColumnText(stmt, 3);
    edges.push_back(edge);
  }
  sqlite3_finalize(stmt);
  return edges;
}

void HistoryStore::TouchSession(const std::string &session_id) {
  if (!db_ || session_id.empty()) return;
  sqlite3_stmt *stmt = nullptr;
  const char *sql = "UPDATE sessions SET updated_at = ? WHERE id = ?;";
  if (sqlite3_prepare_v2(AsDb(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
    SetError(sqlite3_errmsg(AsDb(db_)));
    return;
  }
  BindText(stmt, 1, NowIso());
  BindText(stmt, 2, session_id);
  if (sqlite3_step(stmt) != SQLITE_DONE) SetError(sqlite3_errmsg(AsDb(db_)));
  sqlite3_finalize(stmt);
}
