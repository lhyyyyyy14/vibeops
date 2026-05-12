#include "ui_draw.h"

#include "layout_metrics.h"

#ifdef HAVE_SDL2_TTF
#include <SDL_ttf.h>
#endif

#include <algorithm>
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

int PointSizeForScale(int scale) { return std::max(12, scale * 8); }

bool IsUtf8Continuation(unsigned char c) { return (c & 0xC0) == 0x80; }

std::string NextUtf8Char(const std::string &text, std::size_t *index) {
  if (*index >= text.size()) return "";
  const unsigned char c = static_cast<unsigned char>(text[*index]);
  int char_len = 1;
  if ((c & 0xF0) == 0xE0 && *index + 2 < text.size()) {
    char_len = 3;
  } else if ((c & 0xE0) == 0xC0 && *index + 1 < text.size()) {
    char_len = 2;
  } else if ((c & 0xF8) == 0xF0 && *index + 3 < text.size()) {
    char_len = 4;
  }
  std::string out = text.substr(*index, static_cast<std::size_t>(char_len));
  *index += static_cast<std::size_t>(char_len);
  return out;
}

int EstimateTextWidth(const std::string &text, int scale) {
#ifdef HAVE_SDL2_TTF
  if (TTF_WasInit()) {
    if (TTF_Font *font = GetFont(PointSizeForScale(scale))) {
      int w = 0;
      int h = 0;
      if (TTF_SizeUTF8(font, text.c_str(), &w, &h) == 0) return w;
    }
  }
#endif
  int width = 0;
  for (std::size_t i = 0; i < text.size();) {
    const unsigned char c = static_cast<unsigned char>(text[i]);
    if (c < 0x80) {
      width += scale * 6;
      ++i;
    } else {
      width += scale * 10;
      ++i;
      while (i < text.size() && IsUtf8Continuation(static_cast<unsigned char>(text[i]))) ++i;
    }
  }
  return width;
}

void DrawPaperTexture(SDL_Renderer *renderer, const SDL_Rect &rect) {
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
  for (int y = rect.y + 4; y < rect.y + rect.h; y += 16) {
    SDL_RenderDrawLine(renderer, rect.x + 4, y, rect.x + rect.w - 5, y);
  }
  SDL_SetRenderDrawColor(renderer, 84, 140, 204, 28);
  for (int x = rect.x + 4; x < rect.x + rect.w; x += 16) {
    SDL_RenderDrawLine(renderer, x, rect.y + 4, x, rect.y + rect.h - 5);
  }
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void DrawThickRect(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color color, int thickness) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  for (int i = 0; i < thickness; ++i) {
    SDL_Rect r{rect.x + i, rect.y + i, rect.w - i * 2, rect.h - i * 2};
    SDL_RenderDrawRect(renderer, &r);
  }
}

std::string FitWithEllipsis(const std::string &text, int max_width, int scale) {
  if (EstimateTextWidth(text, scale) <= max_width) return text;
  std::string out;
  for (std::size_t i = 0; i < text.size();) {
    const std::size_t before = i;
    std::string ch = NextUtf8Char(text, &i);
    if (EstimateTextWidth(out + ch + "...", scale) > max_width) break;
    out += ch;
    if (before == i) break;
  }
  return out + "...";
}

void DrawTextClipped(SDL_Renderer *renderer, const SDL_Rect &clip, int x, int y, const std::string &text, int scale,
                     SDL_Color color) {
  SDL_RenderSetClipRect(renderer, &clip);
  DrawText(renderer, x, y, FitWithEllipsis(text, clip.w - std::max(0, x - clip.x), scale), scale, color);
  SDL_RenderSetClipRect(renderer, nullptr);
}

int DrawButtonBadge(SDL_Renderer *renderer, int x, int y, const std::string &key, const std::string &label) {
  const int key_w = std::max(28, EstimateTextWidth(key, 2) + 14);
  const SDL_Rect badge{x, y, key_w, 28};
  DrawPanel(renderer, badge, SDL_Color{255, 255, 255, 255}, UiDark());
  DrawThickRect(renderer, badge, UiDark(), 2);
  DrawTextClipped(renderer, badge, x + 7, y + 4, key, 2, UiDark());
  const int label_w = EstimateTextWidth(label, 2);
  DrawText(renderer, x + key_w + 8, y + 5, label, 2, SDL_Color{255, 255, 255, 255});
  return key_w + 8 + label_w;
}

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
    const int point_size = PointSizeForScale(scale);
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
                     int scale, SDL_Color color, int max_lines) {
  int cursor_y = y;
  int lines_drawn = 0;
  std::string line;
  for (std::size_t i = 0; i < text.size();) {
    const bool newline = text[i] == '\n';
    std::size_t next = i;
    std::string ch = newline ? "" : NextUtf8Char(text, &next);
    const std::string candidate = line + ch;
    if (newline || (!line.empty() && EstimateTextWidth(candidate, scale) > max_width)) {
      if (max_lines > 0 && lines_drawn >= max_lines - 1) {
        DrawText(renderer, x, cursor_y, FitWithEllipsis(line + (newline ? "" : ch), max_width, scale), scale, color);
        return;
      }
      DrawText(renderer, x, cursor_y, line, scale, color);
      line.clear();
      cursor_y += line_height;
      ++lines_drawn;
      if (newline) {
        ++i;
        continue;
      }
    }
    line.append(ch);
    i = next;
  }
  if (!line.empty() && (max_lines <= 0 || lines_drawn < max_lines)) {
    DrawText(renderer, x, cursor_y, FitWithEllipsis(line, max_width, scale), scale, color);
  }
}

