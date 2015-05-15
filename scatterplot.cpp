#include "scatterplot.h"

#include <cstdio>
#include "glyph.h"
#include <QSGNode>
#include <QSGGeometryNode>

const int GLYPH_SIZE = 10;

Scatterplot::Scatterplot()
{
    setFlag(QQuickItem::ItemHasContents);
}

Scatterplot::~Scatterplot()
{
}

void Scatterplot::setData(const arma::mat &data)
{
    if (data.n_cols != 2)
        return;

    m_data = data;
    qreal xmin = m_data.col(0).min(),
          xmax = m_data.col(0).max(),
          ymin = m_data.col(1).min(),
          ymax = m_data.col(1).max();

    for (arma::uword i = 0; i < m_data.n_rows; i++) {
        arma::rowvec row = m_data.row(i);

        Glyph *glyph = new Glyph();

        glyph->setSize(5);
        glyph->setX((row[0] - xmin) / (xmax - xmin) * width());
        glyph->setY((row[1] - ymin) / (ymax - ymin) * height());

        glyph->setParent(this);
    }

    update();
}

QSGNode *Scatterplot::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *node = 0;

    if (!oldNode) {
        node = new QSGNode;
        for (QObjectList::const_iterator it = children().begin(); it != children().end(); it++)
            node->appendChildNode(static_cast<Glyph *>(*it)->updatePaintNode(0, 0));
    } else {
        node = static_cast<QSGNode *>(oldNode);
    }

    node->markDirty(QSGNode::DirtyGeometry);

    return node;
}
