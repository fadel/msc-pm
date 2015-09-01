#ifndef COLORSCALE_H
#define COLORSCALE_H

#include <initializer_list>
#include <QColor>
#include <QList>

class ColorScale
{
public:
    ColorScale(const QColor &firstColor, const QColor &lastColor);
    ColorScale(std::initializer_list<QColor> colors);
    ColorScale(const QList<QColor> &colors);
    virtual ~ColorScale();

    virtual QColor color(qreal t) const;
    void setExtents(qreal min, qreal max);

protected:
    qreal m_min, m_max;
    QList<QColor> m_colors;
};

#endif // COLORSCALE_H
