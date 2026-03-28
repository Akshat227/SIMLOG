#include "Node.h"
#include <cmath>

Node::Node(Vector2 pos, Vector2 sz, const std::string& lbl)
    : position(pos), size(sz), label(lbl), isSelected(false), isDragging(false) {}

void Node::UpdatePinPositions() {
    float inputY = position.y + 30;
    for (auto& pin : inputPins) {
        pin.position = {position.x, inputY};
        inputY += 25;
    }

    float outputY = position.y + 30;
    for (auto& pin : outputPins) {
        pin.position = {position.x + size.x, outputY};
        outputY += 25;
    }
}

void Node::Draw() const {
    DrawRectangleLines((int)position.x, (int)position.y, (int)size.x, (int)size.y,
                       isSelected ? SKYBLUE : DARKGRAY);
    DrawText(label.c_str(), (int)position.x + 10, (int)position.y + 5, 12, WHITE);

    for (const auto& pin : inputPins) {
        Color pinColor = pin.value ? GREEN : GRAY;
        DrawCircle((int)pin.position.x, (int)pin.position.y, 6, pinColor);
    }

    for (const auto& pin : outputPins) {
        Color pinColor = pin.value ? GREEN : GRAY;
        DrawCircle((int)pin.position.x, (int)pin.position.y, 6, pinColor);
    }
}

bool Node::ContainsPoint(Vector2 point) const {
    return point.x >= position.x && point.x <= position.x + size.x &&
           point.y >= position.y && point.y <= position.y + size.y;
}

Pin* Node::GetPinAtPoint(Vector2 point) {
    for (auto& pin : inputPins) {
        float dist = std::sqrt(std::pow(point.x - pin.position.x, 2) +
                               std::pow(point.y - pin.position.y, 2));
        if (dist < 10) return &pin;
    }
    for (auto& pin : outputPins) {
        float dist = std::sqrt(std::pow(point.x - pin.position.x, 2) +
                               std::pow(point.y - pin.position.y, 2));
        if (dist < 10) return &pin;
    }
    return nullptr;
}

Pin* Node::GetOutputPinAtPoint(Vector2 point) {
    for (auto& pin : outputPins) {
        float dist = std::sqrt(std::pow(point.x - pin.position.x, 2) +
                               std::pow(point.y - pin.position.y, 2));
        if (dist < 10) return &pin;
    }
    return nullptr;
}

Pin* Node::GetInputPinAtPoint(Vector2 point) {
    for (auto& pin : inputPins) {
        float dist = std::sqrt(std::pow(point.x - pin.position.x, 2) +
                               std::pow(point.y - pin.position.y, 2));
        if (dist < 10) return &pin;
    }
    return nullptr;
}

void Node::PropagateValue() {
    for (auto& pin : outputPins) {
        pin.value = ComputeOutput();
        if (pin.connectedTo) {
            pin.connectedTo->value = pin.value;
            if (pin.connectedTo->connectedTo) {
            }
        }
    }
}