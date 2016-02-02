#include "colorscale.h"

ColorScale::ColorScale(const QColor &firstColor, const QColor &lastColor)
    : m_colors{{firstColor, lastColor}}
{
    setExtents(0, 1);
}

ColorScale::ColorScale(std::initializer_list<QColor> colors)
    : m_colors(colors)
{
    setExtents(0, 1);
}

ColorScale::ColorScale(const QList<QColor> &colors)
    : m_colors(colors)
{
    setExtents(0, 1);
}

ColorScale::~ColorScale()
{
}

void ColorScale::setExtents(float min, float max)
{
    if (min >= max) {
        return;
    }

    m_min = min;
    m_max = max;
}

static QColor lerp(const QColor &c1, const QColor &c2, float _t)
{
    qreal r1, g1, b1, a1;
    qreal r2, g2, b2, a2;
    qreal t = _t;

    c1.getRgbF(&r1, &g1, &b1, &a1);
    c2.getRgbF(&r2, &g2, &b2, &a2);
    QColor color;
    color.setRgbF(r1 * (1 - t) + r2 * t,
                  g1 * (1 - t) + g2 * t,
                  b1 * (1 - t) + b2 * t,
                  a1 * (1 - t) + a2 * t);
    return color;
}

QColor ColorScale::color(float t) const
{
    if (t < m_min || t > m_max) {
        return QColor();
    }

    // normalize t
    t = (t - m_min) / (m_max - m_min);

    // two colors, use a simpler solution
    if (m_colors.size() == 2) {
        return lerp(m_colors.first(), m_colors.last(), t);
    }

    // find which colors in the scale are adjacent to ours
    int i = int(t * m_colors.size());
    int j = i + 1;
    if (i >= m_colors.size() - 1) {
        return QColor(m_colors.last());
    }

    // normalize t between the two colors
    float step = 1.0f / m_colors.size();
    t = (t - i*step) / (j*step - i*step);
    return lerp(m_colors[i], m_colors[j], t);
}
