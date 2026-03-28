#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <vector>
#include "Gate.h"

class Simulator {
private:
    std::vector<Gate> gates;
    std::vector<Wire> wires;
    bool simPlaying;
    double simAccum;

public:
    Simulator();
    ~Simulator();

    std::vector<Gate>& GetGates() { return gates; }
    std::vector<Wire>& GetWires() { return wires; }

    void AddGate(const Gate& gate);
    void RemoveGate(int idx);
    void AddWire(const Wire& wire);
    void RemoveWire(int idx);
    void DeleteGate(int idx);

    void Propagate();
    void TogglePlay() { simPlaying = !simPlaying; }
    bool IsPlaying() const { return simPlaying; }

    void DrawAll(bool panning, const Camera2D& cam);
};

#endif