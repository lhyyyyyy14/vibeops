#pragma once

#include <SDL.h>

#include <string>
#include <utility>
#include <vector>

struct FooterButtonHint {
  std::string key;
  std::string label;
};

bool InitUiText();
void ShutdownUiText();
void ClearScreen(SDL_Renderer *renderer, SDL_Color color);
void DrawPanel(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color fill, SDL_Color border);
void DrawText(SDL_Renderer *renderer, int x, int y, const std::string &text, int scale, SDL_Color color);
void DrawTextWrapped(SDL_Renderer *renderer, int x, int y, int max_width, int line_height, const std::string &text,
                     int scale, SDL_Color color, int max_lines = 0);
void DrawFooterHint(SDL_Renderer *renderer, const std::string &text);

SDL_Color UiDark();
SDL_Color UiPaper();
SDL_Color UiPaperAlt();
SDL_Color UiInk();
SDL_Color UiMuted();
SDL_Color UiLine();
SDL_Color UiAccent();
SDL_Color UiGreen();
SDL_Color UiRed();

void DrawAppShell(SDL_Renderer *renderer, const std::string &title, const std::string &center_status,
                  const std::string &right_status);
void DrawHeader(SDL_Renderer *renderer, const std::string &title, const std::string &center_status,
                const std::string &right_status);
void DrawFooterButtons(SDL_Renderer *renderer, const std::vector<FooterButtonHint> &buttons);
void DrawSectionPanel(SDL_Renderer *renderer, const SDL_Rect &rect, const std::string &title);
void DrawTextBox(SDL_Renderer *renderer, const SDL_Rect &rect, const std::string &title, const std::string &text,
                 int max_lines);
void DrawMenuItem(SDL_Renderer *renderer, const SDL_Rect &rect, const std::string &label, const std::string &meta,
                  bool selected);
void DrawChoiceRow(SDL_Renderer *renderer, const SDL_Rect &rect, char label, const std::string &text, bool selected);
void DrawTabs(SDL_Renderer *renderer, int x, int y, const std::vector<std::string> &tabs, int active);
void DrawDashedLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, SDL_Color color);
