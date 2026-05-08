#include "ui_draw.h"

#include "layout_metrics.h"

#ifdef HAVE_SDL2_TTF
#include <SDL_ttf.h>
#endif

#include <array>
#include <cctype>
#include <map>
#include <sstream>
#include <vector>

namespace {
using Glyph = std::array<const char *, 7>;

#ifdef HAVE_SDL2_TTF
std::map<int, TTF_Font *> g_fonts;

std::vector<std::string> FontCandidates() {
  return {
      "assets/ui_font.ttf",
      "C:/Windows/Fonts/msyh.ttc",
      "C:/Windows/Fonts/simhei.ttf",
      "C:/Windows/Fonts/simsun.ttc",
      "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
      "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
  };
}

TTF_Font *GetFont(int point_size) {
  auto it = g_fonts.find(point_size);
  if (it != g_fonts.end()) return it->second;
  for (const std::string &path : FontCandidates()) {
    TTF_Font *font = TTF_OpenFont(path.c_str(), point_size);
    if (font) {
      g_fonts[point_size] = font;
      return font;
    }
  }
  g_fonts[point_size] = nullptr;
  return nullptr;
}
#endif

Glyph GlyphFor(char ch) {
  switch (static_cast<char>(std::toupper(static_cast<unsigned char>(ch)))) {
  case 'A': return {"01110", "10001", "10001", "11111", "10001", "10001", "10001"};
  case 'B': return {"11110", "10001", "10001", "11110", "10001", "10001", "11110"};
  case 'C': return {"01111", "10000", "10000", "10000", "10000", "10000", "01111"};
  case 'D': return {"11110", "10001", "10001", "10001", "10001", "10001", "11110"};
  case 'E': return {"11111", "10000", "10000", "11110", "10000", "10000", "11111"};
  case 'F': return {"11111", "10000", "10000", "11110", "10000", "10000", "10000"};
  case 'G': return {"01111", "10000", "10000", "10111", "10001", "10001", "01111"};
  case 'H': return {"10001", "10001", "10001", "11111", "10001", "10001", "10001"};
  case 'I': return {"11111", "00100", "00100", "00100", "00100", "00100", "11111"};
  case 'J': return {"00111", "00010", "00010", "00010", "10010", "10010", "01100"};
  case 'K': return {"10001", "10010", "10100", "11000", "10100", "10010", "10001"};
  case 'L': return {"10000", "10000", "10000", "10000", "10000", "10000", "11111"};
  case 'M': return {"10001", "11011", "10101", "10101", "10001", "10001", "10001"};
  case 'N': return {"10001", "11001", "10101", "10011", "10001", "10001", "10001"};
  case 'O': return {"01110", "10001", "10001", "10001", "10001", "10001", "01110"};
  case 'P': return {"11110", "10001", "10001", "11110", "10000", "10000", "10000"};
  case 'Q': return {"01110", "10001", "10001", "10001", "10101", "10010", "01101"};
  case 'R': return {"11110", "10001", "10001", "11110", "10100", "10010", "10001"};
  case 'S': return {"01111", "10000", "10000", "01110", "00001", "00001", "11110"};
  case 'T': return {"11111", "00100", "00100", "00100", "00100", "00100", "00100"};
  case 'U': return {"10001", "10001", "10001", "10001", "10001", "10001", "01110"};
  case 'V': return {"10001", "10001", "10001", "10001", "10001", "01010", "00100"};
  case 'W': return {"10001", "10001", "10001", "10101", "10101", "10101", "01010"};
  case 'X': return {"10001", "10001", "01010", "00100", "01010", "10001", "10001"};
  case 'Y': return {"10001", "10001", "01010", "00100", "00100", "00100", "00100"};
  case 'Z': return {"11111", "00001", "00010", "00100", "01000", "10000", "11111"};
  case '0': return {"01110", "10001", "10011", "10101", "11001", "10001", "01110"};
  case '1': return {"00100", "01100", "00100", "00100", "00100", "00100", "01110"};
  case '2': return {"01110", "10001", "00001", "00010", "00100", "01000", "11111"};
  case '3': return {"11110", "00001", "00001", "01110", "00001", "00001", "11110"};
  case '4': return {"10010", "10010", "10010", "11111", "00010", "00010", "00010"};
  case '5': return {"11111", "10000", "10000", "11110", "00001", "00001", "11110"};
  case '6': return {"01110", "10000", "10000", "11110", "10001", "10001", "01110"};
  case '7': return {"11111", "00001", "00010", "00100", "01000", "01000", "01000"};
  case '8': return {"01110", "10001", "10001", "01110", "10001", "10001", "01110"};
  case '9': return {"01110", "10001", "10001", "01111", "00001", "00001", "01110"};
  case '-': return {"00000", "00000", "00000", "11111", "00000", "00000", "00000"};
  case ':': return {"00000", "00100", "00100", "00000", "00100", "00100", "00000"};
  case '/': return {"00001", "00001", "00010", "00100", "01000", "10000", "10000"};
  case '.': return {"00000", "00000", "00000", "00000", "00000", "01100", "01100"};
  case ' ': return {"00000", "00000", "00000", "00000", "00000", "00000", "00000"};
  default: return {"11111", "10001", "00010", "00100", "00100", "00000", "00100"};
  }
}
}  // namespace

