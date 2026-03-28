#ifndef NODE_H
#define NODE_H

#include "raylib.h"
#include <vector>
#include <string>

struct Pin {
    Vector2 position;
    bool isOutput;
    int value;
    Pin* connectedTo;
    Pin(Vector2 pos, bool output) : position(pos), isOutput(output), value(0), connectedTo(nullptr) {}
};

class Node {
protected:
    Vector2 position;
    Vector2 size;
    std::string label;
    std::vector<Pin> inputPins;
    std::vector<Pin> outputPins;
    bool isSelected;
    bool isDragging;

public:
    Node(Vector2 pos, Vector2 sz, const std::string& lbl);
    virtual ~Node() = default;

    Vector2 GetPosition() const { return position; }
    Vector2 GetSize() const { return size; }
    std::string GetLabel() const { return label; }
    bool GetSelected() const { return isSelected; }
    bool GetDragging() const { return isDragging; }

    void SetPosition(Vector2 pos) { position = pos; }
    void SetSelected(bool sel) { isSelected = sel; }
    void SetDragging(bool drag) { isDragging = drag; }

    virtual void UpdatePinPositions();
    virtual void Draw() const;
    virtual bool ContainsPoint(Vector2 point) const;
    Pin* GetPinAtPoint(Vector2 point);
    Pin* GetOutputPinAtPoint(Vector2 point);
    Pin* GetInputPinAtPoint(Vector2 point);

    const std::vector<Pin>& GetInputPins() const { return inputPins; }
    const std::vector<Pin>& GetOutputPins() const { return outputPins; }
    std::vector<Pin>& GetInputPins() { return inputPins; }
    std::vector<Pin>& GetOutputPins() { return outputPins; }

    virtual bool ComputeOutput() = 0;
    void PropagateValue();
};

#endif