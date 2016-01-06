#ifndef VORONOISPLAT_H
#define VORONOISPLAT_H

#include <QSGDynamicTexture>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <armadillo>

#include "colorscale.h"

class VoronoiSplatTexture
    : public QSGDynamicTexture
{
public:
    // 'size' must be square (and power of 2)
    VoronoiSplatTexture(const QSize &size);
    virtual ~VoronoiSplatTexture();

    void bind();

    // When true is returned, should probably call resetOpenGLState() on window
    bool updateTexture();

    bool hasAlphaChannel() const { return true; }
    bool hasMipmaps() const { return false; }
    int textureId() const { return m_tex; }
    QSize textureSize() const { return m_size; }

    // 'points' should be a 2D points matrix (each point in a row)
    void setSites(const arma::mat &points);

    // Set the value to be colormapped in each site
    void setValues(const arma::vec &values);

    // Set colormap data based on the given color scale;
    void setColormap(const ColorScale *scale);

private:
    void setupShaders();
    void setupVAOs();
    void setupTextures();

    void updateSites();
    void updateValues();
    void updateColormap();
    void computeDT();

    QOpenGLFunctions gl;
    QOpenGLShaderProgram *m_program1, *m_program2;
    QOpenGLFramebufferObject m_FBO;
    GLuint m_VBOs[3];
    GLuint m_textures[2], m_colormapTex, m_tex;
    QOpenGLVertexArrayObject m_sitesVAO, m_2ndPassVAO;

    std::vector<float> m_sites, m_values, m_cmap;
    QSize m_size;

    bool m_sitesUpdated, m_valuesUpdated, m_colormapUpdated;
};

#endif // VORONOISPLAT_H
