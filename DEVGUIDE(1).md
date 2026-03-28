# GateSim Developer Cheat Sheet

This document explains the complete source architecture of `gatesim.cpp` and gives step-by-step instructions for adding new gate types, new toolbar buttons, new wire behaviours, and new analysis tabs.

---

## Source structure overview

The entire simulator is a single file. Reading top to bottom:

    1.   Includes and namespace alias
    2.   Global constants (colours, layout dimensions, zoom limits)
    3.   CIRCUITS_DIR global (set in main() at startup)
    4.   GateType enum
    5.   ToolBtn array (toolbar definition)
    6.   Node struct
    7.   Wire struct
    8.   Gate struct  (the core data type)
    9.   SavedCircuit struct
    10.  saveCircuitToFile()
    11.  loadCircuitFromFile()
    12.  loadAllCircuits()
    13.  placeCircuit()        (expand saved circuit directly onto canvas)
    14.  placeAsNode()         (place saved circuit as a single CUSTOM node)
    15.  deleteCircuitFile()
    16.  buildSavePayload()
    17.  screenToWorld()
    18.  DrawBezier()
    19.  v2dist(), pSegDist(), bezierDist(), nodeDist()   (geometry helpers)
    20.  runPropagate()         (headless simulation, used by analysis tabs)
    21.  buildAnalysis()        (pre-computes full truth table)
    22.  DrawAnalysisModal()
    23.  DrawNodeCircuitView()
    24.  propagate()            (live per-frame simulation)
    25.  DrawGate()
    26.  DrawToolbarButton()
    27.  DrawSavedPanel()
    28.  DrawSaveModal()
    29.  deleteGate()
    30.  main()

---

## Data types

### Node

    struct Node {
        Vector2 pos;        // world-space position (absolute, not relative to gate)
        bool isOutput;      // true = output pin, false = input pin
        bool signal;        // current logic level
    };

Nodes are rebuilt from scratch every time the gate moves (rebuildNodes). Their world positions are re-derived from rect each rebuild, so you never store them separately.

### Wire

    struct Wire {
        int gateA, nodeA;   // source gate index and node index (must be isOutput=true)
        int gateB, nodeB;   // destination gate index and node index (must be isOutput=false)
    };

Wire indices are into the flat `std::vector<Gate> gates` in main(). When a gate is deleted, all wire indices above the deleted index are decremented. This is done in deleteGate().

### Gate

    GateType  type;
    Rectangle rect;         // world-space bounding box (x, y, width, height)
    bool      dragging;
    Vector2   dragOffset;   // mouse offset at drag start, in world space
    bool      checked;      // INPUT gate toggle state
    bool      outputSig;    // current output logic level
    bool      renaming;     // true while name-edit mode is active
    char      name[32];     // custom name (empty = use defaultLabel())
    std::vector<Node> nodes;

    // CUSTOM only:
    std::string        circuitLabel;  // the saved circuit's name
    std::vector<Gate>  intGates;      // full copy of internal gates
    std::vector<Wire>  intWires;      // full copy of internal wires
    std::vector<int>   inPins;        // indices into intGates that are INPUT type
    std::vector<int>   outPins;       // indices into intGates that are OUTPUT type
    bool               expanded;      // toggle expanded/collapsed view
    float              collapsedW, collapsedH;  // saved compact size

### SavedCircuit

    std::string name;
    struct GateData { GateType type; float relX,relY,w,h; char name[32]; bool checked; };
    struct WireData { int gA,nA,gB,nB; };
    std::vector<GateData> gates;
    std::vector<WireData> wires;

Coordinates in GateData are relative to the centroid of all gates. Wire indices reference the GateData list within the same SavedCircuit.

---

## The propagation loop

propagate() is called once per frame. It runs N+1 passes where N is the gate count. This guarantees that any chain of gates, no matter how deep, reaches a stable state within a single frame.

    for each gate: clear all node signals to false
    for each INPUT gate: set nodes[0].signal = gate.checked
    repeat N+1 times:
        for each wire: gateB.nodes[nodeB].signal = gateA.nodes[nodeA].signal
        for each gate:
            NOT:    outputSig = !nodes[0].signal;  nodes[1].signal = outputSig
            AND:    outputSig = nodes[0] && nodes[1];  nodes[2].signal = outputSig
            OR:     outputSig = nodes[0] || nodes[1];  nodes[2].signal = outputSig
            OUTPUT: outputSig = nodes[0].signal
            CUSTOM: run internal propagation loop on intGates/intWires

For CUSTOM gates the same structure repeats recursively on intGates and intWires using the input pin signals fed from the outer nodes.

runPropagate() is a stateless copy of this logic that takes gates and wires by value. It is used by the analysis modal to enumerate truth table rows without disturbing the live canvas state.

