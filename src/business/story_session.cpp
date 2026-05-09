#include "business/story_session.h"

StorySession::StorySession() : config_(LoadAiConfig("experiment_config.json")) {}

StorySession::~StorySession() {
  if (worker_.joinable()) worker_.join();
}

void StorySession::EnsureStarted() {
  bool should_start = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    should_start = !started_ && !loading_;
  }
  if (should_start) StartRequest(nullptr);
}

void StorySession::StartNew(const StorySetup &setup) {
  if (worker_.joinable()) worker_.join();
  {
    std::lock_guard<std::mutex> lock(mutex_);
    setup_ = setup;
    session_id_ = history_.StartSession(config_.model, config_.world);
    history_.InsertStorySetup(session_id_, setup_);
    story_ = "正在根据你的关键词和风格生成开场...";
    choices_ = {Choice{'A', ""}, Choice{'B', ""}, Choice{'X', ""}, Choice{'Y', ""}};
    last_action_ = StorySetupSummary(setup_);
    error_.clear();
    recent_story_.clear();
    last_choice_ = {};
    pending_edge_choice_ = {};
    turn_index_ = 0;
    current_turn_id_ = 0;
    pending_edge_from_turn_id_ = 0;
    has_last_choice_ = false;
    has_pending_edge_ = false;
    started_ = false;
    loading_ = false;
  }
  EnsureStarted();
}

std::string StorySession::Story() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return story_;
}

std::array<Choice, 4> StorySession::Choices() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return choices_;
}

std::string StorySession::LastAction() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_action_;
}

bool StorySession::Loading() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return loading_;
}

std::string StorySession::Error() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return error_;
}

void StorySession::Choose(char label) {
  std::array<Choice, 4> choices;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (loading_) return;
    started_ = true;
    choices = choices_;
  }
  for (const Choice &choice : choices) {
    if (choice.label == label) {
      StartRequest(&choice);
      return;
    }
  }
}

void StorySession::StartRequest(const Choice *choice) {
  Choice selected{};
  int selected_turn_id = 0;
  AiTurnRequest request;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (loading_) return;
    if (choice) {
      selected = *choice;
      request.last_choice = selected;
      request.has_last_choice = true;
      last_action_ = std::string("Selected ") + selected.label + ": " + selected.text;
      selected_turn_id = current_turn_id_;
      if (current_turn_id_ > 0) {
        pending_edge_from_turn_id_ = current_turn_id_;
        pending_edge_choice_ = selected;
        has_pending_edge_ = true;
      }
    } else if (has_last_choice_) {
      request.last_choice = last_choice_;
      request.has_last_choice = true;
    }
    request.setup = setup_;
    request.recent_story = recent_story_;
    started_ = true;
    loading_ = true;
    error_.clear();
  }

  if (selected_turn_id > 0) history_.UpdateTurnSelection(selected_turn_id, selected);
  if (worker_.joinable()) worker_.join();
  worker_ = std::thread([this, request, selected, has_choice = choice != nullptr]() {
    AiTurnResult result = GenerateStoryTurn(config_, request);
    const Choice *choice_ptr = has_choice ? &selected : nullptr;
    ApplyResult(result, choice_ptr);
  });
}

void StorySession::ApplyResult(const AiTurnResult &result, const Choice *choice) {
  std::lock_guard<std::mutex> lock(mutex_);
  loading_ = false;

  const int new_turn_id =
      history_.InsertTurn(session_id_, ++turn_index_, result.story, result.choices, result.prompt, result.raw_response,
                          result.error, result.latency_ms);

  if (!result.ok) {
    error_ = result.error.empty() ? "Story generation failed." : result.error;
    return;
  }
  if (turn_index_ == 1 && new_turn_id > 0) history_.UpdateStorySetupInitialTurn(session_id_, new_turn_id);

  if (has_pending_edge_ && pending_edge_from_turn_id_ > 0 && new_turn_id > 0) {
    history_.InsertEdge(session_id_, pending_edge_from_turn_id_, new_turn_id, pending_edge_choice_);
    has_pending_edge_ = false;
    pending_edge_from_turn_id_ = 0;
  }

  story_ = result.story;
  choices_ = result.choices;
  current_turn_id_ = new_turn_id;
  recent_story_.push_back(story_);
  if (recent_story_.size() > 2) recent_story_.erase(recent_story_.begin());
  if (choice) {
    last_choice_ = *choice;
    has_last_choice_ = true;
  }
  error_.clear();
}
