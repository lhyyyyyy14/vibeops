#pragma once

#include <SDL.h>

#include <array>
#include <string>

enum class Button {
  Up,
  Down,
  Left,
  Right,
  A,
  B,
  X,
  Y,
  L1,
  R1,
  Start,
  Select,
  Menu,
  Count,
};

constexpr int kButtonCount = static_cast<int>(Button::Count);

const char *ButtonName(Button button);

struct ButtonState {
  bool down = false;
  bool just_pressed = false;
  bool just_released = false;
};

class InputManager {
public:
  void LoadOverrides(const std::string &path);
  void BeginFrame();
  void HandleEvent(const SDL_Event &event);

  bool IsPressed(Button button) const;
  bool IsJustPressed(Button button) const;
  bool IsJustReleased(Button button) const;

private:
  static constexpr int kMappedButtonCount = static_cast<int>(Button::Count);

  void SetDown(Button button, bool down);
  Button KeyToButton(SDL_Keycode key) const;
  Button ControllerButtonToButton(Uint8 button) const;
  Button JoyButtonToButton(Uint8 button) const;
  static Button InvalidButton();
  static bool IsValid(Button button);
  static bool ParseButtonName(const std::string &name, Button &out);

  std::array<ButtonState, kMappedButtonCount> states_{};
  std::array<Button, 32> controller_map_{};
  std::array<Button, 32> joy_map_{};
};
