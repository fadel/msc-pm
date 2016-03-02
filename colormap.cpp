#include "colormap.h"

#include <algorithm>

#include <QOpenGLFunctions>
#include <QSGSimpleTextureNode>

class ColormapTexture
    : public QSGDynamicTexture
{
public:
    ColormapTexture(const std::vector<float> *cmap,
                    Colormap::Orientation orientation = Colormap::Horizontal);
    ~ColormapTexture();

    bool hasAlphaChannel() const { return false; }
    bool hasMipmaps() const { return false; }
    void bind();
    bool updateTexture();

    int textureId() const { return m_texture; }
    QSize textureSize() const { return m_size; }

    void setOrientation(Colormap::Orientation orientation);

private:
    QOpenGLFunctions gl;

    Colormap::Orientation m_orientation;
    QSize m_size;
    GLuint m_texture;
    const std::vector<float> *m_cmap;
};

ColormapTexture::ColormapTexture(const std::vector<float> *cmap,
                                 Colormap::Orientation orientation)
    : gl(QOpenGLContext::currentContext())
    , m_cmap(cmap)
{
    // Setup OpenGL texture
    gl.glGenTextures(1, &m_texture);
    gl.glBindTexture(GL_TEXTURE_2D, m_texture);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    setOrientation(orientation);
}

ColormapTexture::~ColormapTexture()
{
    gl.glDeleteTextures(1, &m_texture);
}

void ColormapTexture::bind()
{
    gl.glBindTexture(GL_TEXTURE_2D, m_texture);
}

void ColormapTexture::setOrientation(Colormap::Orientation orientation)
{
    if (m_orientation == orientation) {
        return;
    }

    m_orientation = orientation;
    updateTexture();
}

bool ColormapTexture::updateTexture()
{
    switch (m_orientation) {
    case Colormap::Horizontal:
        m_size.setWidth(m_cmap->size() / 3);
        m_size.setHeight(1);
        break;
    case Colormap::Vertical:
        m_size.setWidth(1);
        m_size.setHeight(m_cmap->size() / 3);
        break;
    }

    gl.glBindTexture(GL_TEXTURE_2D, m_texture);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_size.width(), m_size.height(),
            0, GL_RGB, GL_FLOAT, m_cmap->data());
    return true;
}

Colormap::Colormap(QQuickItem *parent)
    : QQuickItem(parent)
    , m_texture(0)
    , m_shouldUpdateTexture(false)
    , m_orientation(Colormap::Horizontal)
{
    setFlag(QQuickItem::ItemHasContents);
}

Colormap::~Colormap()
{
    if (m_texture) {
        delete m_texture;
    }
}

static void reverseCMap(std::vector<float> &cmap)
{
    decltype(cmap.size()) i = 0, j = cmap.size() - 3;

    while (i < j) {
        std::swap(cmap[i++], cmap[j++]);
        std::swap(cmap[i++], cmap[j++]);
        std::swap(cmap[i++], cmap[j++]);

        j -= 6;
    }
}

void Colormap::setOrientation(Colormap::Orientation orientation)
{
    if (m_orientation == orientation) {
        return;
    }

    if (!m_cmap.empty()) {
        reverseCMap(m_cmap);
    }

    m_orientation = orientation;
    m_shouldUpdateOrientation = true;
    update();
}

void Colormap::setColorScale(const ColorScale *scale)
{
    m_cmap.resize(SAMPLES * 3);
    scale->sample(SAMPLES, m_cmap.data());
    if (m_orientation == Colormap::Vertical) {
        reverseCMap(m_cmap);
    }

    emit colorScaleChanged(scale);

    m_shouldUpdateTexture = true;
    update();
}

QSGNode *Colormap::newSceneGraph()
{
    QSGSimpleTextureNode *node = new QSGSimpleTextureNode;
    m_texture = new ColormapTexture(&m_cmap);

    node->setTexture(m_texture);
    node->setOwnsTexture(false);

    const QSize &texSize = m_texture->textureSize();
    node->setSourceRect(0, 0, texSize.width(), texSize.height());

    return node;
}

QSGNode *Colormap::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = oldNode ? oldNode : newSceneGraph();

    QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(root);
    node->setRect(x(), y(), width(), height());

    ColormapTexture *texture = static_cast<ColormapTexture *>(m_texture);
    if (m_shouldUpdateOrientation) {
        texture->setOrientation(m_orientation);
        m_shouldUpdateOrientation = false;
    }

    if (m_shouldUpdateTexture) {
        texture->updateTexture();
        m_shouldUpdateTexture = false;
    }

    return root;
}
