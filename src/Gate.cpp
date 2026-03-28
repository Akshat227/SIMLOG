#include "Gate.h"
#include <cmath>

Gate::Gate(Vector2 pos, Vector2 sz, const std::string& lbl, int numInputs)
    : Node(pos, sz, lbl) {
    for (int i = 0; i < numInputs; i++) {
        inputPins.push_back(Pin({0, 0}, false));
    }
    outputPins.push_back(Pin({0, 0}, true));
    UpdatePinPositions();
}

bool Gate::ComputeOutput() {
    return false;
}

NOTGate::NOTGate(Vector2 pos) : Gate(pos, {60, 60}, "NOT", 1) {}

bool NOTGate::ComputeOutput() {
    if (inputPins.empty()) return false;
    return !inputPins[0].value;
}

ANDGate::ANDGate(Vector2 pos) : Gate(pos, {60, 70}, "AND", 2) {}

bool ANDGate::ComputeOutput() {
    if (inputPins.size() < 2) return false;
    return inputPins[0].value && inputPins[1].value;
}

ORGate::ORGate(Vector2 pos) : Gate(pos, {60, 70}, "OR", 2) {}

bool ORGate::ComputeOutput() {
    if (inputPins.size() < 2) return false;
    return inputPins[0].value || inputPins[1].value;
}

NORGate::NORGate(Vector2 pos) : Gate(pos, {60, 70}, "NOR", 2) {}

bool NORGate::ComputeOutput() {
    if (inputPins.size() < 2) return false;
    return !(inputPins[0].value || inputPins[1].value);
}

NANDGate::NANDGate(Vector2 pos) : Gate(pos, {60, 70}, "NAND", 2) {}

bool NANDGate::ComputeOutput() {
    if (inputPins.size() < 2) return false;
    return !(inputPins[0].value && inputPins[1].value);
}

INPUTNode::INPUTNode(Vector2 pos)
    : Node(pos, {60, 40}, "IN"), inputValue(false) {
    outputPins.push_back(Pin({0, 0}, true));
    UpdatePinPositions();
}

bool INPUTNode::ComputeOutput() {
    return inputValue;
}

void INPUTNode::Draw() const {
    DrawRectangle((int)position.x, (int)position.y, (int)size.x, (int)size.y,
                  inputValue ? GREEN : DARKGRAY);
    DrawText("IN", (int)position.x + 20, (int)position.y + 12, 14, WHITE);

    for (const auto& pin : outputPins) {
        Color pinColor = inputValue ? GREEN : GRAY;
        DrawCircle((int)pin.position.x, (int)pin.position.y, 6, pinColor);
    }
}

void INPUTNode::ToggleValue() {
    inputValue = !inputValue;
}

OUTPUTNode::OUTPUTNode(Vector2 pos)
    : Node(pos, {60, 40}, "OUT"), outputValue(false) {
    inputPins.push_back(Pin({0, 0}, false));
    UpdatePinPositions();
}

bool OUTPUTNode::ComputeOutput() {
    if (inputPins.empty()) return false;
    outputValue = inputPins[0].value;
    return outputValue;
}

void OUTPUTNode::Draw() const {
    DrawRectangle((int)position.x, (int)position.y, (int)size.x, (int)size.y,
                  outputValue ? GREEN : DARKGRAY);
    DrawText("OUT", (int)position.x + 15, (int)position.y + 12, 14, WHITE);

    for (const auto& pin : inputPins) {
        Color pinColor = outputValue ? GREEN : GRAY;
        DrawCircle((int)pin.position.x, (int)pin.position.y, 6, pinColor);
    }
}