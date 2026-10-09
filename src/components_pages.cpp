// The components of the Deck's pages: the resource ledger with its detail view, the speed dial and the calendar, sparklines of the host's own series, effect cards
// and their log, the theme picker, switches and information lines. Ported from the Command Deck of stellaris-guiexpand.
#include "argon.h"

namespace argon {

std::string Localized(const std::string& key) {  // the game's own name for a resource (or any key); the key itself when the game has none
    static std::unordered_map<std::string, std::string> cache;
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    auto lookup = [](const std::string& k) {
        char buf[160] = {};
        g_ctx->api->localize(k.c_str(), buf, sizeof(buf));
        return std::string(buf);
    };
    // `energy` itself is only a pointer to `$concept_energy$`, and a key the game does not know comes back as it is: the concept's text is the name
    std::string v = lookup(key);
    if (v.empty() || v == key || v[0] == '$') {
        const std::string c = lookup("concept_" + key);
        if (!c.empty() && c != "concept_" + key && c[0] != '$') v = c;
    }
    if (v.empty() || v[0] == '$') v = key;
    return cache.emplace(key, v).first->second;
}

namespace {

float PageBottom(float top_y) {  // the height from top_y down to the bottom of the window
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    return std::max(40.f * S(), w->Pos.y + w->Size.y - 24.f * S() - top_y);
}

// ------------------------------------------------------------------------------------------------------------------------------------------- heading
// A line of large text: the name of the empire, a title. `text` is a loc key and may contain [Root.GetName]. Parameters: text, scale (default 1).
void Heading(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const std::string text = LocP(n, "text");
    const bool ascii = std::all_of(text.begin(), text.end(), [](char ch) { return (uint8_t)ch < 0x80; });  // the title font has no CJK glyphs
    const float k = NumP(n, "scale", 1.f) * (ascii ? 1.f : 1.55f);
    ImFont* f = ascii ? FontTitle() : FontBody();
    const ImVec2 a = ImGui::GetCursorScreenPos(), sz = TextSz(f, k, text.empty() ? "-" : text.c_str());
    ImGui::Dummy(ImVec2(std::max(sz.x, 1.f), sz.y + 6.f * S()));
    Txt(dl, f, k, a, ColText(), text.empty() ? "-" : text.c_str());
}

// ----------------------------------------------------------------------------------------------------------------------------------------- ledger
std::unordered_map<std::string, std::string>& Selected() {  // ledger id -> the resource shown in its detail view
    static std::unordered_map<std::string, std::string> sel;
    return sel;
}
std::vector<uint32_t> LedgerRows(const StlGuiSnapshot& s) {
    std::vector<uint32_t> rows;
    for (uint32_t i = 0; i < s.resource_count; ++i)
        if (fabs(s.resources[i].stock) > 0.001 || fabs(s.income[i]) > 0.001 || fabs(s.expense[i]) > 0.001) rows.push_back(i);
    return rows;
}

// The resources the country has, one row each: icon, name, stock, monthly net and a small history; clicking a row selects it for the argon_resource_detail
// with the same `id`. Parameters: id.
void Ledger(const StlGuiNode* n) {
    const StlGuiSnapshot& s = Snap();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const std::string id = StrP(n, "id", "ledger");
    const float w = AvailW();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const std::vector<uint32_t> rows = LedgerRows(s);
    const float room = PageBottom(a.y);
    ImGui::Dummy(ImVec2(w, room));
    CursorKeep keep;
    if (rows.empty()) return;
    std::string& sel = Selected()[id];
    bool known = false;
    for (uint32_t i : rows) known |= sel == s.resources[i].key;
    if (!known) sel = s.resources[rows[0]].key;
    const float rh = std::min(46.f * S(), room / (float)rows.size());
    for (size_t r = 0; r < rows.size(); ++r) {
        const uint32_t i = rows[r];
        const StlGuiResource& res = s.resources[i];
        const ImVec2 ra(a.x, a.y + r * rh), rb(a.x + w, ra.y + rh - 4.f * S());
        char rid[24];
        snprintf(rid, sizeof(rid), "row%zu", r);
        const bool hov = ImGui::IsMouseHoveringRect(ra, rb) && ImGui::IsWindowHovered();
        if (Hit(ra, rb, rid)) sel = res.key;
        const bool on = sel == res.key;
        const float selv = Smooth(n, rid, on ? 1.f : 0.f), hv = Smooth(n, (std::string(rid) + "h").c_str(), hov ? 1.f : 0.f);
        const ImU32 rc = ResColor(res.key);
        dl->AddRectFilled(ra, rb, Al(rc, 0.10f * selv + 0.05f * hv), 10.f * S());
        if (selv > 0.02f) dl->AddRectFilled(ra + ImVec2(0, 8.f * S()), ImVec2(ra.x + 3.f * S(), rb.y - 8.f * S()), Al(rc, selv), 2.f);
        Icon(dl, res.key, ImVec2(ra.x + 24.f * S(), (ra.y + rb.y) * 0.5f), 9.f * S(), rc);
        const std::string name = Localized(res.key);
        Txt(dl, FontBody(), 0.95f, ImVec2(ra.x + 44.f * S(), (ra.y + rb.y) * 0.5f - 10.f * S()), ColText(), name.c_str());
        char v[32], net[32];
        Fmt(v, sizeof(v), res.stock);
        Fmt(net, sizeof(net), res.net, true);
        const ImU32 net_col = res.net < -0.004 ? ColBad() : (res.net > 0.004 ? ColGood() : ColDim());
        if (rh >= 40.f * S()) {  // roomy: the stock over the net
            Txt(dl, FontBold(), 0.95f, ImVec2(rb.x - 78.f * S(), ra.y + 4.f * S()), ColText(), v, 2);
            Txt(dl, FontBody(), 0.76f, ImVec2(rb.x - 78.f * S(), ra.y + rh * 0.5f), net_col, net, 2);
        } else {  // crowded: one line, the net left of the stock
            const float ty = (ra.y + rb.y) * 0.5f - FontBold()->FontSize * 0.95f * Fit() * 0.5f;
            Txt(dl, FontBold(), 0.95f, ImVec2(rb.x - 78.f * S(), ty), ColText(), v, 2);
            Txt(dl, FontBody(), 0.76f, ImVec2(rb.x - 150.f * S(), ty + 2.f * S()), net_col, net, 2);
        }
        float hist[160];
        const int hn = g_ctx->api->get_history(res.key, hist, 160);
        if (hn > 2) {
            float lo = FLT_MAX, hi = -FLT_MAX;
            for (int k = 0; k < hn; ++k) {
                lo = std::min(lo, hist[k]);
                hi = std::max(hi, hist[k]);
            }
            if (hi - lo < 1e-3f) hi = lo + 1.f;
            std::vector<ImVec2> pts;
            const float sx = rb.x - 66.f * S(), sw = 56.f * S();
            for (int k = 0; k < hn; ++k) pts.push_back(ImVec2(sx + sw * k / (float)(hn - 1), rb.y - 10.f * S() - (hist[k] - lo) / (hi - lo) * (rb.y - ra.y - 20.f * S())));
            dl->AddPolyline(pts.data(), (int)pts.size(), Al(rc, 0.9f), 0, 1.5f * S());
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------------------- resource detail
// The selected resource of a ledger as a card: icon, name, stock, income against expense, the history chart with a crosshair and the monthly net as bars.
// Parameters: id (of the ledger), kicker_chart, kicker_net, label_income, label_expense, label_net, label_cap (loc keys), height (px or fill).
void ResourceDetail(const StlGuiNode* n) {
    const StlGuiSnapshot& s = Snap();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const std::string id = StrP(n, "id", "ledger");
    const float w = AvailW();
    const ImVec2 da = ImGui::GetCursorScreenPos();
    const char* hs = StrP(n, "height", "fill");
    const float h = !strcmp(hs, "fill") ? PageBottom(da.y) : (float)atof(hs) * S();
    const ImVec2 db = da + ImVec2(w, h);
    ImGui::Dummy(ImVec2(w, h));
    uint32_t idx = 0;
    const StlGuiResource* r = FindRes(s, Selected()[id], &idx);
    if (!r) {
        Panel(dl, da, db, 16.f * S(), Acc());
        return;
    }
    const ImU32 rc = ResColor(r->key);
    Panel(dl, da, db, 16.f * S(), rc);
    dl->PushClipRect(da, db, true);
    Glow(dl, ImVec2(db.x - 90.f * S(), da.y + 70.f * S()), 190.f * S(), rc, 0.28f);
    dl->PopClipRect();
    Icon(dl, r->key, ImVec2(da.x + 40.f * S(), da.y + 46.f * S()), 17.f * S(), rc);
    Txt(dl, FontBold(), 0.62f, ImVec2(da.x + 72.f * S(), da.y + 18.f * S()), Al(rc, 0.9f), r->key);
    const std::string name = Localized(r->key);
    Txt(dl, FontBody(), 1.5f, ImVec2(da.x + 72.f * S(), da.y + 32.f * S()), ColText(), name.c_str());
    char v[32];
    Fmt(v, sizeof(v), r->stock);
    Txt(dl, FontNum(), 1.f, ImVec2(db.x - 28.f * S(), da.y + 22.f * S()), ColText(), v, 2);
    if (r->max > 0) {
        const std::string cap = LocP(n, "label_cap", "Cap");
        TxtF(dl, FontBody(), 0.8f, ImVec2(db.x - 28.f * S(), da.y + 78.f * S()), ColDim(), 2, "%s %.0f", cap.c_str(), r->max);
    }
    // the flow strip: income against expense
    const float fy = da.y + 110.f * S(), fw = w - 56.f * S(), fx = da.x + 28.f * S();
    const double tot = std::max(1e-6, s.income[idx] + s.expense[idx]);
    dl->AddRectFilled(ImVec2(fx, fy), ImVec2(fx + fw, fy + 12.f * S()), C(255, 255, 255, 14), 6.f * S());
    dl->AddRectFilled(ImVec2(fx, fy), ImVec2(fx + fw * (float)(s.income[idx] / tot), fy + 12.f * S()), Al(ColGood(), 0.85f), 6.f * S());
    dl->AddRectFilled(ImVec2(fx + fw * (float)(s.income[idx] / tot), fy), ImVec2(fx + fw, fy + 12.f * S()), Al(ColBad(), 0.85f), 6.f * S());
    char i1[32], e1[32], n1[32];
    Fmt(i1, 32, s.income[idx], true);
    Fmt(e1, 32, -s.expense[idx]);
    Fmt(n1, 32, r->net, true);
    const std::string li = LocP(n, "label_income", "Income"), le = LocP(n, "label_expense", "Expense"), ln = LocP(n, "label_net", "Net / month");
    TxtF(dl, FontBody(), 0.9f, ImVec2(fx, fy + 20.f * S()), ColGood(), 0, "%s %s", li.c_str(), i1);
    TxtF(dl, FontBody(), 0.9f, ImVec2(fx + fw * 0.5f, fy + 20.f * S()), ColBad(), 1, "%s %s", le.c_str(), e1);
    TxtF(dl, FontBold(), 0.95f, ImVec2(fx + fw, fy + 20.f * S()), r->net < 0 ? ColBad() : ColGood(), 2, "%s %s", ln.c_str(), n1);
    // the history chart
    float hist[160], nets[160];
    const int hn = g_ctx->api->get_history(r->key, hist, 160);
    const std::string netseries = std::string(r->key) + ".net";
    const int nn = g_ctx->api->get_history(netseries.c_str(), nets, 160);
    const ImVec2 ca(fx, fy + 62.f * S()), cb(fx + fw, db.y - 110.f * S());
    if (cb.y - ca.y < 40.f * S()) return;
    dl->AddRectFilled(ca - ImVec2(8.f * S(), 8.f * S()), cb + ImVec2(8.f * S(), 8.f * S()), C(255, 255, 255, 8), 12.f * S());
    const std::string kc = LocP(n, "kicker_chart", "STOCKPILE");
    Txt(dl, FontBold(), 0.62f, ca - ImVec2(0, 22.f * S()), Al(rc, 0.85f), kc.c_str());
    if (hn >= 2) {
        float lo = FLT_MAX, hi = -FLT_MAX;
        for (int i = 0; i < hn; ++i) {
            lo = std::min(lo, hist[i]);
            hi = std::max(hi, hist[i]);
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
            Fmt(t, 32, hi - (hi - lo) * g / 3.f);
            Txt(dl, FontBody(), 0.7f, ImVec2(ca.x + 4.f * S(), y - 14.f * S()), ColDim(), t);
        }
        std::vector<ImVec2> pts;
        for (int k = 0; k < hn; ++k) pts.push_back(ImVec2(ca.x + (cb.x - ca.x) * k / (float)(hn - 1), cb.y - (hist[k] - lo) / (hi - lo) * (cb.y - ca.y)));
        AreaFill(dl, pts.data(), (int)pts.size(), cb.y, Al(rc, 0.38f), Al(rc, 0.0f));
        dl->AddPolyline(pts.data(), (int)pts.size(), rc, 0, 2.4f * S());
        Glow(dl, pts.back(), 12.f * S(), rc, 0.6f);
        dl->AddCircleFilled(pts.back(), 3.6f * S(), ColText(), 12);
        if (ImGui::IsMouseHoveringRect(ca, cb) && ImGui::IsWindowHovered()) {
            const float mx = ImGui::GetIO().MousePos.x;
            const int k = (int)std::clamp((mx - ca.x) / (cb.x - ca.x) * (hn - 1) + 0.5f, 0.f, (float)(hn - 1));
            dl->AddLine(ImVec2(pts[k].x, ca.y), ImVec2(pts[k].x, cb.y), C(255, 255, 255, 50));
            dl->AddCircleFilled(pts[k], 5.f * S(), rc, 14);
            char t[32];
            Fmt(t, 32, hist[k]);
            TxtF(dl, FontBold(), 0.85f, ImVec2(pts[k].x, ca.y - 2.f * S()), ColText(), 1, "-%d: %s", hn - 1 - k, t);
        }
        // the monthly net as bars under the chart
        const float by = db.y - 78.f * S(), bh = 46.f * S();
        float amax = 1e-3f;
        for (int k = 0; k < nn; ++k) amax = std::max(amax, fabsf(nets[k]));
        dl->AddLine(ImVec2(ca.x, by + bh * 0.5f), ImVec2(cb.x, by + bh * 0.5f), C(255, 255, 255, 24));
        const float bw = (cb.x - ca.x) / (float)std::max(hn, 1);
        for (int k = 0; k < nn; ++k) {
            const float vv = nets[k] / amax * bh * 0.5f, x0 = ca.x + bw * k;
            dl->AddRectFilled(ImVec2(x0 + 0.5f, by + bh * 0.5f - std::max(vv, 0.f)), ImVec2(x0 + std::max(1.f, bw - 1.f), by + bh * 0.5f - std::min(vv, 0.f)),
                              nets[k] >= 0 ? Al(ColGood(), 0.8f) : Al(ColBad(), 0.8f), 1.5f);
        }
        const std::string kn = LocP(n, "kicker_net", "NET / MONTH");
        Txt(dl, FontBold(), 0.62f, ImVec2(ca.x, by - 15.f * S()), Al(rc, 0.85f), kn.c_str());
    } else {
        Txt(dl, FontBody(), 0.95f, (ca + cb) * 0.5f, ColDim(), "...", 1);
    }
}

// --------------------------------------------------------------------------------------------------------------------------------- speed and time
// A dial of the game speed with the state under it and the pause / speed buttons. Parameters: size (px), label_paused, label_running (loc keys).
void SpeedDial(const StlGuiNode* n) {
    const StlGuiSnapshot& s = Snap();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float avail = AvailW(), size = NumP(n, "size", 250.f) * S();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float R = size * 0.5f, total = size + 150.f * S();
    ImGui::Dummy(ImVec2(avail, total));
    CursorKeep keep;
    const ImVec2 c(a.x + avail * 0.5f, a.y + 36.f * S() + R);
    const float level = s.paused ? 0.f : (float)s.speed;
    const float sm = Smooth(n, "dial", level, 7.f);
    Ring(dl, c, R, 16.f * S(), sm / 5.f, Acc(), Acc2());
    for (int i = 0; i <= 5; ++i) {
        const float ang = IM_PI * 0.75f + IM_PI * 1.5f * i / 5.f;
        const ImVec2 d(cosf(ang), sinf(ang));
        dl->AddLine(c + d * (R + 18.f * S()), c + d * (R + 28.f * S()), i == (int)level ? ColText() : C(255, 255, 255, 60), 2.f);
        char t[4];
        snprintf(t, sizeof(t), "%d", i);
        Txt(dl, FontBody(), 0.8f, c + d * (R + 44.f * S()) - ImVec2(0, 8.f * S()), i == (int)level ? ColText() : ColDim(), i == 0 ? "0" : t, 1);
    }
    if (s.paused) Txt(dl, FontNum(), 1.5f, c - ImVec2(0, 38.f * S()), ColWarn(), "II", 1);
    else TxtF(dl, FontNum(), 1.5f, c - ImVec2(0, 38.f * S()), ColText(), 1, "x%u", s.speed);
    const std::string st = LocP(n, s.paused ? "label_paused" : "label_running", s.paused ? "Paused" : "Running");
    Txt(dl, FontBody(), 0.9f, c + ImVec2(0, 34.f * S()), s.paused ? ColWarn() : Al(Acc(), 0.95f), st.c_str(), 1);
    const float ph = 44.f * S(), pw = ph + 10.f * S() + 5.f * 31.f * S();
    SpeedPips(dl, n, ImVec2(c.x - pw * 0.5f, a.y + total - 58.f * S()), ph);
}

// The date as two rings, the months of the year (outer) and the days of the month (inner), with the date in the middle. Parameters: label_month, label_day (loc keys).
void Calendar(const StlGuiNode* n) {
    const StlGuiSnapshot& s = Snap();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float avail = AvailW();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float total = 270.f * S();
    ImGui::Dummy(ImVec2(avail, total));
    const ImVec2 rc(a.x + avail * 0.5f, a.y + 130.f * S());
    const float day_f = Smooth(n, "cal_day", (s.day - 1 + 0.5f) / 30.f, 8.f), mon_f = Smooth(n, "cal_mon", (s.month - 1 + day_f) / 12.f, 8.f);
    Ring(dl, rc, 104.f * S(), 11.f * S(), mon_f, Acc2(), Acc());
    Ring(dl, rc, 80.f * S(), 11.f * S(), day_f, Acc(), Acc2());
    char date[32];
    snprintf(date, sizeof(date), "%04u.%02u.%02u", s.year, s.month, s.day);
    Txt(dl, FontNum(), 0.5f, rc - ImVec2(0, 24.f * S()), ColText(), date, 1);
    const std::string lm = LocP(n, "label_month", "Month"), ld = LocP(n, "label_day", "Day");
    TxtF(dl, FontBody(), 0.74f, rc + ImVec2(0, 14.f * S()), ColDim(), 1, "%s %u/12", lm.c_str(), s.month);
    TxtF(dl, FontBody(), 0.74f, rc + ImVec2(0, 32.f * S()), ColDim(), 1, "%s %u/30", ld.c_str(), s.day);
}

// A small area chart of one series of the host with its latest value. Parameters: series (@frame_ms, @tick_rate or a resource), caption (loc key), unit, max (the
// top of the scale), color (accent or accent2), height (px).
void Spark(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float w = AvailW(), h = NumP(n, "height", 52.f) * S();
    const ImVec2 a0 = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, h + 22.f * S()));
    const ImVec2 sa(a0.x, a0.y + 18.f * S()), sb(a0.x + w, sa.y + h);
    const ImU32 col = !strcmp(StrP(n, "color", "accent"), "accent2") ? Acc2() : Acc();
    const std::string cap = LocP(n, "caption");
    Txt(dl, FontBold(), 0.62f, a0, Al(col, 0.9f), cap.c_str());
    dl->AddRectFilled(sa, sb, C(255, 255, 255, 8), 8.f * S());
    float v[160];
    const int cnt = g_ctx->api->get_history(StrP(n, "series", "@frame_ms"), v, 160);
    if (cnt <= 2) return;
    const float vmax = std::max(1e-3f, NumP(n, "max", 40.f));
    std::vector<ImVec2> pts;
    for (int k = 0; k < cnt; ++k) pts.push_back(ImVec2(sa.x + (sb.x - sa.x) * k / (float)(cnt - 1), sb.y - 4.f * S() - std::min(v[k] / vmax, 1.f) * (sb.y - sa.y - 8.f * S())));
    AreaFill(dl, pts.data(), cnt, sb.y, Al(col, 0.3f), Al(col, 0.f));
    dl->AddPolyline(pts.data(), cnt, col, 0, 1.8f * S());
    TxtF(dl, FontBody(), 0.8f, ImVec2(sb.x - 8.f * S(), sa.y + 4.f * S()), ColText(), 2, "%.1f %s", v[cnt - 1], StrP(n, "unit", ""));
}

// --------------------------------------------------------------------------------------------------------------------------------------- effects
struct EffectLog {
    std::string title, result;
    double time;
    bool ok;
    int64_t tick;
};
std::vector<EffectLog>& History() {
    static std::vector<EffectLog> log;
    return log;
}
struct EffectState {
    int valid = -1;  // -1 unknown, 0 refused, 1 may run
    std::string reason;
    double t = -10;
};

// A card for one button effect of a mod: its key, title and description, a chip with what the engine says (may run / refused, with its reason) and a run button.
// Parameters: effect, title, desc (loc keys), chip_ready, chip_refused (loc keys), run (loc key of the button), height (px).
void EffectCard(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const std::string key = StrP(n, "effect", "");
    const float w = AvailW(), h = NumP(n, "height", 150.f) * S();
    const ImVec2 ca = ImGui::GetCursorScreenPos(), cb = ca + ImVec2(w, h);
    ImGui::Dummy(ImVec2(w, h));
    CursorKeep keep;
    static std::unordered_map<std::string, EffectState> states;
    EffectState& st = states[key];
    if (Tm() - st.t > 0.5) {  // the engine's verdict, twice a second
        char why[512] = {};
        const int v = g_ctx->api->effect_state(key.c_str(), why, sizeof(why));
        if (v >= 0) {
            st.valid = v;
            st.reason = why;
            st.t = Tm();
        }
    }
    const bool valid = st.valid == 1;
    const ImU32 col = st.valid < 0 ? ColDim() : (valid ? ColGood() : ColBad());
    Panel(dl, ca, cb, 16.f * S(), col);
    Txt(dl, FontBold(), 0.62f, ImVec2(ca.x + 20.f * S(), ca.y + 16.f * S()), Al(Acc(), 0.85f), key.c_str());
    const std::string title = LocP(n, "title"), desc = LocP(n, "desc");
    Txt(dl, FontBold(), 1.05f, ImVec2(ca.x + 20.f * S(), ca.y + 30.f * S()), ColText(), title.c_str());
    dl->AddText(FontBody(), FontBody()->FontSize * 0.86f * Fit(), ImVec2(ca.x + 20.f * S(), ca.y + 58.f * S()), ColDim(), desc.c_str(), nullptr, w - 150.f * S());
    const std::string chip = LocP(n, valid ? "chip_ready" : "chip_refused", valid ? "Ready" : "Unavailable");
    const ImVec2 sz = TextSz(FontBold(), 0.8f, chip.c_str());
    const ImVec2 pa(ca.x + 20.f * S(), cb.y - 36.f * S()), pb(pa.x + sz.x + 22.f * S(), pa.y + 22.f * S());
    dl->AddRectFilled(pa, pb, Al(col, 0.18f), 11.f * S());
    dl->AddCircleFilled(ImVec2(pa.x + 11.f * S(), (pa.y + pb.y) * 0.5f), 3.f * S(), col, 10);
    Txt(dl, FontBold(), 0.8f, ImVec2(pa.x + 19.f * S(), pa.y + 3.f * S()), col, chip.c_str());
    if (!valid && !st.reason.empty()) {
        std::string r = st.reason;
        for (char& ch : r)
            if (ch == '\n') ch = ' ';
        dl->AddText(FontBody(), FontBody()->FontSize * 0.74f * Fit(), ImVec2(pb.x + 10.f * S(), pa.y + 3.f * S()), ColDim(), r.c_str(), nullptr, cb.x - pb.x - 160.f * S());
    }
    const ImVec2 ba(cb.x - 112.f * S(), cb.y - 44.f * S()), bb(cb.x - 18.f * S(), cb.y - 14.f * S());
    const bool hov = valid && ImGui::IsMouseHoveringRect(ba, bb) && ImGui::IsWindowHovered();
    if (Hit(ba, bb, "run") && valid && g_ctx->api->post_effect(key.c_str())) History().insert(History().begin(), { title.empty() ? key : title, "queued", Tm(), true, Snap().tick });
    const float hv = Smooth(n, "runh", hov ? 1.f : 0.f);
    const std::string run = LocP(n, "run", "Run");
    const ImU32 ink = valid ? C(8, 12, 28, 255) : C(255, 255, 255, 70);
    if (valid) {
        dl->AddRectFilledMultiColor(ba, bb, Acc(), Acc2(), Acc2(), Acc());
        if (hv > 0.01f) Glow(dl, (ba + bb) * 0.5f, 54.f * S(), Acc(), 0.3f * hv);
    } else {
        dl->AddRectFilled(ba, bb, C(255, 255, 255, 14), 8.f * S());
    }
    Txt(dl, FontBold(), 0.95f, (ba + bb) * 0.5f - ImVec2(10.f * S(), 9.f * S()), ink, run.c_str(), 1);
    const ImVec2 ar((ba.x + bb.x) * 0.5f + 22.f * S(), (ba.y + bb.y) * 0.5f);
    dl->AddTriangleFilled(ar + ImVec2(-4 * S(), -6 * S()), ar + ImVec2(-4 * S(), 6 * S()), ar + ImVec2(5 * S(), 0), ink);
}

// What the effect cards of this plugin sent, newest first, with how long ago. Parameters: empty (loc key shown when nothing was sent), limit (entries, default 6).
void EffectLogEl(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float w = AvailW();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const int limit = (int)NumP(n, "limit", 6.f);
    ImGui::Dummy(ImVec2(w, std::max(1, std::min((int)History().size(), limit)) * 28.f * S()));
    if (History().empty()) {
        const std::string t = LocP(n, "empty", "-");
        Txt(dl, FontBody(), 0.9f, a, ColDim(), t.c_str());
        return;
    }
    float y = a.y;
    for (int i = 0; i < limit && i < (int)History().size(); ++i) {
        const EffectLog& e = History()[i];
        const bool done = Snap().tick != e.tick || Tm() - e.time > 2.0;  // a turn tick has passed since: the engine has run it
        const ImU32 col = !e.ok ? ColBad() : (done ? ColGood() : ColWarn());
        dl->AddCircleFilled(ImVec2(a.x + 8.f * S(), y + 10.f * S()), 4.f * S(), col, 10);
        Txt(dl, FontBold(), 0.9f, ImVec2(a.x + 24.f * S(), y), ColText(), e.title.c_str());
        const std::string res = done ? LocP(n, "done", "Done") : LocP(n, "queued", "Queued");
        dl->AddText(FontBody(), FontBody()->FontSize * 0.86f * Fit(), ImVec2(a.x + 180.f * S(), y + 1), col, res.c_str());
        TxtF(dl, FontBody(), 0.76f, ImVec2(a.x + w - 4.f * S(), y + 2), ColDim(), 2, "%.0fs", Tm() - e.time);
        y += 28.f * S();
    }
}

// ------------------------------------------------------------------------------------------------------------------------------ settings parts
// A row per theme of the host; clicking one makes it the player's theme for every component.
void ThemePicker(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float w = AvailW();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const int count = (int)g_ctx->theme->theme_count;
    ImGui::Dummy(ImVec2(w, count * 58.f * S()));
    CursorKeep keep;
    for (int i = 0; i < count; ++i) {
        StlGuiTheme t;
        memset(&t, 0, sizeof(t));
        t.size = sizeof(t);
        if (!g_ctx->api->theme_info(i, &t)) continue;
        const ImVec2 sa(a.x, a.y + i * 58.f * S()), sb(a.x + w, sa.y + 48.f * S());
        char id[16], idh[16];
        snprintf(id, sizeof(id), "theme%d", i);
        snprintf(idh, sizeof(idh), "theme%dh", i);
        const bool hov = ImGui::IsMouseHoveringRect(sa, sb) && ImGui::IsWindowHovered();
        if (Hit(sa, sb, id)) g_ctx->api->set_theme(i);
        const float sel = Smooth(n, id, g_ctx->theme->index == (uint32_t)i ? 1.f : 0.f), hv = Smooth(n, idh, hov ? 1.f : 0.f);
        dl->AddRectFilled(sa, sb, C(255, 255, 255, (int)(10 + 14 * hv + 12 * sel)), 12.f * S());
        dl->AddRectFilledMultiColor(sa + ImVec2(14.f * S(), 12.f * S()), ImVec2(sa.x + 118.f * S(), sb.y - 12.f * S()), t.accent, t.accent2, t.accent2, t.accent);
        Txt(dl, FontBold(), 0.95f, ImVec2(sa.x + 134.f * S(), sa.y + 14.f * S()), ColText(), t.name);
        if (sel > 0.02f) dl->AddRect(sa, sb, Al(t.accent, sel), 12.f * S(), 0, 1.6f * S());
    }
}

// A switch with a label. `state` names a switch of this plugin (stars: the starfield of argon_window); `panel` is the id of a panel it shows and hides. Parameters:
// label (loc key), state or panel.
void Switch(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float w = AvailW();
    const ImVec2 ta = ImGui::GetCursorScreenPos(), tb = ta + ImVec2(w, 44.f * S());
    ImGui::Dummy(ImVec2(w, 44.f * S()));
    CursorKeep keep;
    const char* panel = StrP(n, "panel", "");
    bool on;
    if (*panel) on = g_ctx->api->panel_visibility(panel, -1) == 1;
    else on = StateFlag(StrP(n, "state", "stars"));
    if (Hit(ta, tb, "switch")) {
        if (*panel) g_ctx->api->panel_visibility(panel, 2);
        else StateFlag(StrP(n, "state", "stars")) = !on;
    }
    const float v = Smooth(n, "on", on ? 1.f : 0.f, 14.f);
    const std::string label = LocP(n, "label");
    Txt(dl, FontBody(), 0.95f, ImVec2(ta.x + 4.f * S(), ta.y + 10.f * S()), ColText(), label.c_str());
    const ImVec2 sa(tb.x - 54.f * S(), ta.y + 8.f * S()), sb(tb.x - 4.f * S(), ta.y + 32.f * S());
    dl->AddRectFilled(sa, sb, Mix(C(255, 255, 255, 30), Acc(), v), 12.f * S());
    dl->AddCircleFilled(ImVec2(sa.x + 12.f * S() + (sb.x - sa.x - 24.f * S()) * v, (sa.y + sb.y) * 0.5f), 8.5f * S(), ColText(), 16);
}

// A line of information in two rows, a small caption and the value. `item` picks the value: imgui (the version), context (the shared ImGui context), display (the
// size of the screen and the layout scale), draw (what the frame renders), host (the host's API version and the game build it was made for). Parameters: item, label.
void Info(const StlGuiNode* n) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float w = AvailW();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, 46.f * S()));
    const ImGuiIO& io = ImGui::GetIO();
    char v[160] = "";
    const std::string item = StrP(n, "item", "imgui");
    if (item == "imgui") snprintf(v, sizeof(v), "%s", ImGui::GetVersion());
    else if (item == "context") snprintf(v, sizeof(v), "%p", g_ctx->imgui_context);
    else if (item == "display") snprintf(v, sizeof(v), "%.0f x %.0f   x%.2f", io.DisplaySize.x, io.DisplaySize.y, S());
    else if (item == "draw") snprintf(v, sizeof(v), "%d vtx   %d idx   %d windows", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderWindows);
    else if (item == "host") snprintf(v, sizeof(v), "API %u   exe 0x%08X", g_ctx->api_version, g_ctx->api->game_exe_timestamp);
    const std::string label = LocP(n, "label", item.c_str());
    Txt(dl, FontBold(), 0.62f, a, Al(Acc(), 0.8f), label.c_str());
    Txt(dl, FontBody(), 0.92f, a + ImVec2(0, 14.f * S()), ColText(), v);
}

}  // namespace

void AddPagesComponents(std::vector<ComponentDef>& out) {
    out.push_back({ "argon_heading", Heading });
    out.push_back({ "argon_ledger", Ledger });
    out.push_back({ "argon_resource_detail", ResourceDetail });
    out.push_back({ "argon_speed_dial", SpeedDial });
    out.push_back({ "argon_calendar", Calendar });
    out.push_back({ "argon_spark", Spark });
    out.push_back({ "argon_effect_card", EffectCard });
    out.push_back({ "argon_effect_log", EffectLogEl });
    out.push_back({ "argon_theme_picker", ThemePicker });
    out.push_back({ "argon_switch", Switch });
    out.push_back({ "argon_info", Info });
}

}  // namespace argon
