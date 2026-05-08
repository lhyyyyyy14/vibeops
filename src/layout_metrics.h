#pragma once

struct LayoutMetrics {
  int screen_w = 720;
  int screen_h = 480;
  int safe_margin_x = 24;
  int safe_margin_y = 18;
  int header_h = 58;
  int footer_h = 42;
  int panel_gap = 14;
  int list_item_h = 48;
  int option_h = 54;
};

const LayoutMetrics &Layout();
