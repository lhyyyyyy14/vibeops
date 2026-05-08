#pragma once

#include <SDL.h>

#include <string>

bool InitUiText();
void ShutdownUiText();
void ClearScreen(SDL_Renderer *renderer, SDL_Color color);
void DrawPanel(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color fill, SDL_Color border);
void DrawText(SDL_Renderer *renderer, int x, int y, const std::string &text, int scale, SDL_Color color);
void DrawTextWrapped(SDL_Renderer *renderer, int x, int y, int max_width, int line_height, const std::string &text,
                     int scale, SDL_Color color);
void DrawFooterHint(SDL_Renderer *renderer, const std::string &text);
