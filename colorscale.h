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

    QColor operator ()(float t) const { return color(t); }
    virtual QColor color(float t) const;

    void setExtents(float min, float max);
    float min() const { return m_min; }
    float max() const { return m_max; }

protected:
    float m_min, m_max;
    QList<QColor> m_colors;
};

#endif // COLORSCALE_H