bool InitUiText() {
#ifdef HAVE_SDL2_TTF
  return TTF_Init() == 0;
#else
  return true;
#endif
}

void ShutdownUiText() {
#ifdef HAVE_SDL2_TTF
  for (auto &entry : g_fonts) {
    if (entry.second) TTF_CloseFont(entry.second);
  }
  g_fonts.clear();
  TTF_Quit();
#endif
}

void ClearScreen(SDL_Renderer *renderer, SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderClear(renderer);
}

void DrawPanel(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color fill, SDL_Color border) {
  SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
  SDL_RenderFillRect(renderer, &rect);
  SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
  SDL_RenderDrawRect(renderer, &rect);
}

void DrawText(SDL_Renderer *renderer, int x, int y, const std::string &text, int scale, SDL_Color color) {
#ifdef HAVE_SDL2_TTF
  if (TTF_WasInit()) {
    const int point_size = std::max(12, scale * 8);
    if (TTF_Font *font = GetFont(point_size)) {
      SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
      if (surface) {
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
          SDL_Rect dst{x, y, surface->w, surface->h};
          SDL_RenderCopy(renderer, texture, nullptr, &dst);
          SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
        return;
      }
    }
  }
#endif
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  int cursor_x = x;
  for (char ch : text) {
    const Glyph glyph = GlyphFor(ch);
    for (int row = 0; row < 7; ++row) {
      for (int col = 0; col < 5; ++col) {
        if (glyph[static_cast<std::size_t>(row)][col] != '1') continue;
        SDL_Rect pixel{cursor_x + col * scale, y + row * scale, scale, scale};
        SDL_RenderFillRect(renderer, &pixel);
      }
    }
    cursor_x += 6 * scale;
  }
}

void DrawTextWrapped(SDL_Renderer *renderer, int x, int y, int max_width, int line_height, const std::string &text,
                     int scale, SDL_Color color) {
  const int ascii_w = scale * 6;
  const int cjk_w = scale * 16;
  int cursor_w = 0;
  int cursor_y = y;
  std::string line;
  for (std::size_t i = 0; i < text.size();) {
    const unsigned char c = static_cast<unsigned char>(text[i]);
    int char_len = 1;
    int char_w = ascii_w;
    if ((c & 0xF0) == 0xE0 && i + 2 < text.size()) {
      char_len = 3;
      char_w = cjk_w;
    } else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
      char_len = 2;
      char_w = cjk_w;
    } else if ((c & 0xF8) == 0xF0 && i + 3 < text.size()) {
      char_len = 4;
      char_w = cjk_w;
    }
    if (text[i] == '\n' || (cursor_w > 0 && cursor_w + char_w > max_width)) {
      DrawText(renderer, x, cursor_y, line, scale, color);
      line.clear();
      cursor_w = 0;
      cursor_y += line_height;
      if (text[i] == '\n') {
        ++i;
        continue;
      }
    }
    line.append(text.substr(i, static_cast<std::size_t>(char_len)));
    cursor_w += char_w;
    i += static_cast<std::size_t>(char_len);
  }
  if (!line.empty()) DrawText(renderer, x, cursor_y, line, scale, color);
}

void DrawFooterHint(SDL_Renderer *renderer, const std::string &text) {
  const LayoutMetrics &layout = Layout();
  DrawPanel(renderer, SDL_Rect{0, layout.screen_h - layout.footer_h, layout.screen_w, layout.footer_h},
            SDL_Color{24, 27, 34, 255}, SDL_Color{62, 72, 92, 255});
  DrawText(renderer, layout.safe_margin_x, layout.screen_h - 29, text, 2, SDL_Color{188, 198, 215, 255});
}