---

## How rebuildNodes works

rebuildNodes() is called after every position change (drag, initial placement). For primitive gates it recomputes node world positions from rect:

    INPUT:  one output node at (rect.x+w, rect.y+h/2)
    OUTPUT: one input node at  (rect.x,   rect.y+h/2)
    NOT:    input at left center, output at right center
    AND/OR: two inputs at left (h/3 and 2h/3), output at right center

For CUSTOM gates rebuildNodes() delegates to rebuildCustomNodes() which has two branches:

Collapsed branch: behaves like a primitive gate. Heights are set to max(nIn, nOut)*34+20. Input nodes evenly spaced on the left edge, output nodes on the right.

Expanded branch: computes the bounding box of all intGates in circuit-local space, expands rect to fit with padX=56, padY=30. Translates each input pin's world y to the midpoint of its corresponding intGates[inPins[k]].rect. Places external nodes on the rect boundary (left edge for inputs, right edge for outputs).

The world-space origin formula used in both rebuildCustomNodes and DrawGate must match exactly:

    wcx = rect.x + padX - bx1
    wcy = rect.y + padY - by1

where bx1 and by1 are the minimum x and y of all intGate rects.

---

## How to add a new primitive gate type

This requires changes in six places.

Step 1. Add to the GateType enum:

    enum class GateType { NOT, AND, OR, INPUT, OUTPUT, CUSTOM, NAND };

Step 2. Add a toolbar entry in the TOOLBAR array and increment N_BTNS:

    constexpr int N_BTNS = 6;

    const ToolBtn TOOLBAR[N_BTNS] = {
        ...
        { "NAND", GateType::NAND, { 180, 60, 160, 255 } },
    };

Step 3. Add bodyColor():

    case GateType::NAND: return { 180, 60, 160, 255 };

Step 4. Add defaultLabel():

    case GateType::NAND: return "NAND";

Step 5. Add node layout in rebuildNodes(). The node index convention is: input nodes first (isOutput=false), then output nodes (isOutput=true). The output node index must be used consistently in propagate().

    case GateType::NAND:
        nodes.push_back({{x, y+h/3},  false});   // node 0: input A
        nodes.push_back({{x, y+h*2/3},false});   // node 1: input B
        nodes.push_back({{x+w, y+h/2},true});    // node 2: output
        break;

Step 6. Add propagation logic in propagate():

    case GateType::NAND:
        if(g.nodes.size()>=3){
            g.outputSig = !(g.nodes[0].signal && g.nodes[1].signal);
            g.nodes[2].signal = g.outputSig;
        }
        break;

DrawGate() will render the gate with the correct body color and label automatically. No drawing changes needed for a standard rectangular gate.

---

## How to add a gate with custom drawing

Add the rendering inside DrawGate() after the generic node loop, wrapped in a type check:

    if(g.type==GateType::NAND)
    {
        // draw a small circle at the output to indicate inversion
        Vector2 outPos = g.nodes[2].pos;
        DrawCircleV(outPos, 5, g.nodes[2].signal ? YELLOW : LIGHTGRAY);
        DrawRectangleLinesEx({outPos.x-5, outPos.y-5, 10, 10}, 1, {0,0,0,180});
    }

Draw calls inside DrawGate execute inside BeginMode2D, so coordinates are in world space.

---

## How to add a gate that does not use the standard two-input layout

The only rule the wiring system enforces is that the source node must have isOutput=true and the destination node must have isOutput=false. You can have any number of input or output nodes. Example: a three-input AND gate.

    case GateType::AND3:
        nodes.push_back({{x, y+h/4},  false});   // node 0
        nodes.push_back({{x, y+h/2},  false});   // node 1
        nodes.push_back({{x, y+h*3/4},false});   // node 2
        nodes.push_back({{x+w, y+h/2},true});    // node 3: output
        break;

Propagation:

    case GateType::AND3:
        if(g.nodes.size()>=4){
            g.outputSig = g.nodes[0].signal && g.nodes[1].signal && g.nodes[2].signal;
            g.nodes[3].signal = g.outputSig;
        }
        break;

---

## How to add a new analysis tab

The analysis modal is DrawAnalysisModal(). All tab rendering is done inside a sequence of if/else blocks on the `tab` integer.

Step 1. Add the tab label to the tabs array:

    const char* tabs[]={"Truth Table","K-Map","Live I/O","Timing"};

Step 2. Update the loop count where tabs are drawn (it currently loops 0 to 2).

Step 3. Add an `else if(tab==3)` block below the Live I/O block. Inside it, you have access to:

    gates      - the live gate list
    wires      - the live wire list
    inIdx      - indices of all INPUT gates
    outIdx     - indices of all OUTPUT gates
    inNames    - names of each input
    outNames   - names of each output
    nIn, nOut  - counts

