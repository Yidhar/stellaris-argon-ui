// The drawing helpers of the components. Most of them are the ones of the Command Deck (stellaris-guiexpand, src/deck.cpp), ported to read the frame state
// through the context of the element being drawn.
#include "draw.h"

namespace argon {

const StlGuiCallbackCtx* g_ctx = nullptr;

ImU32 Al(ImU32 c, float a) {
    const uint32_t base = (c >> 24) & 255;
    const uint32_t v = (uint32_t)std::clamp(base * a, 0.f, 255.f);
    return (c & 0x00FFFFFF) | (v << 24);
}
ImU32 Mix(ImU32 x, ImU32 y, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [&](int sh) { return (uint32_t)(((x >> sh) & 255) * (1 - t) + ((y >> sh) & 255) * t); };
    return (ch(24) << 24) | (ch(16) << 16) | (ch(8) << 8) | ch(0);
}

void Fmt(char* out, size_t n, double v, bool sign) {
    const double a = fabs(v);
    const char* sg = sign && v > 0.004 ? "+" : "";
    if (a >= 1e9) snprintf(out, n, "%s%.2fB", sg, v / 1e9);
    else if (a >= 1e6) snprintf(out, n, "%s%.2fM", sg, v / 1e6);
    else if (a >= 1e4) snprintf(out, n, "%s%.1fk", sg, v / 1e3);
    else if (a >= 100) snprintf(out, n, "%s%.0f", sg, v);
    else snprintf(out, n, "%s%.1f", sg, v);
}

float Smooth(const void* owner, const char* name, float target, float rate) {
    static std::unordered_map<std::string, float> anim;
    char key[96];
    snprintf(key, sizeof(key), "%p/%s", owner, name);
    auto it = anim.try_emplace(key, target).first;
    it->second += (target - it->second) * (1.f - expf(-rate * Dt()));
    return it->second;
}

ImVec2 TextSz(ImFont* f, float k, const char* s) { return f->CalcTextSizeA(f->FontSize * k * Fit(), FLT_MAX, 0.f, s); }
void Txt(ImDrawList* dl, ImFont* f, float k, ImVec2 p, ImU32 col, const char* s, int align) {
    const float size = f->FontSize * k * Fit();
    if (align) {
        const ImVec2 sz = f->CalcTextSizeA(size, FLT_MAX, 0.f, s);
        p.x -= align == 1 ? sz.x * 0.5f : sz.x;
    }
    dl->AddText(f, size, p, col, s);
}
void TxtF(ImDrawList* dl, ImFont* f, float k, ImVec2 p, ImU32 col, int align, const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Txt(dl, f, k, p, col, buf, align);
}

bool Hit(ImVec2 a, ImVec2 b, const char* id) {
    ImGui::SetCursorScreenPos(a);
    ImGui::InvisibleButton(id, ImVec2(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y)));
    return ImGui::IsItemClicked();
}

ImU32 ResColor(const std::string& k) {
    if (k == "energy") return C(255, 214, 64, 255);
    if (k == "minerals") return C(255, 104, 96, 255);
    if (k == "food") return C(124, 232, 112, 255);
    if (k == "consumer_goods") return C(255, 152, 64, 255);
    if (k == "alloys") return C(132, 192, 255, 255);
    if (k == "influence") return C(192, 132, 255, 255);
    if (k == "unity") return C(255, 122, 212, 255);
    if (k == "physics_research") return C(84, 204, 255, 255);
    if (k == "society_research") return C(112, 255, 172, 255);
    if (k == "engineering_research") return C(255, 172, 84, 255);
    return C(170, 180, 215, 255);
}

void Glow(ImDrawList* dl, ImVec2 c, float r, ImU32 col, float strength) {
    const int N = 14;
    for (int i = 0; i < N; ++i) {
        const float t = (float)i / N;
        dl->AddCircleFilled(c, r * (1.f - t * 0.97f), Al(col, strength / N * (0.4f + 1.2f * t)), 40);
    }
}

