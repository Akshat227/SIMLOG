#include "raylib.h"
#include "Gate.h"
#include "Simulator.h"
#include <vector>
#include <string>

struct ToolTrayItem {
    std::string label;
    Rectangle rect;
    Node* (*createFunc)(Vector2);
};

Rectangle toolTray = {10, 10, 100, 400};
Rectangle canvas = {120, 10, 670, 500};

Node* selectedNode = nullptr;
Pin* connectionStartPin = nullptr;
Vector2 mousePos = {0, 0};
Vector2 draggingOffset = {0, 0};

NOTGate* CreateNOT(Vector2 pos) { return new NOTGate(pos); }
ANDGate* CreateAND(Vector2 pos) { return new ANDGate(pos); }
ORGate* CreateOR(Vector2 pos) { return new ORGate(pos); }
NORGate* CreateNOR(Vector2 pos) { return new NORGate(pos); }
NANDGate* CreateNAND(Vector2 pos) { return new NANDGate(pos); }
INPUTNode* CreateINPUT(Vector2 pos) { return new INPUTNode(pos); }
OUTPUTNode* CreateOUTPUT(Vector2 pos) { return new OUTPUTNode(pos); }

ToolTrayItem toolItems[] = {
    {"NOT", {20, 30, 80, 35}, (Node* (*)(Vector2))CreateNOT},
    {"AND", {20, 75, 80, 35}, (Node* (*)(Vector2))CreateAND},
    {"OR", {20, 120, 80, 35}, (Node* (*)(Vector2))CreateOR},
    {"NOR", {20, 165, 80, 35}, (Node* (*)(Vector2))CreateNOR},
    {"NAND", {20, 210, 80, 35}, (Node* (*)(Vector2))CreateNAND},
    {"IN", {20, 255, 80, 35}, (Node* (*)(Vector2))CreateINPUT},
    {"OUT", {20, 300, 80, 35}, (Node* (*)(Vector2))CreateOUTPUT},
};

Simulator simulator;

Node* CreateNodeAtCenter(const std::string& label) {
    Vector2 centerPos = {canvas.x + canvas.width / 2 - 30, canvas.y + canvas.height / 2 - 30};
    if (label == "NOT") return new NOTGate(centerPos);
    if (label == "AND") return new ANDGate(centerPos);
    if (label == "OR") return new ORGate(centerPos);
    if (label == "NOR") return new NORGate(centerPos);
    if (label == "NAND") return new NANDGate(centerPos);
    if (label == "IN") return new INPUTNode(centerPos);
    if (label == "OUT") return new OUTPUTNode(centerPos);
    return nullptr;
}

void DrawToolTray() {
    DrawRectangleRec(toolTray, DARKGRAY);
    DrawText("TOOLS", (int)toolTray.x + 20, (int)toolTray.y + 5, 14, WHITE);

    for (auto& item : toolItems) {
        DrawRectangleRec(item.rect, LIGHTGRAY);
        DrawRectangleLines((int)item.rect.x, (int)item.rect.y, (int)item.rect.width, (int)item.rect.height, DARKGRAY);
        DrawText(item.label.c_str(), (int)item.rect.x + 15, (int)item.rect.y + 10, 14, BLACK);
    }
}

void DrawCanvas() {
    DrawRectangleRec(canvas, BLACK);
    DrawRectangleLines((int)canvas.x, (int)canvas.y, (int)canvas.width, (int)canvas.height, DARKGRAY);
}

void DrawConnectionPreview() {
    if (connectionStartPin) {
        DrawLineEx(connectionStartPin->position, mousePos, 3, YELLOW);
    }
}

Node* GetNodeAtPoint(Vector2 point) {
    for (auto it = simulator.GetNodes().rbegin(); it != simulator.GetNodes().rend(); ++it) {
        if ((*it)->ContainsPoint(point)) {
            return *it;
        }
    }
    return nullptr;
}

int main() {
    InitWindow(800, 520, "Logic Gate Simulator");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        mousePos = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mousePos, toolTray)) {
                for (auto& item : toolItems) {
                    if (CheckCollisionPointRec(mousePos, item.rect)) {
                        Node* newNode = item.createFunc({canvas.x + canvas.width / 2 - 30, canvas.y + canvas.height / 2 - 30});
                        simulator.AddNode(newNode);
                        break;
                    }
                }
            } else if (CheckCollisionPointRec(mousePos, canvas)) {
                Node* clickedNode = GetNodeAtPoint(mousePos);

                if (clickedNode) {
                    INPUTNode* inputNode = dynamic_cast<INPUTNode*>(clickedNode);
                    if (inputNode) {
                        inputNode->ToggleValue();
                        simulator.Propagate();
                    } else {
                        Pin* pin = clickedNode->GetOutputPinAtPoint(mousePos);
                        if (pin) {
                            connectionStartPin = pin;
                        } else {
                            selectedNode = clickedNode;
                            selectedNode->SetSelected(true);
                            selectedNode->SetDragging(true);
                            draggingOffset = {mousePos.x - clickedNode->GetPosition().x,
                                             mousePos.y - clickedNode->GetPosition().y};
                        }
                    }
                }
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            if (selectedNode && selectedNode->GetDragging()) {
                selectedNode->SetDragging(false);
                selectedNode = nullptr;
            }

            if (connectionStartPin) {
                Node* endNode = GetNodeAtPoint(mousePos);
                if (endNode) {
                    Pin* endPin = endNode->GetInputPinAtPoint(mousePos);
                    if (endPin && connectionStartPin != endPin) {
                        Connection* conn = new Connection(connectionStartPin, endPin);
                        simulator.AddConnection(conn);
                    }
                }
                connectionStartPin = nullptr;
            }
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            Node* node = GetNodeAtPoint(mousePos);
            if (node) {
                simulator.RemoveNode(node);
            }
        }

        if (selectedNode && selectedNode->GetDragging()) {
            Vector2 newPos = {mousePos.x - draggingOffset.x, mousePos.y - draggingOffset.y};

            float nodeWidth = selectedNode->GetSize().x;
            float nodeHeight = selectedNode->GetSize().y;

            if (newPos.x < canvas.x) newPos.x = canvas.x;
            if (newPos.y < canvas.y) newPos.y = canvas.y;
            if (newPos.x + nodeWidth > canvas.x + canvas.width) newPos.x = canvas.x + canvas.width - nodeWidth;
            if (newPos.y + nodeHeight > canvas.y + canvas.height) newPos.y = canvas.y + canvas.height - nodeHeight;

            selectedNode->SetPosition(newPos);
            selectedNode->UpdatePinPositions();
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawToolTray();
        DrawCanvas();
        simulator.DrawAll();
        DrawConnectionPreview();

        DrawText("Left Click: Add/Toggle IN | Right Click: Delete", 10, 490, 10, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}