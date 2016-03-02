#ifndef DIVERGENTCOLORSCALE_H
#define DIVERGENTCOLORSCALE_H

#include "colorscale.h"

class DivergentColorScale
    : public ColorScale
{
public:
    DivergentColorScale(const QColor &color1,
                        const QColor &colorMiddle,
                        const QColor &color2);

    enum BuiltinDivergentColorScale {
        RedGrayBlue
    };

    QColor color(float t) const;

    static DivergentColorScale builtin(BuiltinDivergentColorScale);
    static DivergentColorScale *builtin(BuiltinDivergentColorScale, void *);
};

#endif // DIVERGENTCOLORSCALE_H
