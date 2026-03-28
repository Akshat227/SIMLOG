#ifndef NODE_H
#define NODE_H

#include "raylib.h"

struct Node {
    Vector2 pos;
    bool isOutput;
    bool signal;
    Node(Vector2 p, bool out, bool sig = false) : pos(p), isOutput(out), signal(sig) {}
};

struct Wire { int gateA, nodeA, gateB, nodeB; };

#endif