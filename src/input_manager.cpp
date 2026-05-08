#include "input_manager.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>

namespace {
std::string Trim(std::string value) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

std::string Upper(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return value;
}
}  // namespace

const char *ButtonName(Button button) {
  switch (button) {
  case Button::Up: return "UP";
  case Button::Down: return "DOWN";
  case Button::Left: return "LEFT";
  case Button::Right: return "RIGHT";
  case Button::A: return "A";
  case Button::B: return "B";
  case Button::X: return "X";
  case Button::Y: return "Y";
  case Button::L1: return "L1";
  case Button::R1: return "R1";
  case Button::Start: return "START";
  case Button::Select: return "SELECT";
  case Button::Menu: return "MENU";
  default: return "INVALID";
  }
}

void InputManager::LoadOverrides(const std::string &path) {
  controller_map_.fill(InvalidButton());
  joy_map_.fill(InvalidButton());
  controller_map_[SDL_CONTROLLER_BUTTON_A] = Button::A;
  controller_map_[SDL_CONTROLLER_BUTTON_B] = Button::B;
  controller_map_[SDL_CONTROLLER_BUTTON_X] = Button::X;
  controller_map_[SDL_CONTROLLER_BUTTON_Y] = Button::Y;
  controller_map_[SDL_CONTROLLER_BUTTON_DPAD_UP] = Button::Up;
  controller_map_[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = Button::Down;
  controller_map_[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = Button::Left;
  controller_map_[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = Button::Right;
  controller_map_[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = Button::L1;
  controller_map_[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = Button::R1;
  controller_map_[SDL_CONTROLLER_BUTTON_START] = Button::Start;
  controller_map_[SDL_CONTROLLER_BUTTON_BACK] = Button::Select;
  joy_map_[0] = Button::A;
  joy_map_[1] = Button::B;
  joy_map_[2] = Button::X;
  joy_map_[3] = Button::Y;
  joy_map_[4] = Button::L1;
  joy_map_[5] = Button::R1;
  joy_map_[6] = Button::Select;
  joy_map_[7] = Button::Start;

  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    line = Trim(line);
    if (line.empty() || line[0] == '#' || line[0] == ';') continue;
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string key = Trim(line.substr(0, eq));
    Button button = InvalidButton();
    if (!ParseButtonName(Trim(line.substr(eq + 1)), button)) continue;
    const std::size_t dot = key.find('.');
    if (dot == std::string::npos) continue;
    const std::string device = key.substr(0, dot);
    const int index = std::atoi(key.substr(dot + 1).c_str());
    if (index < 0 || index >= 32) continue;
    if (device == "pad") controller_map_[static_cast<std::size_t>(index)] = button;
    if (device == "joy") joy_map_[static_cast<std::size_t>(index)] = button;
  }
}

void InputManager::BeginFrame() {
  for (auto &state : states_) {
    state.just_pressed = false;
    state.just_released = false;
  }
}

void InputManager::HandleEvent(const SDL_Event &event) {
  if (event.type == SDL_KEYDOWN && !event.key.repeat) {
    SetDown(KeyToButton(event.key.keysym.sym), true);
  } else if (event.type == SDL_KEYUP) {
    SetDown(KeyToButton(event.key.keysym.sym), false);
  } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
    SetDown(ControllerButtonToButton(event.cbutton.button), true);
  } else if (event.type == SDL_CONTROLLERBUTTONUP) {
    SetDown(ControllerButtonToButton(event.cbutton.button), false);
  } else if (event.type == SDL_JOYBUTTONDOWN) {
    SetDown(JoyButtonToButton(event.jbutton.button), true);
  } else if (event.type == SDL_JOYBUTTONUP) {
    SetDown(JoyButtonToButton(event.jbutton.button), false);
  } else if (event.type == SDL_JOYHATMOTION) {
    SetDown(Button::Up, (event.jhat.value & SDL_HAT_UP) != 0);
    SetDown(Button::Down, (event.jhat.value & SDL_HAT_DOWN) != 0);
    SetDown(Button::Left, (event.jhat.value & SDL_HAT_LEFT) != 0);
    SetDown(Button::Right, (event.jhat.value & SDL_HAT_RIGHT) != 0);
  } else if (event.type == SDL_CONTROLLERAXISMOTION) {
    constexpr int kDeadzone = 16000;
    if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
      SetDown(Button::Left, event.caxis.value < -kDeadzone);
      SetDown(Button::Right, event.caxis.value > kDeadzone);
    }
    if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
      SetDown(Button::Up, event.caxis.value < -kDeadzone);
      SetDown(Button::Down, event.caxis.value > kDeadzone);
    }
  }
}

bool InputManager::IsPressed(Button button) const {
  return IsValid(button) && states_[static_cast<int>(button)].down;
}

bool InputManager::IsJustPressed(Button button) const {
  return IsValid(button) && states_[static_cast<int>(button)].just_pressed;
}

bool InputManager::IsJustReleased(Button button) const {
  return IsValid(button) && states_[static_cast<int>(button)].just_released;
}

void InputManager::SetDown(Button button, bool down) {
  if (!IsValid(button)) return;
  ButtonState &state = states_[static_cast<int>(button)];
  if (state.down == down) return;
  state.down = down;
  state.just_pressed = down;
  state.just_released = !down;
}

Button InputManager::KeyToButton(SDL_Keycode key) const {
  switch (key) {
  case SDLK_UP: return Button::Up;
  case SDLK_DOWN: return Button::Down;
  case SDLK_LEFT: return Button::Left;
  case SDLK_RIGHT: return Button::Right;
  case SDLK_SPACE: return Button::A;
  case SDLK_RETURN: return Button::A;
  case SDLK_z: return Button::A;
  case SDLK_x: return Button::B;
  case SDLK_a: return Button::X;
  case SDLK_s: return Button::Y;
  case SDLK_q: return Button::L1;
  case SDLK_e: return Button::R1;
  case SDLK_TAB: return Button::Start;
  case SDLK_BACKSPACE: return Button::Select;
  case SDLK_ESCAPE: return Button::Menu;
  default: return InvalidButton();
  }
}

Button InputManager::ControllerButtonToButton(Uint8 button) const {
  if (button >= controller_map_.size()) return InvalidButton();
  return controller_map_[button];
}

Button InputManager::JoyButtonToButton(Uint8 button) const {
  if (button >= joy_map_.size()) return InvalidButton();
  return joy_map_[button];
}

Button InputManager::InvalidButton() { return Button::Count; }

bool InputManager::IsValid(Button button) {
  return static_cast<int>(button) >= 0 && static_cast<int>(button) < static_cast<int>(Button::Count);
}

bool InputManager::ParseButtonName(const std::string &name, Button &out) {
  const std::string upper = Upper(name);
  for (int i = 0; i < static_cast<int>(Button::Count); ++i) {
    const Button button = static_cast<Button>(i);
    if (upper == ButtonName(button)) {
      out = button;
      return true;
    }
  }
  if (upper == "NONE") {
    out = InvalidButton();
    return true;
  }
  return false;
}