void DrawFooterHint(SDL_Renderer *renderer, const std::string &text) {
  const LayoutMetrics &layout = Layout();
  DrawPanel(renderer, SDL_Rect{24, layout.screen_h - 60, layout.screen_w - 48, 42}, UiDark(), UiLine());
  DrawTextWrapped(renderer, layout.safe_margin_x + 22, layout.screen_h - 50, layout.screen_w - 92, 20, text, 2,
                  SDL_Color{255, 255, 255, 255}, 1);
}

SDL_Color UiDark() { return SDL_Color{24, 31, 58, 255}; }
SDL_Color UiPaper() { return SDL_Color{244, 248, 255, 255}; }
SDL_Color UiPaperAlt() { return SDL_Color{255, 255, 255, 255}; }
SDL_Color UiInk() { return SDL_Color{28, 30, 38, 255}; }
SDL_Color UiMuted() { return SDL_Color{87, 95, 116, 255}; }
SDL_Color UiLine() { return SDL_Color{18, 24, 42, 255}; }
SDL_Color UiAccent() { return SDL_Color{44, 97, 178, 255}; }
SDL_Color UiGreen() { return SDL_Color{34, 139, 76, 255}; }
SDL_Color UiRed() { return SDL_Color{204, 54, 54, 255}; }

void DrawDashedLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  const bool horizontal = y1 == y2;
  const int length = horizontal ? std::abs(x2 - x1) : std::abs(y2 - y1);
  const int sign = horizontal ? (x2 >= x1 ? 1 : -1) : (y2 >= y1 ? 1 : -1);
  for (int step = 0; step < length; step += 8) {
    const int dash = std::min(4, length - step);
    if (horizontal) {
      SDL_RenderDrawLine(renderer, x1 + sign * step, y1, x1 + sign * (step + dash), y1);
    } else {
      SDL_RenderDrawLine(renderer, x1, y1 + sign * step, x1, y1 + sign * (step + dash));
    }
  }
}

void DrawHeader(SDL_Renderer *renderer, const std::string &title, const std::string &center_status,
                const std::string &right_status) {
  const SDL_Rect header{24, 20, 672, 44};
  DrawPanel(renderer, header, UiDark(), UiLine());
  DrawThickRect(renderer, header, UiLine(), 3);
  DrawTextClipped(renderer, SDL_Rect{42, 28, 236, 26}, 42, 32, title, 2, SDL_Color{255, 255, 255, 255});
  DrawTextClipped(renderer, SDL_Rect{282, 28, 250, 26}, 282, 32, center_status, 2, SDL_Color{215, 232, 255, 255});
  DrawTextClipped(renderer, SDL_Rect{570, 28, 106, 26}, 570, 32, right_status, 2, SDL_Color{255, 230, 132, 255});
}

void DrawAppShell(SDL_Renderer *renderer, const std::string &title, const std::string &center_status,
                  const std::string &right_status) {
  ClearScreen(renderer, SDL_Color{132, 184, 226, 255});
  DrawPanel(renderer, SDL_Rect{0, 0, 720, 480}, SDL_Color{132, 184, 226, 255}, SDL_Color{132, 184, 226, 255});
  DrawPaperTexture(renderer, SDL_Rect{0, 0, 720, 480});
  DrawPanel(renderer, SDL_Rect{10, 10, 700, 460}, SDL_Color{204, 224, 244, 255}, UiLine());
  DrawThickRect(renderer, SDL_Rect{10, 10, 700, 460}, UiLine(), 4);
  DrawHeader(renderer, title, center_status, right_status);
}

void DrawFooterButtons(SDL_Renderer *renderer, const std::vector<FooterButtonHint> &buttons) {
  const SDL_Rect footer{24, 420, 672, 42};
  DrawPanel(renderer, footer, UiDark(), UiLine());
  DrawThickRect(renderer, footer, UiLine(), 3);
  int x = 42;
  for (const FooterButtonHint &button : buttons) {
    const int used = DrawButtonBadge(renderer, x, 427, button.key, button.label);
    x += used + 22;
    if (x > 650) break;
  }
}

