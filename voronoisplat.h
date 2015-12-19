#ifndef VORONOISPLAT_H
#define VORONOISPLAT_H

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <armadillo>

#include "colorscale.h"

class VoronoiSplat; // defined after this class

class VoronoiSplatRenderer
    : public QQuickFramebufferObject::Renderer
{
public:
    // 'size' must be square (and power of 2)
    VoronoiSplatRenderer(const QSize &size);
    ~VoronoiSplatRenderer();

    void synchronize(QQuickFramebufferObject *item);

    // 'points' should be a 2D points matrix (each point in a row)
    void setSites(const arma::mat &points);

    // Set the value to be colormapped in each site
    void setValues(const arma::vec &values);

    // Set colormap data based on the given color scale;
    void setColorMap(const ColorScale *scale);

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size);
    void render();

private:
    void setupShaders();
    void setupVAOs();
    void setupTextures();
    void copyPoints(const arma::mat &points);
    void computeDT();

    VoronoiSplat *m_item;
    QOpenGLFunctions gl;
    QOpenGLShaderProgram *m_program1, *m_program2;
    GLuint m_VBOs[3];
    GLuint m_textures[2], m_colorMapTex;
    QOpenGLVertexArrayObject m_sitesVAO, m_2ndPassVAO;

    std::vector<float> m_sites, m_values;
    QSize m_size;
};

class VoronoiSplat
    : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    VoronoiSplat(QQuickItem *parent = 0);
    Renderer *createRenderer() const;

    const arma::mat &points() const { return m_points; }
    const arma::vec &values() const { return m_values; }
    const ColorScale *colorScale() const { return m_colorScale; }

    bool colorScaleUpdated() const { return m_colorScaleUpdated; }
    bool pointsUpdated() const { return m_pointsUpdated; }
    bool valuesUpdated() const { return m_valuesUpdated; }

public slots:
    void setColorScale(const ColorScale *scale)
    {
        m_colorScale = scale;
        m_colorScaleUpdated = true;
        update();
    }

    void setPoints(const arma::mat &points)
    {
        m_points = points;
        m_pointsUpdated = true;
        update();
    }

    void setValues(const arma::vec &values)
    {
        m_values = values;
        m_valuesUpdated = true;
        update();
    }

    void setUpdated(bool updated)
    {
        m_colorScaleUpdated = updated;
        m_pointsUpdated     = updated;
        m_valuesUpdated     = updated;
    }

private:
    bool m_colorScaleUpdated, m_pointsUpdated, m_valuesUpdated;
    const ColorScale *m_colorScale;
    arma::mat m_points;
    arma::vec m_values;
};

#endif // VORONOISPLAT_H
