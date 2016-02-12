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
        RedGrayBlue,
        Rainbow,
    };

    QColor color(qreal t) const;

    static ContinuousColorScale builtin(enum BuiltinContinuousColorScale);
};

#endif // CONTINUOUSCOLORSCALE_H
