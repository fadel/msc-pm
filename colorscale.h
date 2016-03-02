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
    int numColors() const { return m_colors.size(); }

    template<typename OutputIterator>
    void sample(int samples, OutputIterator it) const;

    static QColor lerp(const QColor &c1, const QColor &c2, float _t);

protected:
    float m_min, m_max;
    QList<QColor> m_colors;
};

template<typename OutputIterator>
void ColorScale::sample(int samples, OutputIterator it) const
{
    if (samples < 1) {
        return;
    }

    float step = (max() - min()) / samples;
    qreal r, g, b;
    for (float t = min(); samples-- > 0; t += step) {
        color(t).getRgbF(&r, &g, &b);
        *it++ = r;
        *it++ = g;
        *it++ = b;
    }
}

#endif // COLORSCALE_H