void Stars(ImDrawList* dl, ImVec2 a, ImVec2 b, double t) {
    uint32_t s = 20260717u;
    auto rnd = [&]() {
        s = s * 1664525u + 1013904223u;
        return (s >> 8) / 16777216.f;
    };
    const float w = b.x - a.x, h = b.y - a.y;
    for (int i = 0; i < 120; ++i) {
        const float u = rnd(), v = rnd(), d = 0.25f + 0.75f * rnd(), ph = rnd() * 6.28f;
        const float x = a.x + fmodf(u + (float)t * 0.004f * d, 1.f) * w, y = a.y + v * h;
        const float tw = 0.55f + 0.45f * sinf((float)t * (0.7f + d * 1.5f) + ph);
        const int al = (int)(255 * tw * d * 0.75f);
        dl->AddCircleFilled(ImVec2(x, y), (0.6f + d * 1.0f) * S(), C(205, 222, 255, al), 6);
        if (d > 0.92f) {
            const float L = 5.f * S() * tw;
            dl->AddLine(ImVec2(x - L, y), ImVec2(x + L, y), C(205, 222, 255, al / 3));
            dl->AddLine(ImVec2(x, y - L), ImVec2(x, y + L), C(205, 222, 255, al / 3));
        }
    }
    const float cyc = fmodf((float)t, 9.f) / 1.1f;  // a shooting star every 9 s
    if (cyc < 1.f) {
        const ImVec2 p0(a.x + w * (0.15f + 0.5f * cyc), a.y + h * (0.05f + 0.45f * cyc));
        for (int k = 0; k < 10; ++k)
            dl->AddLine(p0 - ImVec2(k * 9.f * S(), k * 4.f * S()), p0 - ImVec2((k + 1) * 9.f * S(), (k + 1) * 4.f * S()),
                        C(220, 235, 255, (int)(200 * (1 - cyc) * (1 - k / 10.f))), 1.6f * S());
    }
}

void Panel(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, ImU32 accent, float alpha) {
    dl->AddRectFilled(a, b, Al(g_ctx->theme->panel, alpha), r);
    dl->AddRectFilledMultiColor(ImVec2(a.x + r * 0.4f, a.y + 1), ImVec2(b.x - r * 0.4f, a.y + (b.y - a.y) * 0.45f), C(255, 255, 255, 12),
                                C(255, 255, 255, 12), C(255, 255, 255, 0), C(255, 255, 255, 0));
    dl->AddRect(a, b, C(255, 255, 255, 24), r, 0, 1.f);
    dl->AddRectFilledMultiColor(ImVec2(a.x + r, a.y), ImVec2(a.x + r + (b.x - a.x - 2 * r) * 0.35f, a.y + 2.f * S()), Al(accent, 0.f), accent,
                                accent, Al(accent, 0.f));
}

void CardTitle(ImDrawList* dl, ImVec2 a, const char* en, const char* zh) {
    Txt(dl, FontBold(), 0.62f, ImVec2(a.x + 18 * S(), a.y + 14 * S()), Al(Acc(), 0.85f), en);
    Txt(dl, FontBold(), 0.95f, ImVec2(a.x + 18 * S(), a.y + 28 * S()), ColText(), zh);
}

void AreaFill(ImDrawList* dl, const ImVec2* p, int n, float base_y, ImU32 top, ImU32 bottom) {
    if (n < 2) return;
    dl->PrimReserve((n - 1) * 6, (n - 1) * 4);
    const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
    for (int i = 0; i < n - 1; ++i) {
        const ImDrawIdx idx = (ImDrawIdx)dl->_VtxCurrentIdx;
        dl->PrimWriteVtx(p[i], uv, top);
        dl->PrimWriteVtx(p[i + 1], uv, top);
        dl->PrimWriteVtx(ImVec2(p[i + 1].x, base_y), uv, bottom);
        dl->PrimWriteVtx(ImVec2(p[i].x, base_y), uv, bottom);
        dl->PrimWriteIdx(idx);
        dl->PrimWriteIdx(idx + 1);
        dl->PrimWriteIdx(idx + 2);
        dl->PrimWriteIdx(idx);
        dl->PrimWriteIdx(idx + 2);
        dl->PrimWriteIdx(idx + 3);
    }
}

