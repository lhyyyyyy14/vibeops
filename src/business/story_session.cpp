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
  AiTurnRequest request;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (loading_) return;
    if (choice) {
      selected = *choice;
      request.last_choice = selected;
      request.has_last_choice = true;
      last_action_ = std::string("已选择 ") + selected.label + "： " + selected.text;
    } else if (has_last_choice_) {
      request.last_choice = last_choice_;
      request.has_last_choice = true;
    }
    request.recent_story = recent_story_;
    started_ = true;
    loading_ = true;
    error_.clear();
  }

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
  if (!result.ok) {
    error_ = result.error;
    if (error_.empty()) error_ = "生成失败：未知错误";
    return;
  }
  story_ = result.story;
  choices_ = result.choices;
  recent_story_.push_back(story_);
  if (recent_story_.size() > 2) recent_story_.erase(recent_story_.begin());
  if (choice) {
    last_choice_ = *choice;
    has_last_choice_ = true;
  }
  error_.clear();
}
