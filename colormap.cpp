#include "colormap.h"

#include <QOpenGLFunctions>
#include <QSGSimpleTextureNode>

class ColormapTexture
    : public QSGDynamicTexture
{
public:
    ColormapTexture(const std::vector<float> *cmap);
    ~ColormapTexture();

    bool hasAlphaChannel() const { return false; }
    bool hasMipmaps() const { return false; }
    void bind();
    bool updateTexture();

    int textureId() const { return m_texture; }
    QSize textureSize() const { return m_size; }

private:
    QOpenGLFunctions gl;

    QSize m_size;
    GLuint m_texture;
    const std::vector<float> *m_cmap;
};

ColormapTexture::ColormapTexture(const std::vector<float> *cmap)
    : gl(QOpenGLContext::currentContext())
    , m_size(Colormap::SAMPLES, 1)
    , m_cmap(cmap)
{
    // Setup OpenGL texture
    gl.glGenTextures(1, &m_texture);
    gl.glBindTexture(GL_TEXTURE_2D, m_texture);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_size.width(), m_size.height(),
            0, GL_RGB, GL_FLOAT, 0);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

ColormapTexture::~ColormapTexture()
{
    gl.glDeleteTextures(1, &m_texture);
}

void ColormapTexture::bind()
{
    gl.glBindTexture(GL_TEXTURE_2D, m_texture);
}

bool ColormapTexture::updateTexture()
{
    gl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_size.width(), m_size.height(),
            GL_RGB, GL_FLOAT, m_cmap->data());
    return true;
}

Colormap::Colormap(QQuickItem *parent)
    : QQuickItem(parent)
    , m_texture(0)
    , m_shouldUpdateTexture(false)
    , m_cmap(3*SAMPLES)
{
    setFlag(QQuickItem::ItemHasContents);
}

Colormap::~Colormap()
{
    if (m_texture) {
        delete m_texture;
    }
}


void Colormap::setColorScale(const ColorScale &scale)
{
    scale.sample(SAMPLES, m_cmap.begin());
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

    if (m_shouldUpdateTexture) {
        m_texture->updateTexture();
        m_shouldUpdateTexture = false;
    }

    return root;
}
