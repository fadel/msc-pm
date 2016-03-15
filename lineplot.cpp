#include "lineplot.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QQuickWindow>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QSGGeometryNode>
#include <QSGSimpleRectNode>

// From CUBu
#include "cpubundling.h"
#include "graph.h"

#include "continuouscolorscale.h"
#include "geometry.h"
#include "scatterplot.h"

LinePlot::LinePlot(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_sx(0, 1, 0, 1)
    , m_sy(0, 1, 0, 1)
    , m_xyChanged(false)
    , m_valuesChanged(false)
    , m_colorScaleChanged(false)
    , m_bundleGPU(true)
{
    setTextureFollowsItemSize(true);
}

void LinePlot::setColorScale(const ColorScale *colorScale)
{
    m_colorScale = colorScale;
    if (m_values.size() > 0) {
        // FIXME
        m_colorScaleChanged = true;
        update();
    }
}

void LinePlot::bundle()
{
    Graph g(m_xy.n_rows);
    PointSet points;

    for (arma::uword i = 0; i < m_xy.n_rows; i++) {
        const arma::rowvec &row = m_xy.row(i);
        points.push_back(Point2d(row[0], row[1]));

        if (i > 0) {
            g(i - 1, i) = m_values[i];
            g(i, i - 1) = m_values[i];
        }
    }

    m_gdPtr.reset(new GraphDrawing);
    m_gdPtr.get()->build(&g, &points);

    CPUBundling bundling(std::min(width(), height()));
    bundling.setInput(m_gdPtr.get());
    if (m_bundleGPU) {
        bundling.bundleGPU();
    } else {
        bundling.bundleCPU();
    }
}

void LinePlot::setXY(const arma::mat &xy)
{
    if (xy.n_cols != 2) {
        return;
    }

    m_xy = xy;
    m_xyChanged = true;
    emit xyChanged(m_xy);

    // Build the line plot's internal representation: graph where each endpoint
    // of a line is a node, each line is a link; and then bundle the graph's
    // edges
    bundle();

    update();
}

void LinePlot::setValues(const arma::vec &values)
{
    if (m_xy.n_rows > 0
        && (values.n_elem > 0 && values.n_elem != m_xy.n_rows)) {
        return;
    }

    m_values.resize(values.n_elem);
    LinearScale<float> scale(values.min(), values.max(), 0, 1.0f);
    std::transform(values.begin(), values.end(), m_values.begin(), scale);
    emit valuesChanged(values);

    m_valuesChanged = true;
    update();
}

void LinePlot::setScale(const LinearScale<float> &sx,
                        const LinearScale<float> &sy)
{
    m_sx = sx;
    m_sy = sy;
    emit scaleChanged(m_sx, m_sy);
    update();
}

// ----------------------------------------------------------------------------

class LinePlotRenderer
    : public QQuickFramebufferObject::Renderer
{
public:
    LinePlotRenderer();
    virtual ~LinePlotRenderer();

protected:
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size);
    void render();
    void synchronize(QQuickFramebufferObject *item);

private:
    void setupShaders();
    void setupVAOs();
    void setupTextures();

    void updatePoints();
    void updateValues();
    void updateColormap();
    void updateTransform();

    void copyPolylines(const GraphDrawing *gd);

    QSize m_size;
    std::vector<float> m_points;
    const std::vector<float> *m_values;
    std::vector<int> m_offsets;
    float m_alpha, m_beta;
    GLfloat m_transform[4][4];
    LinearScale<float> m_sx, m_sy;

    QQuickWindow *m_window; // used to reset OpenGL state (as per docs)
    QOpenGLFunctions gl;
    QOpenGLShaderProgram *m_program;
    GLuint m_VBOs[2], m_colormapTex;
    QOpenGLVertexArrayObject m_VAO;
    bool m_pointsChanged, m_valuesChanged, m_colormapChanged;
};

QQuickFramebufferObject::Renderer *LinePlot::createRenderer() const
{
    return new LinePlotRenderer;
}

LinePlotRenderer::LinePlotRenderer()
    : m_sx(0, 1, 0, 1)
    , m_sy(0, 1, 0, 1)
    , gl(QOpenGLContext::currentContext())
{
    std::fill(&m_transform[0][0], &m_transform[0][0] + 16, 0.0f);
    m_transform[3][3] = 1.0f;

    setupShaders();
    setupVAOs();
    setupTextures();
}

void LinePlotRenderer::setupShaders()
{
    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
R"EOF(#version 440

uniform mat4 transform;

in vec2 vert;
in float scalar;

out float value;

void main() {
  gl_Position = transform * vec4(vert, 0.0, 1.0);
  value = scalar;
}
)EOF");
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
R"EOF(#version 440

uniform sampler2D colormap;
in float value;

layout (location = 0) out vec4 fragColor;

vec3 getRGB(float value) {
  return texture(colormap, vec2(mix(0.005, 0.995, value), 0)).rgb;
}

void main() {
  fragColor = vec4(getRGB(value), 1.0);
}
)EOF");
    m_program->link();
}