void Ring(ImDrawList* dl, ImVec2 c, float r, float th, float frac, ImU32 ca, ImU32 cb) {
    const float a0 = IM_PI * 0.75f, span = IM_PI * 1.5f;
    dl->PathArcTo(c, r, a0, a0 + span, 64);
    dl->PathStroke(C(255, 255, 255, 18), 0, th);
    frac = std::clamp(frac, 0.f, 1.f);
    if (frac <= 0.002f) return;
    const int segs = std::max(2, (int)(48 * frac));
    for (int k = 0; k < segs; ++k) {
        const float f0 = frac * k / segs, f1 = frac * (k + 1) / segs;
        dl->PathArcTo(c, r, a0 + span * f0, a0 + span * f1 + 0.012f, 3);
        dl->PathStroke(Mix(ca, cb, (f0 + f1) * 0.5f / std::max(frac, 0.05f)), 0, th);
    }
    const float ae = a0 + span * frac;
    const ImVec2 tip(c.x + cosf(ae) * r, c.y + sinf(ae) * r);
    dl->AddCircleFilled(tip, th * 0.5f, cb, 12);
    dl->AddCircleFilled(c + ImVec2(cosf(a0) * r, sinf(a0) * r), th * 0.5f, ca, 12);
    Glow(dl, tip, th * 1.7f, cb, 0.55f);
}

void Icon(ImDrawList* dl, const std::string& key, ImVec2 c, float r, ImU32 col) {
    auto P = [&](float x, float y) { return ImVec2(c.x + x * r, c.y + y * r); };
    const float th = std::max(1.2f, r * 0.2f);
    if (key == "energy") {
        const ImVec2 p[6] = { P(0.25f, -1), P(-0.55f, 0.15f), P(-0.05f, 0.15f), P(-0.25f, 1), P(0.6f, -0.25f), P(0.05f, -0.25f) };
        dl->AddTriangleFilled(p[0], p[1], p[2], col);
        dl->AddTriangleFilled(p[0], p[2], p[5], col);
        dl->AddTriangleFilled(p[5], p[4], p[3], col);
        dl->AddTriangleFilled(p[5], p[3], p[2], col);
    } else if (key == "minerals") {
        const ImVec2 gem[4] = { P(0, -1), P(0.85f, -0.2f), P(0, 1), P(-0.85f, -0.2f) };
        dl->AddConvexPolyFilled(gem, 4, Al(col, 0.85f));
        dl->AddTriangleFilled(P(0, -1), P(0.85f, -0.2f), P(0, -0.2f), Al(C(255, 255, 255, 255), 0.35f));
        dl->AddLine(P(-0.85f, -0.2f), P(0.85f, -0.2f), C(0, 0, 0, 90), 1.f);
        dl->AddLine(P(0, -0.2f), P(0, 1), C(0, 0, 0, 70), 1.f);
    } else if (key == "food") {
        ImVec2 pts[20];
        for (int i = 0; i < 10; ++i) {
            const float u = -1.f + 2.f * i / 9.f, w = 0.62f * powf(std::max(0.f, 1 - u * u), 0.85f);
            pts[i] = P((u - w) * 0.7071f, (-u - w) * 0.7071f);
            pts[19 - i] = P((u + w) * 0.7071f, (-u + w) * 0.7071f);
        }
        dl->AddConvexPolyFilled(pts, 20, col);
        dl->AddLine(P(-0.7f, 0.7f), P(0.7f, -0.7f), C(0, 0, 0, 90), 1.2f);
    } else if (key == "consumer_goods") {
        dl->AddRectFilled(P(-0.8f, -0.45f), P(0.8f, 0.9f), Al(col, 0.9f), r * 0.2f);
        dl->AddRectFilled(P(-0.9f, -0.75f), P(0.9f, -0.35f), col, r * 0.15f);
        dl->AddRectFilled(P(-0.12f, -0.75f), P(0.12f, 0.9f), C(0, 0, 0, 90));
    } else if (key == "alloys") {
        const ImVec2 lower[4] = { P(-0.95f, 0.85f), P(0.95f, 0.85f), P(0.65f, 0.1f), P(-0.65f, 0.1f) };
        const ImVec2 upper[4] = { P(-0.55f, 0.0f), P(0.55f, 0.0f), P(0.3f, -0.75f), P(-0.3f, -0.75f) };
        dl->AddConvexPolyFilled(lower, 4, Al(col, 0.85f));
        dl->AddConvexPolyFilled(upper, 4, col);
    } else if (key == "influence") {
        const float rot[2] = { 0.f, IM_PI / 4 };
        for (float a : rot) {
            ImVec2 q[4];
            for (int i = 0; i < 4; ++i) q[i] = P(cosf(a + i * IM_PI / 2 + IM_PI / 4) * 1.1f, sinf(a + i * IM_PI / 2 + IM_PI / 4) * 1.1f);
            dl->AddConvexPolyFilled(q, 4, Al(col, 0.8f));
        }
        dl->AddCircleFilled(c, r * 0.28f, C(11, 15, 32, 255), 12);
    } else if (key == "unity") {
        for (int i = 0; i < 3; ++i) {
            const float a = -IM_PI / 2 + i * 2 * IM_PI / 3;
            dl->AddCircle(P(cosf(a) * 0.45f, sinf(a) * 0.45f), r * 0.55f, Al(col, 0.9f), 20, th);
        }
    } else if (key == "physics_research") {
        for (int e = 0; e < 3; ++e) {
            ImVec2 pts[24];
            for (int i = 0; i < 24; ++i) {
                const float t = i * 2 * IM_PI / 24, x = cosf(t) * 1.0f, y = sinf(t) * 0.38f, a = e * IM_PI / 3;
                pts[i] = P(x * cosf(a) - y * sinf(a), x * sinf(a) + y * cosf(a));
            }
            dl->AddPolyline(pts, 24, Al(col, 0.9f), ImDrawFlags_Closed, th * 0.8f);
        }
        dl->AddCircleFilled(c, r * 0.22f, col, 12);
    } else if (key == "society_research") {
        const ImVec2 n[3] = { P(0, -0.8f), P(0.8f, 0.6f), P(-0.8f, 0.6f) };
        for (int i = 0; i < 3; ++i) dl->AddLine(n[i], n[(i + 1) % 3], Al(col, 0.7f), th * 0.8f);
        for (int i = 0; i < 3; ++i) dl->AddCircleFilled(n[i], r * 0.3f, col, 12);
    } else if (key == "engineering_research") {
        for (int i = 0; i < 8; ++i) {
            const float a = i * IM_PI / 4, ca = cosf(a), sa = sinf(a);
            dl->AddLine(P(ca * 0.6f, sa * 0.6f), P(ca * 0.98f, sa * 0.98f), col, r * 0.34f);
        }
        dl->AddCircle(c, r * 0.62f, col, 20, th * 1.3f);
        dl->AddCircleFilled(c, r * 0.2f, col, 10);
    } else {
        ImVec2 h[6];
        for (int i = 0; i < 6; ++i) h[i] = P(cosf(i * IM_PI / 3) * 0.95f, sinf(i * IM_PI / 3) * 0.95f);
        dl->AddPolyline(h, 6, col, ImDrawFlags_Closed, th);
    }
}

