#include "Simulator.h"
#include "Gate.h"
#include <algorithm>

Simulator::~Simulator() {
    for (auto node : nodes) delete node;
    for (auto conn : connections) delete conn;
}

void Simulator::AddNode(Node* node) {
    nodes.push_back(node);
}

void Simulator::RemoveNode(Node* node) {
    auto it = std::find(nodes.begin(), nodes.end(), node);
    if (it != nodes.end()) {
        nodes.erase(it);
        delete node;
    }
}

void Simulator::AddConnection(Connection* conn) {
    connections.push_back(conn);
    conn->endPin->connectedTo = conn->startPin;
}

void Simulator::RemoveConnection(Connection* conn) {
    auto it = std::find(connections.begin(), connections.end(), conn);
    if (it != connections.end()) {
        connections.erase(it);
        delete conn;
    }
}

void Simulator::Propagate() {
    for (auto node : nodes) {
        for (auto& pin : node->GetInputPins()) {
            pin.value = 0;
        }
        for (auto& pin : node->GetOutputPins()) {
            pin.value = 0;
        }
    }

    for (auto node : nodes) {
        INPUTNode* inputNode = dynamic_cast<INPUTNode*>(node);
        if (inputNode) {
            node->GetOutputPins()[0].value = inputNode->GetValue() ? 1 : 0;
        }
    }

    for (size_t pass = 0; pass < nodes.size() + 1; ++pass) {
        for (auto& conn : connections) {
            conn->endPin->value = conn->startPin->value;
        }

        for (auto node : nodes) {
            if (!dynamic_cast<INPUTNode*>(node)) {
                bool output = node->ComputeOutput();
                for (auto& pin : node->GetOutputPins()) {
                    pin.value = output ? 1 : 0;
                }
            }
        }
    }
}

void Simulator::DrawAll() const {
    for (const auto& conn : connections) {
        conn->Draw();
    }
    for (const auto& node : nodes) {
        node->Draw();
    }
}