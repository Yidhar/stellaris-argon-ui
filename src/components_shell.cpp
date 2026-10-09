// The shell components: the frame of a big window (background, header, close button), tabs with an icon rail, and the parts of a HUD capsule
// (pill, orb, date, speed controls, resource chips). Ported from the Command Deck of stellaris-guiexpand.
#include "argon.h"

namespace argon {

bool& StateFlag(const char* name) {  // a few switches the settings page flips: stars (the window's starfield)
    static std::unordered_map<std::string, bool> flags = { { "stars", true } };
    return flags.try_emplace(name, false).first->second;
}

namespace {

int PanelVisible(const char* id) { return g_ctx->api->panel_visibility(id, -1); }

// ------------------------------------------------------------------------------------------------------------------------------------------ window
// The frame of a window: shadow, dark glass, the two glows and the starfield, the accent line on top, a header and a close button. Use it as the first entry of
// the content of a `kind = hud` panel (the panel's window has no title bar and no background). Parameters: kicker, title, subtitle (loc keys), inset (px the
// header starts at; leave room for a rail), close (the id of the panel the button hides). The entries after it are the body; it starts 82 px down.
void WindowFrame(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize(), b = a + sz;
    const float R = 22.f * S(), W = sz.x, H = sz.y;
    for (int i = 8; i >= 1; --i) dl->AddRectFilled(a - ImVec2(i * 3.f, i * 3.f - 10), b + ImVec2(i * 3.f, i * 3.f + 10), C(0, 0, 0, 9), R + i * 3.f);
    dl->AddRectFilled(a, b, C(6, 9, 20, 247), R);
    dl->PushClipRect(a, b, true);
    Glow(dl, ImVec2(b.x - W * 0.12f, a.y + H * 0.2f), 460.f * S(), Acc2(), 0.34f);
    Glow(dl, ImVec2(a.x + W * 0.22f, b.y - H * 0.05f), 420.f * S(), Acc(), 0.26f);
    if (StateFlag("stars")) Stars(dl, a, b, Tm());
    dl->PopClipRect();
    dl->AddRect(a, b, C(255, 255, 255, 34), R, 0, 1.f);
    dl->AddRectFilledMultiColor(ImVec2(a.x + R, a.y), ImVec2(b.x - R, a.y + 2.5f * S()), Acc(), Acc2(), Acc2(), Acc());

    const float x = a.x + NumP(n, "inset", 100.f) * S();
    const std::string kicker = LocP(n, "kicker"), title = LocP(n, "title"), sub = LocP(n, "subtitle");
    Txt(dl, FontBold(), 0.62f, ImVec2(x, a.y + 18.f * S()), Al(Acc(), 0.9f), kicker.c_str());
    Txt(dl, FontTitle(), 1.f, ImVec2(x, a.y + 30.f * S()), ColText(), title.c_str());
    Txt(dl, FontBody(), 0.9f, ImVec2(x + TextSz(FontTitle(), 1.f, title.c_str()).x + 14.f * S(), a.y + 40.f * S()), ColDim(), sub.c_str());

    const char* close = StrP(n, "close", "");
    if (*close) {
        const ImVec2 cc(b.x - 36.f * S(), a.y + 40.f * S());
        const float r = 16.f * S();
        const bool hov = ImGui::IsMouseHoveringRect(cc - ImVec2(r, r), cc + ImVec2(r, r)) && ImGui::IsWindowHovered();
        if (Hit(cc - ImVec2(r, r), cc + ImVec2(r, r), "close")) g_ctx->api->panel_visibility(close, 0);
        const float hv = Smooth(n, "closeh", hov ? 1.f : 0.f);
        dl->AddCircleFilled(cc, r, C(255, 255, 255, (int)(14 + 40 * hv)), 24);
        dl->AddLine(cc - ImVec2(5 * S(), 5 * S()), cc + ImVec2(5 * S(), 5 * S()), ColText(), 1.8f * S());
        dl->AddLine(cc - ImVec2(-5 * S(), 5 * S()), cc + ImVec2(-5 * S(), 5 * S()), ColText(), 1.8f * S());
    }
    ImGui::SetCursorScreenPos(ImVec2(a.x, a.y + 82.f * S()));
}

// ------------------------------------------------------------------------------------------------------------------------------------------------ tabs
// An icon rail on the left and the page of the selected tab on the right: `tab = { icon = overview  title = KEY  subtitle = KEY  content = { ... } }`.
// icon: overview, economy, time, script or settings. Meant for the body of argon_window; the page takes the rest of the window.
void Tabs(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    std::vector<const StlGuiNode*> tabs;
    for (uint32_t i = 0; i < NA()->child_count(n); ++i) {
        const StlGuiNode* c = NA()->child_at(n, i);
        if (c && NA()->is_block(c) && !strcmp(NA()->key(c), "tab")) tabs.push_back(c);
    }
    if (tabs.empty()) return;
    static std::unordered_map<const void*, int> selected;
    int& sel = selected.try_emplace(n, 0).first->second;
    sel = std::clamp(sel, 0, (int)tabs.size() - 1);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImVec2 wb = win->Pos + win->Size;
    const float rail_x = origin.x + 18.f * S(), item_h = 82.f * S(), top = origin.y + 14.f * S();
    const float ind = Smooth(n, "rail", (float)sel, 9.f);
    dl->AddRectFilledMultiColor(ImVec2(rail_x - 18.f * S(), top + ind * item_h + 10.f * S()), ImVec2(rail_x - 14.f * S(), top + ind * item_h + item_h - 14.f * S()), Acc(), Acc(),
                                Acc2(), Acc2());
    static const char* const kIcons[5] = { "overview", "economy", "time", "script", "settings" };
    for (size_t i = 0; i < tabs.size(); ++i) {
        const ImVec2 ia(rail_x, top + i * item_h), ib(rail_x + 62.f * S(), ia.y + item_h - 6.f * S());
        char id[24];
        snprintf(id, sizeof(id), "tab%zu", i);
        const bool hov = ImGui::IsMouseHoveringRect(ia, ib) && ImGui::IsWindowHovered();
        if (Hit(ia, ib, id)) sel = (int)i;
        const float act = Smooth(n, id, sel == (int)i ? 1.f : 0.f, 10.f), hv = Smooth(n, (std::string(id) + "h").c_str(), hov ? 1.f : 0.f);
        const ImU32 accent = Mix(Acc(), Acc2(), tabs.size() > 1 ? (float)i / (float)(tabs.size() - 1) : 0.f);
        dl->AddRectFilled(ia, ib, Al(accent, 0.16f * act + 0.06f * hv), 14.f * S());
        const ImU32 col = Mix(ColDim(), ColText(), std::max(act, hv * 0.7f));
        int icon = 0;
        const char* name = StrP(tabs[i], "icon", "");
        for (int k = 0; k < 5; ++k)
            if (!strcmp(name, kIcons[k])) icon = k;
        TabIcon(dl, icon, ImVec2((ia.x + ib.x) * 0.5f, ia.y + 26.f * S()), 11.f * S(), Mix(col, accent, act));
        const std::string title = LocP(tabs[i], "title"), small = LocP(tabs[i], "subtitle");
        Txt(dl, FontBold(), 0.7f, ImVec2((ia.x + ib.x) * 0.5f, ia.y + 47.f * S()), col, title.c_str(), 1);
        Txt(dl, FontBody(), 0.5f, ImVec2((ia.x + ib.x) * 0.5f, ia.y + 60.f * S()), Al(col, 0.6f), small.c_str(), 1);
    }
    const float px = origin.x + 100.f * S(), pw = wb.x - 24.f * S() - px;
    {
        Region r(px, origin.y, pw);
        NA()->draw_block(NA()->child(tabs[sel], "content"));
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy(ImVec2(wb.x - origin.x, std::max(1.f, wb.y - 24.f * S() - origin.y)));
}

// ------------------------------------------------------------------------------------------------------------------------------------------ HUD parts
float HudH() { return ImGui::GetWindowSize().y; }

// The capsule's background: a pill the size of the window, with a soft shadow, a lit rim and the accent line under it. Put it first in a HUD's content.
void Pill(const StlGuiNode*) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetWindowPos(), b = a + ImGui::GetWindowSize();
    const float H = b.y - a.y;
    for (int i = 6; i >= 1; --i) dl->AddRectFilled(a - ImVec2(i * 2.f, i * 2.f - 6), b + ImVec2(i * 2.f, i * 2.f + 6), C(0, 0, 0, 10), H * 0.5f + i * 2.f);
    dl->AddRectFilled(a, b, C(8, 11, 24, 232), H * 0.5f);
    dl->AddRectFilledMultiColor(a + ImVec2(H * 0.5f, 1), ImVec2(b.x - H * 0.5f, a.y + H * 0.5f), C(255, 255, 255, 14), C(255, 255, 255, 14), C(255, 255, 255, 0),
                                C(255, 255, 255, 0));
    dl->AddRect(a, b, C(255, 255, 255, 30), H * 0.5f, 0, 1.f);
    dl->AddRectFilledMultiColor(ImVec2(a.x + H, b.y - 2.f * S()), ImVec2(b.x - H, b.y), Al(Acc(), 0.f), Acc(), Acc2(), Al(Acc2(), 0.f));
}

// The orb at the left of the capsule: it opens and closes the panel named by `toggle`. Parameters: toggle (a panel id).
void Orb(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float H = HudH();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec2 oc(a.x + H * 0.5f, a.y + H * 0.5f);
    const float orad = H * 0.34f;
    const char* target = StrP(n, "toggle", "");
    const bool open = *target && PanelVisible(target) == 1;
    const bool hov = ImGui::IsMouseHoveringRect(oc - ImVec2(orad, orad), oc + ImVec2(orad, orad)) && ImGui::IsWindowHovered();
    if (Hit(oc - ImVec2(orad, orad), oc + ImVec2(orad, orad), "orb") && *target) g_ctx->api->panel_visibility(target, 2);
    const bool paused = Snap().paused != 0;
    const float oh = Smooth(n, "orbh", hov ? 1.f : 0.f), spin = (float)Tm() * (paused ? 0.25f : 0.9f);
    Glow(dl, oc, orad * (1.7f + 0.3f * oh), Acc(), 0.35f + 0.3f * oh);
    dl->AddCircleFilled(oc, orad, C(10, 14, 30, 255), 32);
    for (int i = 0; i < 3; ++i) {
        dl->PathArcTo(oc, orad - i * 4.f * S(), spin * (i % 2 ? -1 : 1) + i, spin * (i % 2 ? -1 : 1) + i + IM_PI * 1.2f, 20);
        dl->PathStroke(Mix(Acc(), Acc2(), i / 2.f), 0, 2.f * S());
    }
    dl->AddCircleFilled(oc, 3.2f * S(), open ? ColText() : Acc(), 12);
    ImGui::SetCursorScreenPos(a);
    ImGui::Dummy(ImVec2(H, H));
}

// The date of the game with a small caption. Parameters: label (loc key for the caption), empty (loc key shown outside a game), width (px, default 168).
void DateBig(const StlGuiNode* n) {
    const StlGuiSnapshot& s = Snap();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float H = HudH(), w = NumP(n, "width", 168.f) * S();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, H));
    if (!s.in_game) {
        const std::string t = LocP(n, "empty", "-");
        Txt(dl, FontBody(), 1.f, ImVec2(a.x, a.y + H * 0.5f - 10.f * S()), ColDim(), t.c_str());
        return;
    }
    char date[32];
    snprintf(date, sizeof(date), "%04u.%02u.%02u", s.year, s.month, s.day);
    const std::string label = LocP(n, "label", "DATE");
    Txt(dl, FontBody(), 0.6f, ImVec2(a.x, a.y + 10.f * S()), Al(Acc(), 0.9f), label.c_str());
    Txt(dl, FontNumS(), 1.f, ImVec2(a.x, a.y + 24.f * S()), ColText(), date);
}

}  // namespace

