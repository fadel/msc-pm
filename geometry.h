#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <QSGGeometry>

// Circle
int calculateCircleVertexCount(float radius);
void updateCircleGeometry(QSGGeometry *geometry, float radius, float cx, float cy);

// Rect
void updateRectGeometry(QSGGeometry *geometry, float x, float y, float w, float h);

// Crosshair
void updateCrossHairGeometry(QSGGeometry *geometry,
                             float x,
                             float y,
                             float thickness,
                             float length);
#endif // GEOMETRY_H
