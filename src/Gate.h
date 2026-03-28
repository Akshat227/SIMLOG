#ifndef GATE_H
#define GATE_H

#include "Node.h"

class Gate : public Node {
public:
    Gate(Vector2 pos, Vector2 sz, const std::string& lbl, int numInputs);
    virtual bool ComputeOutput() override;
};

class NOTGate : public Gate {
public:
    NOTGate(Vector2 pos);
    virtual bool ComputeOutput() override;
};

class ANDGate : public Gate {
public:
    ANDGate(Vector2 pos);
    virtual bool ComputeOutput() override;
};

class ORGate : public Gate {
public:
    ORGate(Vector2 pos);
    virtual bool ComputeOutput() override;
};

class NORGate : public Gate {
public:
    NORGate(Vector2 pos);
    virtual bool ComputeOutput() override;
};

class NANDGate : public Gate {
public:
    NANDGate(Vector2 pos);
    virtual bool ComputeOutput() override;
};

class INPUTNode : public Node {
private:
    bool inputValue;
public:
    INPUTNode(Vector2 pos);
    void ToggleValue();
    void SetValue(bool val) { inputValue = val; }
    bool GetValue() const { return inputValue; }
    virtual bool ComputeOutput() override;
    void Draw() const override;
};

class OUTPUTNode : public Node {
private:
    bool outputValue;
public:
    OUTPUTNode(Vector2 pos);
    bool GetValue() const { return outputValue; }
    virtual bool ComputeOutput() override;
    void Draw() const override;
};

#endif