// Pause button and five speed pips (they set the game's own speed).
void SpeedPips(ImDrawList* dl, const void* owner, ImVec2 origin, float h) {
    const StlGuiSnapshot& s = Snap();
    const float pw = 26.f * S(), gap = 5.f * S(), bw = h;
    const ImVec2 pa = origin, pb(origin.x + bw, origin.y + h);
    const bool hov = ImGui::IsMouseHoveringRect(pa, pb) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (Hit(pa, pb, "pause")) g_ctx->api->set_paused(s.paused ? 0 : 1);
    const float hv = Smooth(owner, "pause_h", hov ? 1.f : 0.f);
    dl->AddRectFilled(pa, pb, s.paused ? Al(ColWarn(), 0.28f + 0.2f * hv) : C(255, 255, 255, (int)(18 + 22 * hv)), h * 0.3f);
    const ImVec2 cc((pa.x + pb.x) * 0.5f, (pa.y + pb.y) * 0.5f);
    if (s.paused) {
        dl->AddTriangleFilled(cc + ImVec2(-5 * S(), -7 * S()), cc + ImVec2(-5 * S(), 7 * S()), cc + ImVec2(8 * S(), 0), ColWarn());
    } else {
        dl->AddRectFilled(cc + ImVec2(-6 * S(), -7 * S()), cc + ImVec2(-2 * S(), 7 * S()), ColText(), 1.5f);
        dl->AddRectFilled(cc + ImVec2(2 * S(), -7 * S()), cc + ImVec2(6 * S(), 7 * S()), ColText(), 1.5f);
    }
    for (int i = 1; i <= 5; ++i) {
        const ImVec2 a(origin.x + bw + 10.f * S() + (i - 1) * (pw + gap), origin.y + h * 0.18f), b(a.x + pw, origin.y + h * 0.82f);
        char id[16], idh[16];
        snprintf(id, sizeof(id), "s%d", i);
        snprintf(idh, sizeof(idh), "s%dh", i);
        const bool hv2 = ImGui::IsMouseHoveringRect(a, b) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        if (Hit(a, b, id)) g_ctx->api->set_speed(i);
        const bool on = (int)s.speed >= i;
        const float lit = Smooth(owner, id, on ? 1.f : 0.f, 14.f), hv3 = Smooth(owner, idh, hv2 ? 1.f : 0.f);
        const ImU32 col = Mix(C(255, 255, 255, 30 + (int)(26 * hv3)), Mix(Acc(), Acc2(), i / 5.f), lit);
        dl->AddRectFilled(a, b, Al(col, s.paused && on ? 0.45f : 1.f), (b.y - a.y) * 0.5f);
        if (on && !s.paused) Glow(dl, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f), 20.f * S(), col, 0.18f);
    }
}

