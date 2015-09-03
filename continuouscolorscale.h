#ifndef CONTINUOUSCOLORSCALE_H
#define CONTINUOUSCOLORSCALE_H

#include "colorscale.h"

class ContinuousColorScale : public ColorScale
{
public:
    ContinuousColorScale(std::initializer_list<QColor> colors);

    enum BuiltinContinuousColorScale {
        HEATED_OBJECTS,
        RED_GRAY_BLUE
    };

    QColor color(qreal t) const;

    static ContinuousColorScale builtin(enum BuiltinContinuousColorScale);
};

#endif // CONTINUOUSCOLORSCALE_H