void DrawSectionPanel(SDL_Renderer *renderer, const SDL_Rect &rect, const std::string &title) {
  DrawPanel(renderer, rect, UiPaperAlt(), UiLine());
  DrawThickRect(renderer, rect, UiLine(), 3);
  if (!title.empty()) {
    const SDL_Rect title_bar{rect.x + 3, rect.y + 3, rect.w - 6, 28};
    DrawPanel(renderer, title_bar, UiAccent(), UiAccent());
    DrawTextClipped(renderer, SDL_Rect{title_bar.x + 10, title_bar.y + 2, title_bar.w - 20, 24}, title_bar.x + 10,
                    title_bar.y + 6, title, 2, SDL_Color{255, 255, 255, 255});
  }
}

void DrawTextBox(SDL_Renderer *renderer, const SDL_Rect &rect, const std::string &title, const std::string &text,
                 int max_lines) {
  DrawSectionPanel(renderer, rect, title);
  const int text_y = title.empty() ? rect.y + 14 : rect.y + 42;
  SDL_Rect clip{rect.x + 14, text_y, rect.w - 28, rect.h - (text_y - rect.y) - 12};
  SDL_RenderSetClipRect(renderer, &clip);
  DrawTextWrapped(renderer, clip.x, clip.y, clip.w, 21, text, 2, UiInk(), max_lines);
  SDL_RenderSetClipRect(renderer, nullptr);
}

void DrawMenuItem(SDL_Renderer *renderer, const SDL_Rect &rect, const std::string &label, const std::string &meta,
                  bool selected) {
  const SDL_Color fill = selected ? SDL_Color{232, 241, 255, 255} : UiPaperAlt();
  DrawPanel(renderer, rect, fill, UiLine());
  DrawThickRect(renderer, rect, selected ? UiAccent() : UiLine(), selected ? 3 : 2);
  const int content_y = rect.y + std::max(3, (rect.h - 18) / 2);
  if (selected) DrawText(renderer, rect.x + 12, content_y, ">", 2, UiAccent());
  const SDL_Rect label_clip{rect.x + 34, rect.y + 4, rect.w - 92, rect.h - 8};
  DrawTextClipped(renderer, label_clip, rect.x + 34, content_y, label, 2, selected ? UiInk() : UiMuted());
  if (!meta.empty()) {
    const int meta_w = std::min(58, EstimateTextWidth(meta, 2));
    const SDL_Rect meta_clip{rect.x + rect.w - meta_w - 14, rect.y + 4, meta_w + 4, rect.h - 8};
    DrawTextClipped(renderer, meta_clip, meta_clip.x, content_y, meta, 2, selected ? UiAccent() : UiMuted());
  }
}

void DrawChoiceRow(SDL_Renderer *renderer, const SDL_Rect &rect, char label, const std::string &text, bool selected) {
  DrawPanel(renderer, rect, selected ? SDL_Color{232, 241, 255, 255} : UiPaperAlt(), UiLine());
  DrawThickRect(renderer, rect, selected ? UiAccent() : UiLine(), selected ? 3 : 2);
  const int content_y = rect.y + std::max(3, (rect.h - 18) / 2);
  DrawPanel(renderer, SDL_Rect{rect.x + 8, rect.y + 5, 28, rect.h - 10}, selected ? UiAccent() : UiDark(),
            selected ? UiAccent() : UiDark());
  DrawText(renderer, rect.x + 17, content_y, std::string(1, label), 2, SDL_Color{255, 255, 255, 255});
  SDL_Rect clip{rect.x + 48, rect.y + 4, rect.w - 60, rect.h - 8};
  SDL_RenderSetClipRect(renderer, &clip);
  DrawTextWrapped(renderer, clip.x, content_y, clip.w, 20, text, 2, selected ? UiInk() : UiMuted(), 1);
  SDL_RenderSetClipRect(renderer, nullptr);
}

void DrawTabs(SDL_Renderer *renderer, int x, int y, const std::vector<std::string> &tabs, int active) {
  int cursor = x;
  for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
    const int w = std::max(76, EstimateTextWidth(tabs[static_cast<std::size_t>(i)], 2) + 28);
    const SDL_Rect tab{cursor, y, w, 30};
    DrawPanel(renderer, tab, i == active ? UiAccent() : UiPaperAlt(), UiLine());
    DrawThickRect(renderer, tab, UiLine(), 2);
    DrawTextClipped(renderer, SDL_Rect{cursor + 12, y + 3, w - 24, 24}, cursor + 12, y + 7,
                    tabs[static_cast<std::size_t>(i)], 2,
                    i == active ? SDL_Color{255, 255, 255, 255} : UiMuted());
    cursor += w + 8;
  }
}
