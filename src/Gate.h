#ifndef GATE_H
#define GATE_H

#include "raylib.h"
#include "Node.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

constexpr int NODE_SZ = 12;
constexpr int NODE_HIT = 14;
constexpr int MAX_NAME = 31;

constexpr Color BG_COLOR = {13, 14, 16, 255};
constexpr Color BAR_COLOR = {8, 9, 10, 255};
constexpr Color BTN_COLOR = {24, 26, 30, 255};
constexpr Color BTN_HOVER = {36, 39, 44, 255};
constexpr Color BTN_OUTLINE = {46, 50, 56, 255};
constexpr Color SEL_COLOR = {230, 140, 0, 255};
constexpr Color SIG_ON = {230, 140, 0, 255};
constexpr Color SIG_OFF = {30, 32, 36, 255};
constexpr Color ACCENT_BLUE = {30, 100, 180, 255};
constexpr Color BORDER_DIM = {32, 35, 40, 255};
constexpr Color BORDER_LIT = {55, 60, 68, 255};
constexpr Color TEXT_PRI = {195, 200, 208, 255};
constexpr Color TEXT_DIM = {70, 76, 86, 255};
constexpr Color DANGER = {180, 40, 30, 255};

enum class GateType { NOT, AND, OR, INPUT, OUTPUT, CUSTOM, CLOCK, MUX, DMUX, ENCODER, DECODER, SEG7 };

struct ToolBtn { const char* label; GateType type; Color accent; };

const ToolBtn TOOLBAR1[] = {
    { "NOT",    GateType::NOT,    { 45, 90, 140, 255 } },
    { "AND",    GateType::AND,    { 40, 110, 55, 255 } },
    { "OR",     GateType::OR,     { 130, 70, 28, 255 } },
    { "INPUT",  GateType::INPUT,  { 55, 55, 130, 255 } },
    { "OUTPUT", GateType::OUTPUT, { 130, 40, 36, 255 } },
};
constexpr int N_BTNS1 = 5;

struct Gate {
    GateType type;
    Rectangle rect;
    bool dragging = false;
    Vector2 dragOffset = {};
    bool checked = false;
    bool outputSig = false;
    bool renaming = false;
    char name[MAX_NAME + 1] = {};
    std::vector<Node> nodes;

    std::string circuitLabel;
    std::vector<Gate> intGates;
    std::vector<Wire> intWires;
    std::vector<int> inPins;
    std::vector<int> outPins;
    bool expanded = false;
    float collapsedW = 140.f, collapsedH = 70.f;
    float clockHz = 1.f;
    double clockLastToggle = 0.0;
    bool segs[7] = {};

    inline Color bodyColor() const {
        switch (type) {
            case GateType::NOT:    return { 28, 52, 82, 255};
            case GateType::AND:    return { 22, 54, 34, 255};
            case GateType::OR:     return { 68, 36, 16, 255};
            case GateType::INPUT:  return { 26, 28, 60, 255};
            case GateType::OUTPUT: return outputSig ? Color{20, 55, 22, 255} : Color{55, 16, 16, 255};
            case GateType::CUSTOM: return { 28, 18, 48, 255};
            case GateType::CLOCK:  return { 52, 40, 8, 255};
            case GateType::MUX:    return { 18, 42, 62, 255};
            case GateType::DMUX:   return { 14, 46, 40, 255};
            case GateType::ENCODER: return { 54, 28, 10, 255};
            case GateType::DECODER: return { 38, 14, 54, 255};
            case GateType::SEG7:   return { 10, 10, 12, 255};
        }
        return {22, 24, 28, 255};
    }

    inline const char* defaultLabel() const {
        switch (type) {
            case GateType::NOT:    return "NOT";
            case GateType::AND:    return "AND";
            case GateType::OR:     return "OR";
            case GateType::INPUT:  return "INPUT";
            case GateType::OUTPUT: return "OUTPUT";
            case GateType::CUSTOM: return circuitLabel.c_str();
            case GateType::CLOCK:  return "CLOCK";
            case GateType::MUX:    return "MUX";
            case GateType::DMUX:   return "DMUX";
            case GateType::ENCODER: return "ENCODE";
            case GateType::DECODER: return "DECODE";
            case GateType::SEG7:   return "7-SEG";
        }
        return "";
    }

    inline const char* displayName() const { return name[0] ? name : defaultLabel(); }

    inline Rectangle checkboxRect() const {
        return {rect.x + rect.width / 2 - 7, rect.y - 18, 14, 14};
    }

    inline Rectangle deleteRect() const {
        return {rect.x + rect.width - 16, rect.y - 14, 14, 14};
    }

    inline Rectangle toggleRect() const {
        return {rect.x + rect.width - 34, rect.y - 14, 14, 14};
    }

