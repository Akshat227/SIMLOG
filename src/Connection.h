#ifndef CONNECTION_H
#define CONNECTION_H

#include "raylib.h"
#include "Node.h"

class Connection {
public:
    Pin* startPin;
    Pin* endPin;

    Connection(Pin* start, Pin* end);
    void Draw() const;
};

#endif