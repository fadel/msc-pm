#include "geometry.h"

#include <cmath>

static const float PI = 3.1415f;

int calculateCircleVertexCount(float diameter)
{
    // 10 * sqrt(r) \approx 2*pi / acos(1 - 1 / (4*r))
    return int(10.0 * sqrt(diameter / 2));
}

void updateCircleGeometry(QSGGeometry *geometry, float diameter, float cx, float cy)
{
    int vertexCount = geometry->vertexCount();

    float theta = 2 * PI / float(vertexCount);
    float c = cosf(theta);
    float s = sinf(theta);
    float x = diameter / 2;
    float y = 0;

    QSGGeometry::Point2D *vertexData = geometry->vertexDataAsPoint2D();
    for (int i = 0; i < vertexCount; i++) {
        vertexData[i].set(x + cx, y + cy);

        float t = x;
        x = c*x - s*y;
        y = s*t + c*y;
    }
}

void updateRectGeometry(QSGGeometry *geometry, float x, float y, float w, float h)
{
    QSGGeometry::Point2D *vertexData = geometry->vertexDataAsPoint2D();

    vertexData[0].set(x, y);
    vertexData[1].set(x + w, y);
    vertexData[2].set(x + w, y + h);
    vertexData[3].set(x, y + h);
}

void updateCrossHairGeometry(QSGGeometry *geometry,
                             float x,
                             float y,
                             float thickness,
                             float length)
{
    QSGGeometry::Point2D *vertexData = geometry->vertexDataAsPoint2D();
    if (geometry->vertexCount() != 12) {
        return;
    }

    vertexData[ 0].set(x + thickness, y - thickness);
    vertexData[ 1].set(x + length,    y - thickness);
    vertexData[ 2].set(x + length,    y + thickness);
    vertexData[ 3].set(x + thickness, y + thickness);
    vertexData[ 4].set(x + thickness, y + length);
    vertexData[ 5].set(x - thickness, y + length);
    vertexData[ 6].set(x - thickness, y + thickness);
    vertexData[ 7].set(x - length,    y + thickness);
    vertexData[ 8].set(x - length,    y - thickness);
    vertexData[ 9].set(x - thickness, y - thickness);
    vertexData[10].set(x - thickness, y - length);
    vertexData[11].set(x + thickness, y - length);
}
