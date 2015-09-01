#ifndef CONTINUOUSCOLORSCALE_H
#define CONTINUOUSCOLORSCALE_H

#include "colorscale.h"

class ContinuousColorScale : public ColorScale
{
public:
    ContinuousColorScale();

    QColor color(qreal t) const;
};

#endif // CONTINUOUSCOLORSCALE_H
