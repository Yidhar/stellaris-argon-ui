// The components. Each draws one entry of a declaration (docs/components.md has the parameters): it reads its parameters from the node, sizes itself inside
// ImGui's layout flow (so it works inside a row, a card, a tab), and draws with the host's theme and scale.
#include "argon.h"

namespace argon {

const StlGuiSnapshot& Snap() {
    static StlGuiSnapshot s;
    static float stamp = -1.f;
    if (stamp != g_ctx->time) {  // once per frame: the snapshot only changes between turn ticks
        stamp = g_ctx->time;
        memset(&s, 0, sizeof(s));
        s.size = sizeof(s);
        g_ctx->api->get_snapshot(&s);
    }
    return s;
}

const StlGuiResource* FindRes(const StlGuiSnapshot& s, const std::string& key, uint32_t* index) {
    for (uint32_t i = 0; i < s.resource_count; ++i)
        if (key == s.resources[i].key) {
            if (index) *index = i;
            return &s.resources[i];
        }
    return nullptr;
}

namespace {

float g_fill_h = 0.f;  // the height a `height = fill` card takes inside a row (set by the row)

float FillHeightFrom(float top_y) {  // down to the bottom of the window
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    return std::max(40.f * S(), w->Pos.y + w->Size.y - 24.f * S() - top_y);
}
float HeightP(const StlGuiNode* n, float def_px, float top_y) {  // `height = 120` (px), `height = fill`, or nothing (def_px, 0 = automatic)
    const char* v = StrP(n, "height", "");
    if (!strcmp(v, "fill")) return g_fill_h > 0.f ? g_fill_h : FillHeightFrom(top_y);
    const float num = (float)atof(v);
    return num > 0.f ? num * S() : def_px;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------ card
// A glass card: kicker and title in the header, `content` below. Parameters: kicker, title (loc keys), accent 0..1, width (px or a fraction of the line),
// height (px, fill, or automatic), padding.
void Card(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float avail = AvailW();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float w = WidthP(n, "width", avail, avail);
    const float fixed = HeightP(n, 0.f, a.y);
    const float pad = NumP(n, "padding", 18.f) * S();
    const std::string kicker = LocP(n, "kicker"), title = LocP(n, "title");
    const float head = (kicker.empty() && title.empty()) ? pad * 0.7f : 52.f * S();
    static std::unordered_map<const void*, float> heights;  // the height the content needed last frame: the background is drawn before the content
    float& cached = heights[n];
    const float h = fixed > 0.f ? fixed : (cached > 0.f ? cached : 120.f * S());
    Panel(dl, a, a + ImVec2(w, h), 16.f * S(), AccentP(n));
    if (FlagP(n, "glow", false)) {  // a soft glow in the top right corner
        dl->PushClipRect(a, a + ImVec2(w, h), true);
        Glow(dl, ImVec2(a.x + w - 60.f * S(), a.y + 40.f * S()), 150.f * S(), Acc2(), 0.35f);
        dl->PopClipRect();
    }
    if (head > pad) CardTitle(dl, a, kicker.c_str(), title.c_str());
    float used;
    {
        Region r(a.x + pad, a.y + head, w - 2.f * pad);
        if (fixed > 0.f) ImGui::PushClipRect(a, a + ImVec2(w, fixed), true);  // a card of fixed height does not let its content spill out
        ImGui::BeginGroup();
        NA()->draw_block(NA()->child(n, "content"));
        ImGui::EndGroup();
        used = ImGui::GetItemRectMax().y - a.y + pad;
        if (fixed > 0.f) ImGui::PopClipRect();
    }
    cached = used;
    ImGui::SetCursorScreenPos(a);
    ImGui::Dummy(ImVec2(w, fixed > 0.f ? fixed : used));
}

// ------------------------------------------------------------------------------------------------------------------------------------------------- row
// Columns side by side: `col = { width = 0.4  content = { ... } }`, a width being a fraction of the line or pixels; columns without a width share what is left.
// Parameters: gap, height (px, fill, or the tallest column).
void Row(const StlGuiNode* n) {
    const float avail = AvailW();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float gap = NumP(n, "gap", 16.f) * S();
    std::vector<const StlGuiNode*> cols;
    for (uint32_t i = 0; i < NA()->child_count(n); ++i) {
        const StlGuiNode* c = NA()->child_at(n, i);
        if (c && NA()->is_block(c) && !strcmp(NA()->key(c), "col")) cols.push_back(c);
    }
    if (cols.empty()) return;
    const float usable = avail - gap * (float)(cols.size() - 1);
    std::vector<float> w(cols.size(), 0.f);
    float fixed_total = 0.f;
    int flex = 0;
    for (size_t i = 0; i < cols.size(); ++i) {
        const float v = NumP(cols[i], "width", 0.f);
        if (v <= 0.f) ++flex;
        else fixed_total += (w[i] = v <= 1.f ? usable * v : v * S());
    }
    for (size_t i = 0; i < cols.size(); ++i)
        if (w[i] <= 0.f) w[i] = std::max(10.f * S(), (usable - fixed_total) / (float)flex);
    const float want = HeightP(n, 0.f, a.y);
    const float saved_fill = g_fill_h;
    g_fill_h = want;  // `height = fill` cards in the columns take the row's height
    float x = a.x, tallest = 0.f;
    for (size_t i = 0; i < cols.size(); ++i) {
        Region r(x, a.y, w[i]);
        ImGui::BeginGroup();
        NA()->draw_block(NA()->child(cols[i], "content"));
        ImGui::EndGroup();
        tallest = std::max(tallest, ImGui::GetItemRectMax().y - a.y);
        x += w[i] + gap;
    }
    g_fill_h = saved_fill;
    ImGui::SetCursorScreenPos(a);
    ImGui::Dummy(ImVec2(avail, want > 0.f ? want : tallest));
}

// ------------------------------------------------------------------------------------------------------------------------------------------ stat tile
// A figure with a label and an accent bar. Parameters: label (loc key), stat (colonies pops empire_size military_power tech_power economy_power) or resource
// + show (stock net income expense max), accent 0..1, width, height.
void StatTile(const StlGuiNode* n) {
    const StlGuiSnapshot& s = Snap();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float avail = AvailW();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float w = WidthP(n, "width", avail, avail), h = HeightP(n, 96.f * S(), a.y);
    const ImVec2 b = a + ImVec2(w, h);
    char value[48] = "-";
    const std::string stat = StrP(n, "stat", ""), res = StrP(n, "resource", "");
    if (s.in_game) {
        if (stat == "colonies") snprintf(value, sizeof(value), "%u", s.colonies);
        else if (stat == "pops") snprintf(value, sizeof(value), "%u", s.pops);
        else if (stat == "empire_size") snprintf(value, sizeof(value), "%d", s.empire_size);
        else if (stat == "military_power") Fmt(value, sizeof(value), s.military_power);
        else if (stat == "tech_power") Fmt(value, sizeof(value), s.tech_power);
        else if (stat == "economy_power") Fmt(value, sizeof(value), s.economy_power);
        else if (!res.empty()) {
            uint32_t i = 0;
            if (const StlGuiResource* r = FindRes(s, res, &i)) {
                const std::string show = StrP(n, "show", "stock");
                const double v = show == "net" ? r->net : show == "income" ? s.income[i] : show == "expense" ? s.expense[i] : show == "max" ? r->max : r->stock;
                Fmt(value, sizeof(value), v, show == "net");
            }
        }
    }
    const ImU32 col = AccentP(n);
    dl->AddRectFilled(a, b, C(255, 255, 255, 10), 12.f * S());
    dl->AddRectFilled(ImVec2(a.x, a.y + 12.f * S()), ImVec2(a.x + 3.f * S(), b.y - 12.f * S()), col, 2.f);
    Txt(dl, FontNumS(), 1.15f, ImVec2(a.x + 16.f * S(), a.y + 12.f * S()), ColText(), value);
    const std::string label = LocP(n, "label");
    Txt(dl, FontBody(), 0.8f, ImVec2(a.x + 16.f * S(), b.y - 28.f * S()), ColDim(), label.c_str());
    ImGui::Dummy(ImVec2(w, h));
}

// ------------------------------------------------------------------------------------------------------------------------------------------------ ring
// A gauge of one resource: the ring (stock against its cap, or against its recent peak), the stock and net in it, the icon and the name under it.
// Parameters: resource, label (loc key; the resource's key when absent), size (diameter in px), width.
void RingGauge(const StlGuiNode* n) {
    const StlGuiSnapshot& s = Snap();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const std::string key = StrP(n, "resource", "energy");
    const float size = NumP(n, "size", 96.f) * S();
    const float w = std::max(size, NumP(n, "width", 0.f) * S());
    const ImVec2 a = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, size + 46.f * S()));
    const ImVec2 c(a.x + w * 0.5f, a.y + size * 0.5f);
    const float th = std::max(5.f * S(), size * 0.09f), rad = size * 0.5f - th * 0.5f;
    uint32_t idx = 0;
    const StlGuiResource* r = s.in_game ? FindRes(s, key, &idx) : nullptr;
    const ImU32 rc = ResColor(key);
    if (!r) {
        Ring(dl, c, rad, th, 0.f, Al(rc, 0.55f), rc);
        Txt(dl, FontBody(), 0.9f, c - ImVec2(0, 8.f * S()), ColDim(), "?", 1);
        return;
    }
    float peak = (float)std::max(1.0, r->stock);
    float hist[160];
    const int hn = g_ctx->api->get_history(key.c_str(), hist, 160);
    for (int i = 0; i < hn; ++i) peak = std::max(peak, hist[i]);
    const double frac = r->max > 0 ? r->stock / r->max : r->stock / (peak * 1.15);
    const float f = Smooth(n, "ring", (float)std::clamp(frac, 0.0, 1.0), 4.f);
    Ring(dl, c, rad, th, f, Al(rc, 0.55f), rc);
    char v[32], net[32];
    Fmt(v, sizeof(v), r->stock);
    Fmt(net, sizeof(net), r->net, true);
    Txt(dl, FontNumS(), 1.0f, c - ImVec2(0, 15.f * S()), ColText(), v, 1);
    Txt(dl, FontBody(), 0.8f, c + ImVec2(0, 10.f * S()), r->net < -0.004 ? ColBad() : (r->net > 0.004 ? ColGood() : ColDim()), net, 1);
    Icon(dl, key, c + ImVec2(0, size * 0.5f + 12.f * S()), 8.f * S(), rc);
    const std::string label = NA()->child(n, "label") ? LocP(n, "label") : Localized(key);  // the game's own name unless the declaration gives one
    Txt(dl, FontBody(), 0.82f, c + ImVec2(0, size * 0.5f + 24.f * S()), ColDim(), label.c_str(), 1);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------- radar
// Five axes of the country against the strongest empire of the galaxy on each (100 % = that empire). Parameters: size (diameter of the outer ring in px),
// label_military, label_tech, label_economy, label_territory, label_population (loc keys), note (loc key).
void Radar(const StlGuiNode* n) {
    const StlGuiSnapshot& s = Snap();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float avail = AvailW();
    const float R = NumP(n, "size", 240.f) * S() * 0.5f;
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const bool note = NA()->child(n, "note") != nullptr;
    // the pentagon's lowest points are at 0.81 of the radius; their labels hang about 41 px under them
    ImGui::Dummy(ImVec2(avail, 48.f * S() + 1.81f * R + 44.f * S() + (note ? 22.f * S() : 0.f)));
    const ImVec2 c(a.x + avail * 0.5f, a.y + 48.f * S() + R);
    struct Axis {
        std::string label;
        double v, m;
    } ax[5] = { { LocP(n, "label_military", "Military"), s.military_power, s.military_power_max },
                { LocP(n, "label_tech", "Technology"), s.tech_power, s.tech_power_max },
                { LocP(n, "label_economy", "Economy"), s.economy_power, s.economy_power_max },
                { LocP(n, "label_territory", "Territory"), (double)s.colonies, s.colonies_max },
                { LocP(n, "label_population", "Population"), (double)s.pops, s.pops_max } };
    for (int ring = 1; ring <= 4; ++ring) {
        ImVec2 pts[5];
        for (int i = 0; i < 5; ++i) {
            const float t = -IM_PI / 2 + i * 2 * IM_PI / 5;
            pts[i] = c + ImVec2(cosf(t), sinf(t)) * (R * ring / 4.f);
        }
        dl->AddPolyline(pts, 5, C(255, 255, 255, ring == 4 ? 44 : 20), ImDrawFlags_Closed, 1.f);
    }
    ImVec2 poly[5];
    for (int i = 0; i < 5; ++i) {
        const float t = -IM_PI / 2 + i * 2 * IM_PI / 5;
        const ImVec2 dir(cosf(t), sinf(t));
        dl->AddLine(c, c + dir * R, C(255, 255, 255, 22));
        char key[16];
        snprintf(key, sizeof(key), "radar%d", i);
        const float frac = Smooth(n, key, (float)std::clamp(ax[i].m > 0 ? ax[i].v / ax[i].m : 0.0, 0.0, 1.0), 4.f);
        poly[i] = c + dir * (R * (0.06f + 0.94f * frac));
        const ImVec2 lp = c + dir * (R + 26.f * S());
        Txt(dl, FontBold(), 0.9f, lp - ImVec2(0, 14.f * S()), ColText(), ax[i].label.c_str(), 1);
        TxtF(dl, FontBody(), 0.8f, lp + ImVec2(0, 3.f * S()), Al(Acc(), 0.95f), 1, "%.0f%%", frac * 100.f);
    }
    for (int i = 0; i < 5; ++i) dl->AddTriangleFilled(c, poly[i], poly[(i + 1) % 5], Al(Mix(Acc(), Acc2(), i / 4.f), 0.22f));
    for (int i = 0; i < 5; ++i) dl->AddLine(poly[i], poly[(i + 1) % 5], Mix(Acc(), Acc2(), i / 4.f), 2.2f * S());
    for (int i = 0; i < 5; ++i) {
        Glow(dl, poly[i], 11.f * S(), Mix(Acc(), Acc2(), i / 4.f), 0.5f);
        dl->AddCircleFilled(poly[i], 3.6f * S(), ColText(), 12);
    }
    if (note) {
        const std::string t = LocP(n, "note");
        Txt(dl, FontBody(), 0.74f, ImVec2(a.x + avail * 0.5f, c.y + 0.81f * R + 52.f * S()), ColDim(), t.c_str(), 1);
    }
}

// ---------------------------------------------------------------------------------------------------------------------------------------------- chart
// The history of a series as a gradient area with a crosshair. Parameters: series (a resource key, "<resource>.net", @frame_ms, @tick_rate), kind (area, the
// default, or line), color (a resource key, accent or accent2), caption (loc key), height.
void Chart(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const std::string series = StrP(n, "series", "energy");
    const float w = AvailW(), h = NumP(n, "height", 170.f) * S();
    const ImVec2 a = ImGui::GetCursorScreenPos(), b = a + ImVec2(w, h);
    ImGui::Dummy(ImVec2(w, h));
    const std::string color = StrP(n, "color", "");
    std::string base = series;
    if (base.size() > 4 && base.compare(base.size() - 4, 4, ".net") == 0) base.resize(base.size() - 4);
    const ImU32 rc = color == "accent" ? Acc() : color == "accent2" ? Acc2() : (!color.empty() ? ResColor(color) : (base[0] == '@' ? Acc() : ResColor(base)));
    dl->AddRectFilled(a, b, C(255, 255, 255, 8), 12.f * S());
    const std::string caption = LocP(n, "caption");
    if (!caption.empty()) Txt(dl, FontBold(), 0.62f, a + ImVec2(12.f * S(), 8.f * S()), Al(rc, 0.85f), caption.c_str());
    const ImVec2 ca = a + ImVec2(12.f * S(), 28.f * S()), cb = b - ImVec2(12.f * S(), 12.f * S());
    float v[160];
    const int cnt = g_ctx->api->get_history(series.c_str(), v, 160);
    if (cnt < 2) {
        Txt(dl, FontBody(), 0.9f, (ca + cb) * 0.5f, ColDim(), "...", 1);
        return;
    }
    float lo = FLT_MAX, hi = -FLT_MAX;
    for (int i = 0; i < cnt; ++i) {
        lo = std::min(lo, v[i]);
        hi = std::max(hi, v[i]);
    }
    if (hi - lo < 1e-3f) {
        hi += 1.f;
        lo -= 1.f;
    }
    const float pad = (hi - lo) * 0.12f;
    lo -= pad;
    hi += pad;
    for (int g = 0; g <= 3; ++g) {
        const float y = ca.y + (cb.y - ca.y) * g / 3.f;
        dl->AddLine(ImVec2(ca.x, y), ImVec2(cb.x, y), C(255, 255, 255, 14));
        char t[32];
        Fmt(t, sizeof(t), hi - (hi - lo) * g / 3.f);
        Txt(dl, FontBody(), 0.7f, ImVec2(ca.x + 4.f * S(), y - 14.f * S()), ColDim(), t);
    }
    std::vector<ImVec2> pts;
    for (int i = 0; i < cnt; ++i) pts.push_back(ImVec2(ca.x + (cb.x - ca.x) * i / (float)(cnt - 1), cb.y - (v[i] - lo) / (hi - lo) * (cb.y - ca.y)));
    if (strcmp(StrP(n, "kind", "area"), "line")) AreaFill(dl, pts.data(), cnt, cb.y, Al(rc, 0.38f), Al(rc, 0.0f));
    dl->AddPolyline(pts.data(), cnt, rc, 0, 2.4f * S());
    Glow(dl, pts.back(), 12.f * S(), rc, 0.6f);
    dl->AddCircleFilled(pts.back(), 3.6f * S(), ColText(), 12);
    if (ImGui::IsMouseHoveringRect(ca, cb) && ImGui::IsWindowHovered()) {
        const float mx = ImGui::GetIO().MousePos.x;
        const int k = (int)std::clamp((mx - ca.x) / (cb.x - ca.x) * (cnt - 1) + 0.5f, 0.f, (float)(cnt - 1));
        dl->AddLine(ImVec2(pts[k].x, ca.y), ImVec2(pts[k].x, cb.y), C(255, 255, 255, 50));
        dl->AddCircleFilled(pts[k], 5.f * S(), rc, 14);
        char t[32];
        Fmt(t, sizeof(t), v[k]);
        TxtF(dl, FontBold(), 0.85f, ImVec2(pts[k].x, ca.y - 2.f * S()), ColText(), 1, "-%d: %s", cnt - 1 - k, t);
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------ icon
// The icon of a resource. Parameters: resource, size (px).
void IconEl(const StlGuiNode* n) {
    const float size = NumP(n, "size", 20.f) * S();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(size, size));
    const std::string key = StrP(n, "resource", "energy");
    Icon(ImGui::GetWindowDrawList(), key, a + ImVec2(size, size) * 0.5f, size * 0.45f, ResColor(key));
}

// ------------------------------------------------------------------------------------------------------------------------------------------------ gap
// Vertical space that scales with the layout (the host's own `spacer` is in raw pixels). Parameters: height (px).
void Gap(const StlGuiNode* n) { ImGui::Dummy(ImVec2(1.f, NumP(n, "height", 16.f) * S() - ImGui::GetStyle().ItemSpacing.y)); }

}  // namespace

void AddCoreComponents(std::vector<ComponentDef>& out) {
    out.push_back({ "argon_gap", Gap });
    out.push_back({ "argon_card", Card });
    out.push_back({ "argon_row", Row });
    out.push_back({ "argon_stat_tile", StatTile });
    out.push_back({ "argon_ring", RingGauge });
    out.push_back({ "argon_radar", Radar });
    out.push_back({ "argon_chart", Chart });
    out.push_back({ "argon_icon", IconEl });
}

}  // namespace argon