void LinePlotRenderer::setupVAOs()
{
    gl.glGenBuffers(2, m_VBOs);

    m_VAO.create();
    m_VAO.bind();
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[0]);
    int vertAttrib = m_program->attributeLocation("vert");
    gl.glVertexAttribPointer(vertAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    gl.glEnableVertexAttribArray(vertAttrib);

    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[1]);
    int valueAttrib = m_program->attributeLocation("scalar");
    gl.glVertexAttribPointer(valueAttrib, 1, GL_FLOAT, GL_FALSE, 0, 0);
    gl.glEnableVertexAttribArray(valueAttrib);
    m_VAO.release();
}

void LinePlotRenderer::setupTextures()
{
    gl.glGenTextures(1, &m_colormapTex);
    gl.glBindTexture(GL_TEXTURE_2D, m_colormapTex);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

LinePlotRenderer::~LinePlotRenderer()
{
    delete m_program;
}

QOpenGLFramebufferObject *LinePlotRenderer::createFramebufferObject(const QSize &size)
{
    m_size = size;
    return QQuickFramebufferObject::Renderer::createFramebufferObject(m_size);
}

void LinePlotRenderer::render()
{
    if (!m_pointsChanged || !m_valuesChanged || !m_colormapChanged) {
        return;
    }

    // Update OpenGL buffers and textures as needed
    if (m_pointsChanged) {
        updatePoints();
    }
    if (m_valuesChanged) {
        updateValues();
    }
    if (m_colormapChanged) {
        updateColormap();
    }

    m_program->bind();
    m_program->setUniformValue("transform", m_transform);

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, m_colormapTex);
    m_program->setUniformValue("colormap", 0);

    gl.glClearColor(0, 0, 0, 0);
    gl.glClear(GL_COLOR_BUFFER_BIT);

    m_VAO.bind();
    for (int i = 0; i < m_offsets.size() - 1; i++) {
        gl.glDrawArrays(GL_LINES, m_offsets[i], m_offsets[i + 1]);
    }
    m_VAO.release();

    m_program->release();
    m_window->resetOpenGLState();
}

void LinePlotRenderer::copyPolylines(const GraphDrawing *gd)
{
    if (!gd || gd->draw_order.empty()) {
        return;
    }

    int pointsNum = 0;
    m_offsets.clear();
    m_offsets.push_back(0);
    for (std::pair<float, const GraphDrawing::Polyline *> p : gd->draw_order) {
        pointsNum += p.second->size();
        m_offsets.push_back(pointsNum);
    }
    m_points.resize(2 * pointsNum);

    arma::uword i = 0, k = 0;
    for (std::pair<float, const GraphDrawing::Polyline *> p : gd->draw_order) {
        for (arma::uword j = 0; j < p.second->size(); j++) {
            m_points[i + j + 0] = (*p.second)[j].x;
            m_points[i + j + 1] = (*p.second)[j].y;
        }

        i += p.second->size();
        k++;
    }
}

void LinePlotRenderer::synchronize(QQuickFramebufferObject *item)
{
    LinePlot *plot = static_cast<LinePlot *>(item);

    m_pointsChanged   = plot->xyChanged();
    m_valuesChanged   = plot->valuesChanged();
    m_colormapChanged = plot->colorScaleChanged();

    copyPolylines(plot->graphDrawing());
    m_values = &(plot->values());
    m_sx     = plot->scaleX();
    m_sy     = plot->scaleY();
    m_window = plot->window();

    // Reset so that we have the correct values by the next synchronize()
    plot->setXYChanged(false);
    plot->setValuesChanged(false);
    plot->setColorScaleChanged(false);
}

void LinePlotRenderer::updateTransform()
{
    GLfloat w = m_size.width(), h = m_size.height();

    GLfloat rangeOffset = Scatterplot::PADDING / w;
    m_sx.setRange(rangeOffset, 1.0f - rangeOffset);
    GLfloat sx = 2.0f * m_sx.slope();
    GLfloat tx = 2.0f * m_sx.offset() - 1.0f;

    rangeOffset = Scatterplot::PADDING / h;
    m_sy.setRange(1.0f - rangeOffset, rangeOffset);
    GLfloat sy = 2.0f * m_sy.slope();
    GLfloat ty = 2.0f * m_sy.offset() - 1.0f;

    // The transform matrix should be this (but transposed -- column major):
    // [   sx  0.0f  0.0f   -tx ]
    // [ 0.0f    sy  0.0f   -ty ]
    // [ 0.0f  0.0f  0.0f  0.0f ]
    // [ 0.0f  0.0f  0.0f  1.0f ]
    m_transform[0][0] = sx;
    m_transform[1][1] = sy;
    m_transform[3][0] = tx;
    m_transform[3][1] = ty;
}

void LinePlotRenderer::updatePoints()
{
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[0]);
    gl.glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(float),
            m_points.data(), GL_DYNAMIC_DRAW);

    updateTransform();
    m_pointsChanged = false;
}

void LinePlotRenderer::updateValues()
{
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[1]);
    gl.glBufferData(GL_ARRAY_BUFFER, m_values->size() * sizeof(float),
            m_values->data(), GL_DYNAMIC_DRAW);

    m_valuesChanged = false;
}

void LinePlotRenderer::updateColormap()
{
    gl.glBindTexture(GL_TEXTURE_2D, m_colormapTex);
    //gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_cmap->size() / 3, 1, 0, GL_RGB,
    //        GL_FLOAT, m_cmap->data());

    m_colormapChanged = false;
}
