#ifndef CONTINUOUSCOLORSCALE_H
#define CONTINUOUSCOLORSCALE_H

#include "colorscale.h"

class ContinuousColorScale
    : public ColorScale
{
public:
    ContinuousColorScale(std::initializer_list<QColor> colors);

    enum BuiltinContinuousColorScale {
        HeatedObjects,
        Rainbow,
    };

    QColor color(float t) const;

    static ContinuousColorScale builtin(BuiltinContinuousColorScale);
    static ContinuousColorScale *builtin(BuiltinContinuousColorScale, void *);
};

#endif // CONTINUOUSCOLORSCALE_H
