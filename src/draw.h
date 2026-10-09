// The drawing helpers of the components: the look of the Command Deck (glow, glass panels, gradient rings and area fills, resource icons).
// Everything reads the frame state of the element being drawn through g_ctx: the layout scale, the host's theme, fonts and time.
#pragma once
#include <windows.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"  // IM_PI, and the window's work rectangle that containers narrow
#include "stellaris_gui_api.h"

namespace argon {

extern const StlGuiCallbackCtx* g_ctx;  // the context of the element being drawn (set by the wrapper of every element)

inline float S() { return g_ctx->ui_scale; }  // layout scale: multiply every pixel size with it
inline float Fit() { return g_ctx->fit; }
inline float Dt() { return g_ctx->delta_time; }
inline double Tm() { return g_ctx->time; }
inline ImFont* FontBody() { return (ImFont*)g_ctx->font_body; }
inline ImFont* FontBold() { return (ImFont*)g_ctx->font_bold; }
inline ImFont* FontTitle() { return (ImFont*)g_ctx->font_title; }
inline ImFont* FontNum() { return (ImFont*)g_ctx->font_numbers_large; }
inline ImFont* FontNumS() { return (ImFont*)g_ctx->font_numbers; }

#define C(r, g, b, a) IM_COL32(r, g, b, a)
inline ImU32 Acc() { return g_ctx->theme->accent; }
inline ImU32 Acc2() { return g_ctx->theme->accent2; }
inline ImU32 ColText() { return g_ctx->theme->text; }
inline ImU32 ColDim() { return g_ctx->theme->text_dim; }
inline ImU32 ColGood() { return g_ctx->theme->good; }
inline ImU32 ColBad() { return g_ctx->theme->bad; }
inline ImU32 ColWarn() { return g_ctx->theme->warn; }

inline ImVec2 operator+(ImVec2 a, ImVec2 b) { return ImVec2(a.x + b.x, a.y + b.y); }
inline ImVec2 operator-(ImVec2 a, ImVec2 b) { return ImVec2(a.x - b.x, a.y - b.y); }
inline ImVec2 operator*(ImVec2 a, float s) { return ImVec2(a.x * s, a.y * s); }

ImU32 Al(ImU32 c, float a);              // alpha of the colour times a
ImU32 Mix(ImU32 x, ImU32 y, float t);    // linear mix
ImU32 ResColor(const std::string& resource_key);  // the colour a resource is drawn in
void Fmt(char* out, size_t n, double v, bool sign = false);  // 12.3k, 4.5M, +7.0

// An animated value that eases to `target`: one per (owner, name), kept for the lifetime of the plugin.
float Smooth(const void* owner, const char* name, float target, float rate = 12.f);

ImVec2 TextSz(ImFont* f, float k, const char* s);
void Txt(ImDrawList* dl, ImFont* f, float k, ImVec2 p, ImU32 col, const char* s, int align = 0);  // align: 0 left, 1 centre, 2 right of p.x
void TxtF(ImDrawList* dl, ImFont* f, float k, ImVec2 p, ImU32 col, int align, const char* fmt, ...);

void Glow(ImDrawList* dl, ImVec2 c, float r, ImU32 col, float strength);
void Stars(ImDrawList* dl, ImVec2 a, ImVec2 b, double t);
void Panel(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, ImU32 accent, float alpha = 1.f);  // a glass card
void CardTitle(ImDrawList* dl, ImVec2 a, const char* small, const char* big);
void AreaFill(ImDrawList* dl, const ImVec2* p, int n, float base_y, ImU32 top, ImU32 bottom);
void Ring(ImDrawList* dl, ImVec2 c, float r, float th, float frac, ImU32 ca, ImU32 cb);  // a 270 degree gauge
void Icon(ImDrawList* dl, const std::string& key, ImVec2 c, float r, ImU32 col);          // a resource icon from primitives
void TabIcon(ImDrawList* dl, int tab, ImVec2 c, float r, ImU32 col);                      // 0 overview 1 economy 2 time 3 script 4 settings
bool Hit(ImVec2 a, ImVec2 b, const char* id);  // an invisible button over the rectangle: true when clicked

}  // namespace argon
