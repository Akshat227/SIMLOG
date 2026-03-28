#include "raylib.h"
#include "Gate.h"
#include "Simulator.h"
#include <vector>
#include <cstring>
#include <cmath>

constexpr int FOOTER_H = 128;
constexpr int PANEL_W = 250;
constexpr int GW = 110;
constexpr int GH = 70;
constexpr int BTN_W = 100;
constexpr int BTN_H = 42;
constexpr int BTN_GAP = 14;

constexpr float ZOOM_MIN = 0.15f;
constexpr float ZOOM_MAX = 4.0f;
constexpr float ZOOM_STEP = 0.12f;

constexpr Color PANEL_BG = {10, 11, 13, 255};
constexpr Color PANEL_CARD = {18, 20, 23, 255};

static float v2dist(Vector2 a, Vector2 b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

static float bezierDist(Vector2 from, Vector2 to, Vector2 p) {
    float dx = std::max(fabsf(to.x - from.x) * 0.5f, 60.0f);
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
        float px = prev.x - p.x, py = prev.y - p.y;
        float segLen = sqrtf(px * px + py * py);
        if (segLen < 1e-6f) mn = std::min(mn, v2dist(p, prev));
        else {
            float t2 = ((p.x - prev.x) * px + (p.y - prev.y) * py) / (segLen * segLen);
            t2 = std::max(0.0f, std::min(1.0f, t2));
            float ix = prev.x + t2 * px, iy = prev.y + t2 * py;
            mn = std::min(mn, v2dist(p, {ix, iy}));
        }
        prev = cur;
    }
    return mn;
}