    inline void rebuildNodes() {
        if (type == GateType::CUSTOM) {
            rebuildCustomNodes();
            return;
        }

        std::vector<bool> sigs(nodes.size(), false);
        for (int i = 0; i < (int)nodes.size(); i++) sigs[i] = nodes[i].signal;
        nodes.clear();

        float x = rect.x, y = rect.y, w = rect.width, h = rect.height;

        switch (type) {
            case GateType::INPUT:
                nodes.push_back({{x + w, y + h / 2}, true});
                break;
            case GateType::OUTPUT:
                nodes.push_back({{x, y + h / 2}, false});
                break;
            case GateType::NOT:
                nodes.push_back({{x, y + h / 2}, false});
                nodes.push_back({{x + w, y + h / 2}, true});
                break;
            case GateType::AND:
            case GateType::OR:
                nodes.push_back({{x, y + h / 3}, false});
                nodes.push_back({{x, y + h * 2 / 3}, false});
                nodes.push_back({{x + w, y + h / 2}, true});
                break;
            case GateType::CLOCK:
                nodes.push_back({{x + w, y + h / 2}, true});
                break;
            case GateType::MUX:
                nodes.push_back({{x, y + h / 4}, false});
                nodes.push_back({{x, y + h / 2}, false});
                nodes.push_back({{x, y + h * 3 / 4}, false});
                nodes.push_back({{x + w, y + h / 2}, true});
                break;
            case GateType::DMUX:
                nodes.push_back({{x, y + h / 3}, false});
                nodes.push_back({{x, y + h * 2 / 3}, false});
                nodes.push_back({{x + w, y + h / 3}, true});
                nodes.push_back({{x + w, y + h * 2 / 3}, true});
                break;
            case GateType::ENCODER:
                nodes.push_back({{x, y + h / 5}, false});
                nodes.push_back({{x, y + h * 2 / 5}, false});
                nodes.push_back({{x, y + h * 3 / 5}, false});
                nodes.push_back({{x, y + h * 4 / 5}, false});
                nodes.push_back({{x + w, y + h / 3}, true});
                nodes.push_back({{x + w, y + h * 2 / 3}, true});
                break;
            case GateType::DECODER:
                nodes.push_back({{x, y + h / 3}, false});
                nodes.push_back({{x, y + h * 2 / 3}, false});
                nodes.push_back({{x + w, y + h / 5}, true});
                nodes.push_back({{x + w, y + h * 2 / 5}, true});
                nodes.push_back({{x + w, y + h * 3 / 5}, true});
                nodes.push_back({{x + w, y + h * 4 / 5}, true});
                break;
            case GateType::SEG7:
                for (int k = 0; k < 7; k++)
                    nodes.push_back({{x, y + (k + 1) * h / 8.0f}, false});
                break;
            default:
                break;
        }

        for (int i = 0; i < (int)nodes.size() && i < (int)sigs.size(); i++)
            nodes[i].signal = sigs[i];
    }

    inline void rebuildCustomNodes() {
        std::vector<bool> sigs(nodes.size(), false);
        for (int i = 0; i < (int)nodes.size(); i++) sigs[i] = nodes[i].signal;
        nodes.clear();

        int nIn = (int)inPins.size(), nOut = (int)outPins.size();

        if (expanded && !intGates.empty()) {
            float bx1 = 1e9f, by1 = 1e9f, bx2 = -1e9f, by2 = -1e9f;
            for (const auto& ig : intGates) {
                bx1 = std::min(bx1, ig.rect.x);
                by1 = std::min(by1, ig.rect.y);
                bx2 = std::max(bx2, ig.rect.x + ig.rect.width);
                by2 = std::max(by2, ig.rect.y + ig.rect.height);
            }

            float padX = 56.0f, padY = 30.0f;
            float ew = bx2 - bx1 + padX * 2, eh = by2 - by1 + padY * 2;
            float cxw = rect.x + collapsedW / 2.0f, cyw = rect.y + collapsedH / 2.0f;
            rect.x = cxw - ew / 2.0f;
            rect.y = cyw - eh / 2.0f;
            rect.width = ew;
            rect.height = eh;

            float wcy = rect.y + padY - by1;

            for (int k = 0; k < nIn; k++) {
                const Gate& ig = intGates[inPins[k]];
                float ny = wcy + ig.rect.y + ig.rect.height / 2.0f;
                bool sig = ((int)sigs.size() > k) ? sigs[k] : false;
                nodes.push_back({{rect.x, ny}, false, sig});
            }
            for (int k = 0; k < nOut; k++) {
                const Gate& ig = intGates[outPins[k]];
                float ny = wcy + ig.rect.y + ig.rect.height / 2.0f;
                bool sig = ((int)sigs.size() > nIn + k) ? sigs[nIn + k] : false;
                nodes.push_back({{rect.x + rect.width, ny}, true, sig});
            }
        } else {
            rect.width = collapsedW;
            int maxP = std::max(std::max(nIn, nOut), 1);
            rect.height = std::max(70.0f, (float)maxP * 34.0f + 20.0f);
            collapsedH = rect.height;
            float x2 = rect.x, y2 = rect.y, w2 = rect.width, h2 = rect.height;

            for (int k = 0; k < nIn; k++) {
                float fy = y2 + (k + 1) * h2 / (nIn + 1);
                bool sig = ((int)sigs.size() > k) ? sigs[k] : false;
                nodes.push_back({{x2, fy}, false, sig});
            }
            for (int k = 0; k < nOut; k++) {
                float fy = y2 + (k + 1) * h2 / (nOut + 1);
                bool sig = ((int)sigs.size() > nIn + k) ? sigs[nIn + k] : false;
                nodes.push_back({{x2 + w2, fy}, true, sig});
            }
        }
    }
};

#endif