namespace {

// Pause and the five speeds. Parameters: height (px; the HUD's height less 30 by default).
void Speed(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float H = HudH(), h = NumP(n, "height", 0.f) > 0.f ? NumP(n, "height", 0.f) * S() : H - 30.f * S();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float w = h + 10.f * S() + 5.f * (26.f + 5.f) * S() + 6.f * S();
    SpeedPips(dl, n, ImVec2(a.x, a.y + (H - h) * 0.5f), h);
    ImGui::SetCursorScreenPos(a);
    ImGui::Dummy(ImVec2(w, H));
}

// A thin vertical line between groups of a HUD.
void VLine(const StlGuiNode*) {
    const float H = HudH();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(a.x + 6.f * S(), a.y + 16.f * S()), ImVec2(a.x + 6.f * S(), a.y + H - 16.f * S()), C(255, 255, 255, 28));
    ImGui::Dummy(ImVec2(14.f * S(), H));
}

// One resource of the HUD: its icon, the stock and the monthly net. Parameters: resource, width (px, default 118).
void Chip(const StlGuiNode* n) {
    const StlGuiSnapshot& s = Snap();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float H = HudH(), w = NumP(n, "width", 118.f) * S();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, H));
    const std::string key = StrP(n, "resource", "energy");
    const StlGuiResource* r = s.in_game ? FindRes(s, key) : nullptr;
    if (!r) return;
    const ImU32 rc = ResColor(key);
    Icon(dl, key, ImVec2(a.x + 12.f * S(), a.y + H * 0.5f), 9.f * S(), rc);
    char v[32], net[32];
    Fmt(v, sizeof(v), r->stock);
    Fmt(net, sizeof(net), r->net, true);
    Txt(dl, FontBold(), 1.f, ImVec2(a.x + 28.f * S(), a.y + 13.f * S()), ColText(), v);
    Txt(dl, FontBody(), 0.76f, ImVec2(a.x + 28.f * S(), a.y + 36.f * S()), r->net < -0.004 ? ColBad() : (r->net > 0.004 ? ColGood() : ColDim()), net);
}

}  // namespace

void AddShellComponents(std::vector<ComponentDef>& out) {
    out.push_back({ "argon_window", WindowFrame });
    out.push_back({ "argon_tabs", Tabs });
    out.push_back({ "argon_pill", Pill });
    out.push_back({ "argon_orb", Orb });
    out.push_back({ "argon_date", DateBig });
    out.push_back({ "argon_speed", Speed });
    out.push_back({ "argon_vline", VLine });
    out.push_back({ "argon_chip", Chip });
}

}  // namespace argon
