// What every component of the library shares: how an element is entered, the parameters of its declaration, the snapshot of the frame, and the layout
// helpers that let a container (card, row, tabs) put its content in a region of the window.
#pragma once
#include <string>
#include <vector>

#include "draw.h"
#include "imgui_internal.h"

namespace argon {

using ComponentFn = void (*)(const StlGuiNode* node);  // draws one entry; g_ctx is set
struct ComponentDef {
    const char* name;  // the key in declaration files
    ComponentFn draw;
};
const ComponentDef* Components(size_t* count);
// each file of components registers its own
void AddCoreComponents(std::vector<ComponentDef>& out);   // components.cpp: card, row, stat tile, ring, radar, chart, icon
void AddShellComponents(std::vector<ComponentDef>& out);  // components_shell.cpp: window, tabs, the parts of a HUD capsule
void AddPagesComponents(std::vector<ComponentDef>& out);  // components_pages.cpp: ledger, detail, speed dial, calendar, effect cards, settings parts
std::string Localized(const std::string& key);            // the game's own text for a key (a resource's name ...); the key itself when it has none
void SpeedPips(ImDrawList* dl, const void* owner, ImVec2 origin, float h);  // pause button and five speed pips (components_shell.cpp)
bool& StateFlag(const char* name);                        // a named switch kept by the plugin (the settings page flips "stars")

// ---- parameters of the entry being drawn
inline const StlGuiNodeApi* NA() { return g_ctx->node; }
inline const char* StrP(const StlGuiNode* n, const char* key, const char* def = "") { return NA()->value(n, key, def); }
inline float NumP(const StlGuiNode* n, const char* key, float def) { return (float)NA()->number(n, key, def); }
inline std::string LocP(const StlGuiNode* n, const char* key, const char* def = "") { return NA()->text(n, key, def); }  // a copy: the host's buffer is short-lived
inline bool FlagP(const StlGuiNode* n, const char* key, bool def) {
    const char* v = NA()->value(n, key, nullptr);
    return v ? strcmp(v, "no") != 0 && strcmp(v, "0") != 0 && strcmp(v, "false") != 0 : def;
}
// `accent = 0` (the theme's first colour) .. `1` (the second), anything between mixes them
inline ImU32 AccentP(const StlGuiNode* n) { return Mix(Acc(), Acc2(), NumP(n, "accent", 0.f)); }

// ---- the data of this frame
const StlGuiSnapshot& Snap();  // taken once per frame
const StlGuiResource* FindRes(const StlGuiSnapshot& s, const std::string& key, uint32_t* index = nullptr);

// ---- layout
// A component that hit-tests after reserving its space moves the cursor around; this puts it back where it was when the component is done.
struct CursorKeep {
    ImVec2 pos;
    CursorKeep() : pos(ImGui::GetCursorScreenPos()) {}
    ~CursorKeep() { ImGui::SetCursorScreenPos(pos); }
};
inline float LineStartX() {  // where a new line of the current region starts
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    return w->Pos.x + w->DC.Indent.x + w->DC.ColumnsOffset.x;
}
inline float AvailW() { return ImGui::GetCurrentWindow()->WorkRect.Max.x - LineStartX(); }
// A width given as a fraction (0 < v <= 1) of `whole` or as pixels (> 1, times the layout scale); `def` when absent
inline float WidthP(const StlGuiNode* n, const char* key, float whole, float def) {
    const float v = NumP(n, key, 0.f);
    return v <= 0.f ? def : (v <= 1.f ? whole * v : v * S());
}
// Puts the following content in the screen rectangle that starts at (x, y) and is w wide: lines start at x and widgets that fill the width stop at x + w.
struct Region {
    ImGuiWindow* win;
    float indent, old_max;
    Region(float x, float y, float w) {
        win = ImGui::GetCurrentWindow();
        indent = x - LineStartX();
        old_max = win->WorkRect.Max.x;
        if (fabsf(indent) > 0.01f) ImGui::Indent(indent);
        win->WorkRect.Max.x = x + w;
        ImGui::SetCursorScreenPos(ImVec2(x, y));
    }
    ~Region() {
        win->WorkRect.Max.x = old_max;
        if (fabsf(indent) > 0.01f) ImGui::Unindent(indent);
    }
};

}  // namespace argon