static void DrawBezier(Vector2 a, Vector2 b, Color col, float thick) {
    float dx = std::max(fabsf(b.x - a.x) * 0.5f, 60.0f);
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

static bool DrawToolbarButton(int x, int y, int w, int h, const char* label, Color accent) {
    Rectangle r = {(float)x, (float)y, (float)w, (float)h};
    bool hov = CheckCollisionPointRec(GetMousePosition(), r);
    bool pressed = hov && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    Color face = pressed ? BTN_COLOR : (hov ? BTN_HOVER : BTN_COLOR);
    DrawRectangleRec(r, face);
    DrawRectangle(x, y + h - 2, w, 2, hov ? accent : Color{(unsigned char)(accent.r / 2), (unsigned char)(accent.g / 2), (unsigned char)(accent.b / 2), 160});
    DrawRectangleLinesEx(r, 1, hov ? BORDER_LIT : BORDER_DIM);
    int fs = 12, tw = MeasureText(label, fs);
    DrawText(label, x + (w - tw) / 2, y + (h - fs) / 2, fs, hov ? TEXT_PRI : TEXT_DIM);
    return hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static Vector2 screenToWorld(Vector2 s, Camera2D cam) {
    return GetScreenToWorld2D(s, cam);
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "GateSim");
    SetWindowMinSize(640, 400);
    SetTargetFPS(60);

    Camera2D cam = {};
    cam.zoom = 1.0f;
    int canvasW = GetScreenWidth() - PANEL_W;
    int canvasH = GetScreenHeight() - FOOTER_H;
    cam.offset = {(float)(canvasW) / 2.0f, (float)(canvasH) / 2.0f};
    cam.target = cam.offset;

    bool panning = false;
    Vector2 panStart = {}, camTargetAtPanStart = {};

    Simulator simulator;

    int draggedIdx = -1;
    bool wiring = false;
    int wireGateA = -1, wireNodeA = -1;

    bool dragSelecting = false;
    std::vector<bool> selected;
    Vector2 dragSelStartW = {};
    Vector2 dragSelCurW = {};

    while (!WindowShouldClose()) {
        canvasW = GetScreenWidth() - PANEL_W;
        canvasH = GetScreenHeight() - FOOTER_H;

        const int totalBarW = N_BTNS1 * BTN_W + (N_BTNS1 - 1) * BTN_GAP;
        const int barStartX = (canvasW - totalBarW) / 2;

        Vector2 mouseScreen = GetMousePosition();
        bool mouseInCanvas = (mouseScreen.x < canvasW && mouseScreen.y < canvasH);
        Vector2 mouseWorld = screenToWorld(mouseScreen, cam);

        if (IsKeyPressed(KEY_ESCAPE) && !wiring) {
            std::fill(selected.begin(), selected.end(), false);
        }

        if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
            for (int i = (int)simulator.GetGates().size() - 1; i >= 0; i--) {
                if (i < (int)selected.size() && selected[i]) {
                    simulator.DeleteGate(i);
                    if (draggedIdx == i) draggedIdx = -1;
                }
            }
            selected.resize(simulator.GetGates().size(), false);
        }

        if (mouseInCanvas) {
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                float nz = std::max(ZOOM_MIN, std::min(ZOOM_MAX, cam.zoom + wheel * ZOOM_STEP * cam.zoom));
                float r = cam.zoom / nz;
                cam.target.x = mouseWorld.x + (cam.target.x - mouseWorld.x) * r;
                cam.target.y = mouseWorld.y + (cam.target.y - mouseWorld.y) * r;
                cam.zoom = nz;
            }
        }

        bool panKey = IsKeyDown(KEY_SPACE);
        bool panBtn = IsMouseButtonDown(MOUSE_MIDDLE_BUTTON);
        if ((panBtn || panKey) && mouseInCanvas) {
            if (!panning) {
                panning = true;
                panStart = mouseScreen;
                camTargetAtPanStart = cam.target;
                SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
            }
            Vector2 d = {(mouseScreen.x - panStart.x) / cam.zoom, (mouseScreen.y - panStart.y) / cam.zoom};
            cam.target.x = camTargetAtPanStart.x - d.x;
            cam.target.y = camTargetAtPanStart.y - d.y;
        } else if (panning) {
            panning = false;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }

        if (IsKeyPressed(KEY_R)) {
            cam.zoom = 1.0f;
            cam.offset = {(float)canvasW / 2.0f, (float)canvasH / 2.0f};
            cam.target = cam.offset;
        }

        if (mouseScreen.y >= canvasH && mouseScreen.x < canvasW) {
            const int row1Y = GetScreenHeight() - FOOTER_H + 10;
            auto spawnGate = [&](GateType gtype) {
                Vector2 c = screenToWorld({(float)canvasW / 2.0f, (float)canvasH / 2.0f}, cam);
                float gw = GW, gh = GH;
                if (gtype == GateType::SEG7) { gw = 120; gh = 175; }
                else if (gtype == GateType::ENCODER || gtype == GateType::DECODER) { gw = 130; gh = 140; }
                else if (gtype == GateType::MUX || gtype == GateType::DMUX) { gw = 120; gh = 105; }
                Gate g2;
                g2.type = gtype;
                g2.rect = {c.x - gw / 2.0f, c.y - gh / 2.0f, gw, gh};
                g2.name[0] = '\0';
                g2.rebuildNodes();
                simulator.AddGate(g2);
                selected.push_back(false);
            };

            for (int i = 0; i < N_BTNS1; i++) {
                Rectangle r = {(float)(barStartX + i * (BTN_W + BTN_GAP)), (float)row1Y, (float)BTN_W, (float)BTN_H};
                if (CheckCollisionPointRec(mouseScreen, r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    spawnGate(TOOLBAR1[i].type);
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouseInCanvas && !panning) {
            bool handled = false;

            for (int i = (int)simulator.GetGates().size() - 1; i >= 0; i--) {
                if (CheckCollisionPointRec(mouseWorld, simulator.GetGates()[i].deleteRect())) {
                    simulator.DeleteGate(i);
                    if (draggedIdx == i) draggedIdx = -1;
                    selected.resize(simulator.GetGates().size(), false);
                    handled = true;
                    break;
                }
            }

            if (!handled) {
                for (auto& g : simulator.GetGates()) {
                    if (g.type == GateType::INPUT && CheckCollisionPointRec(mouseWorld, g.checkboxRect())) {
                        g.checked = !g.checked;
                        handled = true;
                        break;
                    }
                }
            }

            if (!handled && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
                int hitGate = -1, hitNode = -1;
                float hitR = NODE_HIT / cam.zoom;

                for (int gi = (int)simulator.GetGates().size() - 1; gi >= 0; gi--) {
                    for (int ni = 0; ni < (int)simulator.GetGates()[gi].nodes.size(); ni++) {
                        if (v2dist(mouseWorld, simulator.GetGates()[gi].nodes[ni].pos) <= hitR) {
                            hitGate = gi;
                            hitNode = ni;
                            goto foundNode;
                        }
                    }
                }
            foundNode:
                if (hitGate >= 0) {
                    if (!wiring) {
                        wiring = true;
                        wireGateA = hitGate;
                        wireNodeA = hitNode;
                    } else {
                        if (!(hitGate == wireGateA && hitNode == wireNodeA)) {
                            bool aOut = simulator.GetGates()[wireGateA].nodes[wireNodeA].isOutput;
                            bool bOut = simulator.GetGates()[hitGate].nodes[hitNode].isOutput;
                            if (aOut != bOut) {
                                Wire w = aOut ? Wire{wireGateA, wireNodeA, hitGate, hitNode} : Wire{hitGate, hitNode, wireGateA, wireNodeA};
                                simulator.AddWire(w);
                            }
                        }
                        wiring = false;
                        wireGateA = wireNodeA = -1;
                    }
                    handled = true;
                }
            }

            if (!handled && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
                for (int i = (int)simulator.GetGates().size() - 1; i >= 0; i--) {
                    if (CheckCollisionPointRec(mouseWorld, simulator.GetGates()[i].rect)) {
                        if (i >= (int)selected.size()) selected.resize(i + 1, false);
                        selected[i] = !selected[i];
                        handled = true;
                        break;
                    }
                }
            }

            if (!handled) {
                if (wiring) {
                    wiring = false;
                    wireGateA = wireNodeA = -1;
                } else {
                    bool hitGate2 = false;
                    for (int i = (int)simulator.GetGates().size() - 1; i >= 0; i--) {
                        if (CheckCollisionPointRec(mouseWorld, simulator.GetGates()[i].rect)) {
                            draggedIdx = i;
                            simulator.GetGates()[i].dragging = true;
                            simulator.GetGates()[i].dragOffset = {mouseWorld.x - simulator.GetGates()[i].rect.x,
                                                                  mouseWorld.y - simulator.GetGates()[i].rect.y};
                            hitGate2 = true;
                            break;
                        }
                    }
                    if (!hitGate2) {
                        dragSelecting = true;
                        dragSelStartW = mouseWorld;
                        dragSelCurW = mouseWorld;
                        if (!IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT))
                            std::fill(selected.begin(), selected.end(), false);
                    }
                }
            }
        }

        if (dragSelecting && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mouseInCanvas)
            dragSelCurW = mouseWorld;

        if (dragSelecting && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            dragSelecting = false;
            float rx = std::min(dragSelStartW.x, dragSelCurW.x);
            float ry = std::min(dragSelStartW.y, dragSelCurW.y);
            float rw = fabsf(dragSelCurW.x - dragSelStartW.x);
            float rh = fabsf(dragSelCurW.y - dragSelStartW.y);
            if (rw > 4 && rh > 4) {
                Rectangle selRect = {rx, ry, rw, rh};
                for (int i = 0; i < (int)simulator.GetGates().size(); i++)
                    if (CheckCollisionRecs(selRect, simulator.GetGates()[i].rect)) {
                        if (i >= (int)selected.size()) selected.resize(i + 1, false);
                        selected[i] = true;
                    }
            }
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && mouseInCanvas && !panning) {
            int bw = -1;
            float bd = 10.0f / cam.zoom;
            for (int wi = 0; wi < (int)simulator.GetWires().size(); wi++) {
                const Wire& w = simulator.GetWires()[wi];
                if (w.gateA >= (int)simulator.GetGates().size() || w.gateB >= (int)simulator.GetGates().size()) continue;
                if (w.nodeA >= (int)simulator.GetGates()[w.gateA].nodes.size() || w.nodeB >= (int)simulator.GetGates()[w.gateB].nodes.size()) continue;
                float d = bezierDist(simulator.GetGates()[w.gateA].nodes[w.nodeA].pos,
                                     simulator.GetGates()[w.gateB].nodes[w.nodeB].pos, mouseWorld);
                if (d < bd) { bd = d; bw = wi; }
            }
            if (bw >= 0) simulator.RemoveWire(bw);
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && draggedIdx >= 0 && !panning) {
            Gate& g = simulator.GetGates()[draggedIdx];
            g.rect.x = mouseWorld.x - g.dragOffset.x;
            g.rect.y = mouseWorld.y - g.dragOffset.y;
            g.rebuildNodes();
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && draggedIdx >= 0) {
            simulator.GetGates()[draggedIdx].dragging = false;
            draggedIdx = -1;
        }

        if (simulator.IsPlaying()) {
            simulator.Propagate();
        }

        BeginDrawing();
        ClearBackground(BG_COLOR);

        cam.offset = {(float)canvasW / 2.0f, (float)canvasH / 2.0f};

        BeginMode2D(cam);
        {
            Color gridColor = {22, 24, 28, 255};
            Vector2 tl = screenToWorld({0, 0}, cam);
            Vector2 br = screenToWorld({(float)canvasW, (float)canvasH}, cam);
            int gs = 30, gx0 = ((int)tl.x / gs - 1) * gs, gy0 = ((int)tl.y / gs - 1) * gs;
            for (int gx = gx0; gx < (int)br.x + gs; gx += gs)
                for (int gy = gy0; gy < (int)br.y + gs; gy += gs)
                    DrawPixel(gx, gy, gridColor);
        }

        int hovWire = -1;
        {
            float bd = 10.0f / cam.zoom;
            for (int wi = 0; wi < (int)simulator.GetWires().size(); wi++) {
                const Wire& w = simulator.GetWires()[wi];
                if (w.gateA >= (int)simulator.GetGates().size() || w.gateB >= (int)simulator.GetGates().size()) continue;
                if (w.nodeA >= (int)simulator.GetGates()[w.gateA].nodes.size() || w.nodeB >= (int)simulator.GetGates()[w.gateB].nodes.size()) continue;
                float d = bezierDist(simulator.GetGates()[w.gateA].nodes[w.nodeA].pos,
                                     simulator.GetGates()[w.gateB].nodes[w.nodeB].pos, mouseWorld);
                if (d < bd) { bd = d; hovWire = wi; }
            }
        }

        for (int wi = 0; wi < (int)simulator.GetWires().size(); wi++) {
            const Wire& w = simulator.GetWires()[wi];
            if (w.gateA >= (int)simulator.GetGates().size() || w.gateB >= (int)simulator.GetGates().size()) continue;
            if (w.nodeA >= (int)simulator.GetGates()[w.gateA].nodes.size() || w.nodeB >= (int)simulator.GetGates()[w.gateB].nodes.size()) continue;
            Vector2 from = simulator.GetGates()[w.gateA].nodes[w.nodeA].pos;
            Vector2 to = simulator.GetGates()[w.gateB].nodes[w.nodeB].pos;
            bool sig = simulator.GetGates()[w.gateA].nodes[w.nodeA].signal;
            bool hov = (wi == hovWire);
            Color wc = hov ? Color{DANGER.r + 30, DANGER.g, DANGER.b, 255} : (sig ? SIG_ON : Color{36, 40, 46, 255});
            float wthick = hov ? 2.5f : (sig ? 2.0f : 1.5f);
            DrawBezier(from, to, wc, wthick);
            DrawCircleV(from, 3, wc);
            DrawCircleV(to, 3, wc);
        }

        if (wiring && wireGateA >= 0 && wireNodeA >= 0 && wireGateA < (int)simulator.GetGates().size()
            && wireNodeA < (int)simulator.GetGates()[wireGateA].nodes.size()) {
            DrawBezier(simulator.GetGates()[wireGateA].nodes[wireNodeA].pos, mouseWorld,
                       {ACCENT_BLUE.r, ACCENT_BLUE.g, ACCENT_BLUE.b, 140}, 1.5f);
            DrawCircleV(simulator.GetGates()[wireGateA].nodes[wireNodeA].pos, 3, ACCENT_BLUE);
        }

        for (int i = 0; i < (int)simulator.GetGates().size(); i++)
            simulator.DrawAll(panning, cam);

        if (dragSelecting) {
            float rx = std::min(dragSelStartW.x, dragSelCurW.x);
            float ry = std::min(dragSelStartW.y, dragSelCurW.y);
            float rw = fabsf(dragSelCurW.x - dragSelStartW.x);
            float rh = fabsf(dragSelCurW.y - dragSelStartW.y);
            DrawRectangle((int)rx, (int)ry, (int)rw, (int)rh, {255, 160, 0, 12});
            DrawRectangleLinesEx({rx, ry, rw, rh}, 1.0f / cam.zoom, {255, 160, 0, 160});
        }

        EndMode2D();

        DrawRectangle(canvasW, 0, PANEL_W, GetScreenHeight(), PANEL_BG);
        DrawRectangle(0, canvasH, canvasW, FOOTER_H, BAR_COLOR);
        DrawLine(0, canvasH, canvasW, canvasH, BORDER_DIM);
        DrawLine(0, canvasH + FOOTER_H / 2, canvasW, canvasH + FOOTER_H / 2, BORDER_DIM);
        DrawRectangle(0, canvasH, 2, FOOTER_H / 2, ACCENT_BLUE);
        DrawRectangle(0, canvasH + FOOTER_H / 2, 2, FOOTER_H / 2, {30, 90, 44, 255});

        {
            const int row1Y = GetScreenHeight() - FOOTER_H + 10;

            for (int i = 0; i < N_BTNS1; i++)
                DrawToolbarButton(barStartX + i * (BTN_W + BTN_GAP), row1Y, BTN_W, BTN_H, TOOLBAR1[i].label, TOOLBAR1[i].accent);

            Rectangle playBtn = {(float)(canvasW - BTN_W - 14), (float)row1Y, (float)BTN_W, (float)BTN_H};
            bool playHov = CheckCollisionPointRec(GetMousePosition(), playBtn);
            DrawRectangle(canvasW - BTN_W - 14, row1Y, BTN_W, BTN_H, playHov ? Color{60, 60, 64, 255} : Color{40, 40, 44, 255});
            Color pbCol = simulator.IsPlaying() ? Color{80, 200, 80, 255} : Color{200, 200, 60, 255};
            const char* playTxt = simulator.IsPlaying() ? "PAUSE" : "PLAY";
            DrawText(playTxt, canvasW - BTN_W - 14 + (BTN_W - MeasureText(playTxt, 12)) / 2, row1Y + BTN_H - 20, 12, pbCol);
            DrawRectangleLinesEx(playBtn, 1, {70, 70, 75, 255});
            if (playHov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                simulator.TogglePlay();
        }

        char zt[32];
        sprintf(zt, "%.0f%%", (double)(cam.zoom * 100.0f));
        int zfs = 11, ztw = MeasureText(zt, zfs);
        DrawRectangle(canvasW - ztw - 18, 8, ztw + 12, 16, {8, 9, 10, 210});
        DrawRectangleLinesEx({(float)(canvasW - ztw - 18), 8.0f, (float)(ztw + 12), 16.0f}, 1, BORDER_DIM);
        DrawText(zt, canvasW - ztw - 12, 10, zfs, TEXT_DIM);

        int selCount = 0;
        for (bool s : selected) if (s) selCount++;
        if (selCount > 0) {
            char sb[64];
            sprintf(sb, "%d selected | DEL to delete", selCount);
            int sfs = 14, stw = MeasureText(sb, sfs);
            DrawRectangle(canvasW / 2 - stw / 2 - 10, canvasH - 28, stw + 20, 18, {10, 11, 13, 220});
            DrawRectangleLinesEx({(float)(canvasW / 2 - stw / 2 - 10), (float)(canvasH - 28), (float)(stw + 20), 18.0f}, 1, BORDER_LIT);
            DrawText(sb, canvasW / 2 - stw / 2, canvasH - 25, sfs, SIG_ON);
        }

        if (!simulator.IsPlaying()) {
            DrawRectangle(10, canvasH - 22, 72, 14, {20, 12, 10, 220});
            DrawRectangleLinesEx({10, (float)(canvasH - 22), 72, 14}, 1, {80, 28, 20, 255});
            DrawText("PAUSED", 14, canvasH - 20, 10, {160, 60, 40, 255});
        } else if (wiring)
            DrawText("WIRING | click target node | click canvas to cancel", 10, 10, 12, {140, 120, 50, 255});
        else if (panning)
            DrawText("PAN", 10, 10, 12, TEXT_DIM);
        else if (dragSelecting)
            DrawText("SELECTING | release to confirm | Shift=additive", 10, 10, 12, {160, 120, 30, 255});
        else
            DrawText("DRAG=select | SHIFT+click=add | DEL=delete | R=reset | SPACE=pan", 10, 10, 11, {48, 52, 58, 255});

        EndDrawing();
    }

    CloseWindow();
    return 0;
}