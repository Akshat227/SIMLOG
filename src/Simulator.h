#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <vector>
#include "Node.h"
#include "Connection.h"
#include "Gate.h"

class Simulator {
private:
    std::vector<Node*> nodes;
    std::vector<Connection*> connections;

public:
    ~Simulator();

    void AddNode(Node* node);
    void RemoveNode(Node* node);
    void AddConnection(Connection* conn);
    void RemoveConnection(Connection* conn);

    std::vector<Node*>& GetNodes() { return nodes; }
    std::vector<Connection*>& GetConnections() { return connections; }

    void Simulate();
    void Propagate();
    void DrawAll() const;
};

#endif