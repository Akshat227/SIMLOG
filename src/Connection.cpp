#include "Connection.h"

static void DrawBezierCurve(Vector2 start, Vector2 end, Color color) {
    Vector2 cp1 = {start.x + (end.x - start.x) * 0.5f, start.y};
    Vector2 cp2 = {start.x + (end.x - start.x) * 0.5f, end.y};
    Vector2 points[30];
    for (int i = 0; i < 30; i++) {
        float t = (float)i / 29.0f;
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float tt2 = tt * t;
        points[i].x = uuu * start.x + 3 * uu * t * cp1.x + 3 * u * tt * cp2.x + tt2 * end.x;
        points[i].y = uuu * start.y + 3 * uu * t * cp1.y + 3 * u * tt * cp2.y + tt2 * end.y;
    }
    for (int i = 0; i < 29; i++) {
        DrawLineEx(points[i], points[i + 1], 3, color);
    }
}

Connection::Connection(Pin* start, Pin* end) : startPin(start), endPin(end) {}

void Connection::Draw() const {
    Color lineColor = startPin->value ? GREEN : GRAY;
    DrawBezierCurve(startPin->position, endPin->position, lineColor);
}