To simulate any input combination without touching live state, call:

    std::vector<bool> inV = { true, false, true };  // example
    std::vector<bool> outV;
    runPropagate(gates, wires, inIdx, inV, outIdx, outV);

---

## How the .lgc file format works

GateSim stores saved circuits as plain-text pipe-delimited files. The parser is in loadCircuitFromFile(). Each field is split on `|` using std::getline into a parts vector.

The gate type integer maps directly to the GateType enum cast. When you add a new GateType, its enum value is automatically the correct integer to write. No format changes are needed.

Gate names are the optional 8th field. If a gate has no custom name, the 7th field (checked) is followed by `|` and then nothing. The parser handles this: `if(parts.size()>=8) strncpy(gd.name, ...)`.

Wire nodeA must be the output node index of gateA and nodeB must be the input node index of gateB. The node index is the position in the nodes vector as built by rebuildNodes(). For the standard layout: NOT output=1, AND/OR output=2, INPUT output=0.

To write a circuit file by hand or generate one programmatically, follow this template:

    CIRCUIT|MyName
    GATECOUNT|3
    GATE|3|−55|0|110|70|0|A
    GATE|3|55|0|110|70|0|B
    GATE|1|165|0|110|70|0|
    WIRECOUNT|2
    WIRE|0|0|2|0
    WIRE|1|0|2|1

This creates two INPUT gates (type 3) feeding an AND gate (type 1). Gate 0 node 0 (INPUT output pin) connects to gate 2 node 0 (AND input A). Gate 1 node 0 connects to gate 2 node 1 (AND input B).

---

## Coordinate systems

There are two coordinate systems in use simultaneously.

Screen space: pixel coordinates with (0,0) at the top-left of the window. Used for UI elements drawn outside BeginMode2D (footer toolbar, saved panel, modals, HUD text).

World space: the infinite canvas coordinate system. All gates, nodes, and wires use world space. Conversions:

    screenToWorld(screenPos, cam)  ->  GetScreenToWorld2D(screenPos, cam)
    world pos drawn inside BeginMode2D / EndMode2D automatically

The Camera2D has:

    cam.offset   = (canvasW/2, canvasH/2)    re-set every frame so window resize works
    cam.target   = the world-space point shown at the offset position
    cam.zoom     = current zoom level (1.0 = 100%)

Node positions in Gate.nodes are in world space. Node positions in CUSTOM intGates are in circuit-local space (they were built relative to the original gate layout and are never moved). When rendering the expanded view they are translated:

    world_x = wcx + ig.nodes[k].pos.x    where wcx = rect.x + padX - bx1

---

## The selection and drag-select system

selected is a std::vector<bool> that runs in lockstep with gates. selected[i] is true when gate i is part of the current selection. deleteGate() keeps the two vectors synchronized.

Drag-select uses three state variables:

    bool    dragSelecting      true while rubber-band is active
    Vector2 dragSelStartW      world-space click position
    Vector2 dragSelCurW        world-space current mouse position

On release, a rectangle is built from min/max of start and current. Any gate whose rect overlaps that rectangle via CheckCollisionRecs is added to selection. A 4 px minimum size prevents accidental selections from plain clicks.

---

## Input handling order

The left-click handler inside main() processes clicks in priority order:

    1. Delete button (x in corner)
    2. Toggle expand button (> in corner) for CUSTOM gates
    3. Double-click rename (INPUT / OUTPUT)
    4. Double-click open internal viewer (CUSTOM)
    5. Checkbox toggle (INPUT)
    6. Node hit (start or complete a wire)
    7. Shift+click gate (toggle selection)
    8. Gate drag start OR rubber-band drag start

Each step only runs if `handled` is still false. Once any step sets handled=true the rest are skipped.

---

## Adding a persistent state variable

For any state that should be set once and reused across frames, declare it in main() before the game loop:

    int myState = 0;

For state that should reset whenever the analysis modal opens, reset it in the button handler:

    if(DrawToolbarButton(...)) {
        showAnalysis = !showAnalysis;
        myState = 0;          // reset here
    }

For state that should persist until the window closes (like circuit files on disk), use the filesystem API already in scope via `namespace fs = std::filesystem`.

---

## Building and iterating quickly

To rebuild and rerun without retyping the full compile command, add this one-liner to a shell alias or a Makefile:

    alias build='g++ gatesim.cpp -o gatesim $(pkg-config --libs --cflags raylib) -std=c++17 && ./gatesim'

Or with a minimal Makefile:

    gatesim: gatesim.cpp
        g++ $< -o $@ $(shell pkg-config --libs --cflags raylib) -std=c++17

    run: gatesim
        ./gatesim

Then `make run` rebuilds only if the source has changed.
