#include "Simulator.h"
#include <algorithm>
#include <cmath>

static float v2dist(Vector2 a, Vector2 b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

static float pSegDist(Vector2 p, Vector2 a, Vector2 b) {
    float dx = b.x - a.x, dy = b.y - a.y, l2 = dx * dx + dy * dy;
    if (l2 < 1e-6f) return v2dist(p, a);
    float t = std::max(0.0f, std::min(1.0f, ((p.x - a.x) * dx + (p.y - a.y) * dy) / l2));
    return v2dist(p, {a.x + t * dx, a.y + t * dy});
}

static float bezierDist(Vector2 from, Vector2 to, Vector2 p) {
    float dx = std::max(std::fabsf(to.x - from.x) * 0.5f, 60.0f);
    Vector2 cp1 = {from.x + dx, from.y};
    Vector2 cp2 = {to.x - dx, to.y};
    Vector2 prev = from;
    float mn = 1e9f;
    for (int i = 1; i <= 40; i++) {
        float t = (float)i / 40.0f;
        float it = 1.0f - t;
        Vector2 cur = {
            it * it * it * from.x + 3 * it * it * t * cp1.x + 3 * it * t * t * cp2.x + t * t * t * to.x,
            it * it * it * from.y + 3 * it * it * t * cp1.y + 3 * it * t * t * cp2.y + t * t * t * to.y
        };
        mn = std::min(mn, pSegDist(p, prev, cur));
        prev = cur;
    }
    return mn;
}

static void DrawBezier(Vector2 a, Vector2 b, Color col, float thick) {
    float dx = std::max(std::fabsf(b.x - a.x) * 0.5f, 60.0f);
    Vector2 cp1 = {a.x + dx, a.y};
    Vector2 cp2 = {b.x - dx, b.y};
    Vector2 prev = a;
    for (int i = 1; i <= 40; i++) {
        float t = (float)i / 40.0f;
        float it = 1.0f - t;
        Vector2 cur = {
            it * it * it * a.x + 3 * it * it * t * cp1.x + 3 * it * t * t * cp2.x + t * t * t * b.x,
            it * it * it * a.y + 3 * it * it * t * cp1.y + 3 * it * t * t * cp2.y + t * t * t * b.y
        };
        DrawLineEx(prev, cur, thick, col);
        prev = cur;
    }
}

Simulator::Simulator() : simPlaying(false), simAccum(0.0) {}

Simulator::~Simulator() {}

void Simulator::AddGate(const Gate& gate) {
    gates.push_back(gate);
}

void Simulator::RemoveGate(int idx) {
    if (idx < 0 || idx >= (int)gates.size()) return;
    wires.erase(std::remove_if(wires.begin(), wires.end(),
        [idx](const Wire& w) { return w.gateA == idx || w.gateB == idx; }), wires.end());
    for (auto& w : wires) {
        if (w.gateA > idx) w.gateA--;
        if (w.gateB > idx) w.gateB--;
    }
    gates.erase(gates.begin() + idx);
}

void Simulator::AddWire(const Wire& wire) {
    wires.push_back(wire);
}

void Simulator::RemoveWire(int idx) {
    if (idx >= 0 && idx < (int)wires.size())
        wires.erase(wires.begin() + idx);
}

void Simulator::DeleteGate(int idx) {
    RemoveGate(idx);
}

void Simulator::Propagate() {
    for (auto& g : gates)
        for (auto& n : g.nodes)
            n.signal = false;

    for (auto& g : gates)
        if (g.type == GateType::INPUT) {
            g.outputSig = g.checked;
            if (!g.nodes.empty())
                g.nodes[0].signal = g.checked;
        }

    for (int pass = 0; pass < (int)gates.size() + 1; pass++) {
        for (const auto& w : wires)
            gates[w.gateB].nodes[w.nodeB].signal = gates[w.gateA].nodes[w.nodeA].signal;

        for (auto& g : gates) {
            switch (g.type) {
                case GateType::NOT:
                    if (g.nodes.size() >= 2) {
                        g.outputSig = !g.nodes[0].signal;
                        g.nodes[1].signal = g.outputSig;
                    }
                    break;
                case GateType::AND:
                    if (g.nodes.size() >= 3) {
                        g.outputSig = g.nodes[0].signal && g.nodes[1].signal;
                        g.nodes[2].signal = g.outputSig;
                    }
                    break;
                case GateType::OR:
                    if (g.nodes.size() >= 3) {
                        g.outputSig = g.nodes[0].signal || g.nodes[1].signal;
                        g.nodes[2].signal = g.outputSig;
                    }
                    break;
                case GateType::OUTPUT:
                    if (!g.nodes.empty())
                        g.outputSig = g.nodes[0].signal;
                    break;
                case GateType::CLOCK:
                    if (!g.nodes.empty())
                        g.nodes[0].signal = g.checked;
                    break;
                case GateType::MUX:
                    if (g.nodes.size() >= 4) {
                        bool y = g.nodes[2].signal ? g.nodes[1].signal : g.nodes[0].signal;
                        g.outputSig = y;
                        g.nodes[3].signal = y;
                    }
                    break;
                case GateType::DMUX:
                    if (g.nodes.size() >= 4) {
                        g.nodes[2].signal = !g.nodes[1].signal && g.nodes[0].signal;
                        g.nodes[3].signal = g.nodes[1].signal && g.nodes[0].signal;
                        g.outputSig = g.nodes[2].signal;
                    }
                    break;
                case GateType::ENCODER:
                    if (g.nodes.size() >= 6) {
                        int idx = -1;
                        for (int k = 3; k >= 0; k--)
                            if (g.nodes[k].signal) { idx = k; break; }
                        g.nodes[4].signal = (idx >= 2);
                        g.nodes[5].signal = (idx == 1 || idx == 3);
                        g.outputSig = g.nodes[4].signal;
                    }
                    break;
                case GateType::DECODER:
                    if (g.nodes.size() >= 6) {
                        bool a = g.nodes[0].signal, b = g.nodes[1].signal;
                        g.nodes[2].signal = !a && !b;
                        g.nodes[3].signal = a && !b;
                        g.nodes[4].signal = !a && b;
                        g.nodes[5].signal = a && b;
                        g.outputSig = g.nodes[2].signal;
                    }
                    break;
                case GateType::SEG7:
                    break;
                case GateType::CUSTOM: {
                    for (int k = 0; k < (int)g.inPins.size() && k < (int)g.nodes.size(); k++)
                        g.intGates[g.inPins[k]].checked = g.nodes[k].signal;
                    for (auto& ig : g.intGates)
                        for (auto& n : ig.nodes)
                            n.signal = false;
                    for (auto& ig : g.intGates)
                        if (ig.type == GateType::INPUT) {
                            ig.outputSig = ig.checked;
                            if (!ig.nodes.empty())
                                ig.nodes[0].signal = ig.checked;
                        }
                    for (int p = 0; p < (int)g.intGates.size() + 1; p++) {
                        for (const auto& w : g.intWires)
                            g.intGates[w.gateB].nodes[w.nodeB].signal = g.intGates[w.gateA].nodes[w.nodeA].signal;
                        for (auto& ig : g.intGates) {
                            switch (ig.type) {
                                case GateType::NOT:
                                    if (ig.nodes.size() >= 2) {
                                        ig.outputSig = !ig.nodes[0].signal;
                                        ig.nodes[1].signal = ig.outputSig;
                                    }
                                    break;
                                case GateType::AND:
                                    if (ig.nodes.size() >= 3) {
                                        ig.outputSig = ig.nodes[0].signal && ig.nodes[1].signal;
                                        ig.nodes[2].signal = ig.outputSig;
                                    }
                                    break;
                                case GateType::OR:
                                    if (ig.nodes.size() >= 3) {
                                        ig.outputSig = ig.nodes[0].signal || ig.nodes[1].signal;
                                        ig.nodes[2].signal = ig.outputSig;
                                    }
                                    break;
                                case GateType::OUTPUT:
                                    if (!ig.nodes.empty())
                                        ig.outputSig = ig.nodes[0].signal;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    int nIn = (int)g.inPins.size();
                    for (int k = 0; k < (int)g.outPins.size(); k++)
                        if (nIn + k < (int)g.nodes.size())
                            g.nodes[nIn + k].signal = g.intGates[g.outPins[k]].outputSig;
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void Simulator::DrawAll(bool panning, const Camera2D& cam) {
    Color gridColor = {22, 24, 28, 255};

    for (int wi = 0; wi < (int)wires.size(); wi++) {
        const Wire& w = wires[wi];
        Vector2 from = gates[w.gateA].nodes[w.nodeA].pos;
        Vector2 to = gates[w.gateB].nodes[w.nodeB].pos;
        bool sig = gates[w.gateA].nodes[w.nodeA].signal;
        Color wc = sig ? SIG_ON : Color{36, 40, 46, 255};
        DrawBezier(from, to, wc, sig ? 2.0f : 1.5f);
        DrawCircleV(from, 3, wc);
        DrawCircleV(to, 3, wc);
    }

    for (auto& g : gates) {
        Color body = g.bodyColor();
        Color stripe = {
            (unsigned char)std::min(255, body.r + 40),
            (unsigned char)std::min(255, body.g + 30),
            (unsigned char)std::min(255, body.b + 20),
            255
        };

        DrawRectangle((int)g.rect.x + 3, (int)g.rect.y + 3, (int)g.rect.width, (int)g.rect.height, {0, 0, 0, 100});
        DrawRectangleRec(g.rect, body);
        DrawRectangle((int)g.rect.x, (int)g.rect.y, (int)g.rect.width, 2, stripe);
        DrawRectangleLinesEx(g.rect, 1, BORDER_DIM);

        const char* lbl = g.displayName();
        int fs = 14, tw = MeasureText(lbl, fs);
        while (tw > (int)g.rect.width - 8 && fs > 8) {
            fs--;
            tw = MeasureText(lbl, fs);
        }
        DrawText(lbl, (int)(g.rect.x + g.rect.width / 2 - tw / 2), (int)(g.rect.y + g.rect.height / 2 - fs / 2), fs, TEXT_PRI);

        if (g.renaming) {
            DrawRectangleLinesEx(g.rect, 1, {180, 140, 0, 255});
            if ((int)(GetTime() * 2) % 2 == 0)
                DrawRectangle((int)(g.rect.x + g.rect.width / 2 + tw / 2 + 2), (int)(g.rect.y + g.rect.height / 2 - fs / 2), 1, fs, {220, 180, 0, 255});
        }

        Rectangle dr = g.deleteRect();
        DrawRectangleRec(dr, Color{DANGER.r / 2, DANGER.g / 2, DANGER.b / 2, 140});
        DrawText("x", (int)(dr.x + 3), (int)(dr.y + 2), 8, TEXT_DIM);

        if (g.type == GateType::INPUT) {
            Rectangle cb = g.checkboxRect();
            Color cbFill = g.checked ? Color{SIG_ON.r, SIG_ON.g, SIG_ON.b, 180} : Color{18, 20, 24, 255};
            DrawRectangleRec(cb, cbFill);
            DrawRectangleLinesEx(cb, 1, g.checked ? SIG_ON : BORDER_LIT);
        }

        if (g.type == GateType::CLOCK) {
            char hzBuf[16];
            if (g.clockHz >= 1.0f)
                sprintf(hzBuf, "%.0fHz", g.clockHz);
            else if (g.clockHz >= 0.1f)
                sprintf(hzBuf, "%.1fHz", g.clockHz);
            else
                sprintf(hzBuf, "%.2fHz", g.clockHz);
            int hzw = MeasureText(hzBuf, 11);
            DrawText(hzBuf, (int)(g.rect.x + g.rect.width / 2 - hzw / 2), (int)(g.rect.y + g.rect.height - 16), 11,
                     g.checked ? SIG_ON : TEXT_DIM);
        }

        if (g.type == GateType::OUTPUT) {
            const char* badge = g.outputSig ? "HI" : "LO";
            Color bc = g.outputSig ? Color{SIG_ON.r, SIG_ON.g, SIG_ON.b, 210} : Color{44, 14, 12, 210};
            int bfs = 10, btw = MeasureText(badge, bfs);
            Rectangle pill = {g.rect.x + g.rect.width / 2 - (float)(btw + 10) / 2, g.rect.y - 18, (float)(btw + 10), 14};
            DrawRectangleRec(pill, bc);
            DrawRectangleLinesEx(pill, 1, g.outputSig ? SIG_ON : BORDER_DIM);
            DrawText(badge, (int)(pill.x + (pill.width - btw) / 2), (int)(pill.y + 2), bfs, {0, 0, 0, 255});
        }

        for (const auto& n : g.nodes) {
            float half = NODE_SZ / 2.0f;
            Rectangle nr = {n.pos.x - half, n.pos.y - half, (float)NODE_SZ, (float)NODE_SZ};
            Color nFill = n.signal ? SIG_ON : Color{24, 26, 32, 255};
            Color nBord = n.signal ? Color{SIG_ON.r, SIG_ON.g, SIG_ON.b, 160} : BORDER_LIT;
            DrawRectangleRec(nr, nFill);
            DrawRectangleLinesEx(nr, 1, nBord);
            DrawRectangle((int)(n.pos.x - 1), (int)(n.pos.y - 1), 2, 2, n.signal ? Color{0, 0, 0, 180} : Color{50, 56, 66, 255});
        }
    }
}