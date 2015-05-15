#ifndef GLYPH_H
#define GLYPH_H

#include <QColor>
#include <QtQuick/QQuickItem>

class Glyph : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal size READ size WRITE setSize NOTIFY sizeChanged)

public:
    enum GlyphType {
        GLYPH_CIRCLE,
        GLYPH_SQUARE,
        GLYPH_STAR,
        GLYPH_CROSS
    };

    Glyph(QQuickItem *parent = 0);

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);

    QColor color() const { return m_color; }
    void setColor(const QColor &color);

    qreal size() const { return m_size; }
    void setSize(qreal size);

signals:
    void colorChanged();
    void sizeChanged();

public slots:

private:
    QColor m_color;
    qreal m_size;
};

#endif // GLYPH_H