void TabIcon(ImDrawList* dl, int tab, ImVec2 c, float r, ImU32 col) {
    auto P = [&](float x, float y) { return ImVec2(c.x + x * r, c.y + y * r); };
    const float th = std::max(1.5f, r * 0.14f);
    switch (tab) {
    case 0:  // overview: four tiles
        for (int i = 0; i < 4; ++i) {
            const float x = (i % 2) ? 0.08f : -0.92f, y = (i / 2) ? 0.08f : -0.92f;
            dl->AddRectFilled(P(x, y), P(x + 0.84f, y + 0.84f), Al(col, i == 0 ? 1.f : 0.6f), r * 0.14f);
        }
        break;
    case 1:  // economy: bar chart
        for (int i = 0; i < 4; ++i) {
            const float h[4] = { 0.5f, 1.1f, 0.8f, 1.6f };
            dl->AddRectFilled(P(-1.0f + i * 0.52f, 0.9f - h[i]), P(-0.62f + i * 0.52f, 0.9f), Al(col, 0.5f + 0.5f * (i / 3.f)), r * 0.08f);
        }
        break;
    case 2:  // time: clock
        dl->AddCircle(c, r * 0.95f, col, 28, th);
        dl->AddLine(c, P(0, -0.6f), col, th);
        dl->AddLine(c, P(0.45f, 0.2f), col, th);
        break;
    case 3: {  // script: chevrons and cursor
        const ImVec2 chev[3] = { P(-0.9f, -0.6f), P(-0.2f, 0.0f), P(-0.9f, 0.6f) };
        dl->AddPolyline(chev, 3, col, 0, th);
        dl->AddLine(P(0.05f, 0.65f), P(0.95f, 0.65f), col, th);
        break;
    }
    default:  // settings: sliders
        for (int i = 0; i < 3; ++i) {
            const float y = -0.6f + i * 0.6f, kx[3] = { -0.35f, 0.45f, -0.05f };
            dl->AddLine(P(-0.95f, y), P(0.95f, y), Al(col, 0.55f), th);
            dl->AddCircleFilled(P(kx[i], y), r * 0.2f, col, 12);
        }
        break;
    }
}

}  // namespace argon
