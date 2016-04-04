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

static const int SAMPLES = 128;

LinePlot::LinePlot(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_sx(0, 1, 0, 1)
    , m_sy(0, 1, 0, 1)
    , m_linesChanged(false)
    , m_valuesChanged(false)
    , m_colorScaleChanged(false)

    // Q_PROPERTY's
    , m_iterations(15)
    , m_kernelSize(32)
    , m_smoothingFactor(0.2)
    , m_smoothingIterations(1)

    , m_blockEndpoints(true)
    , m_endsIterations(0)
    , m_endsKernelSize(32)
    , m_endsSmoothingFactor(0.5)

    , m_edgeSampling(15)
    , m_advectionSpeed(0.5)
    , m_relaxation(0)
    , m_bundleGPU(true)
{
    setFlag(QQuickItem::ItemHasContents);
    setTextureFollowsItemSize(false);
}

void LinePlot::setColorScale(const ColorScale *scale)
{
    m_cmap.resize(SAMPLES * 3);
    scale->sample(SAMPLES, m_cmap.begin());

    setColorScaleChanged(true);
    update();
}

void LinePlot::bundle()
{
    m_gdBundlePtr.reset(new GraphDrawing);
    *m_gdBundlePtr.get() = *m_gdPtr.get();

    CPUBundling bundling(std::min(width(), height()));
    bundling.setInput(m_gdBundlePtr.get());

    bundling.niter  = m_iterations;
    bundling.h      = m_kernelSize;
    bundling.lambda = m_smoothingFactor;
    bundling.liter  = m_smoothingIterations;

    bundling.block_endpoints = m_blockEndpoints;
    bundling.niter_ms        = m_endsIterations;
    bundling.h_ms            = m_endsKernelSize;
    bundling.lambda_ends     = m_endsSmoothingFactor;

    bundling.spl = m_edgeSampling;
    bundling.eps = m_advectionSpeed;
    // TODO: use m_relaxation as lerp param towards original (without bundling)

    if (m_bundleGPU) {
        bundling.bundleGPU();
    } else {
        bundling.bundleCPU();
    }

    setLinesChanged(true);
}

void LinePlot::setLines(const arma::uvec &indices, const arma::mat &Y)
{
    if (indices.n_elem % 2 != 0 || Y.n_cols != 2) {
        return;
    }

    m_lines = Y.rows(indices);

    // Build the line plot's internal representation: a graph where each
    // endpoint of a line is a node, each line is a link...
    Graph g(Y.n_rows);
    PointSet points;

    m_sx.setRange(0, width());
    m_sy.setRange(0, height());
    for (arma::uword i = 0; i < Y.n_rows; i++) {
        points.push_back(Point2d(m_sx(Y(i, 0)), m_sy(Y(i, 1))));
    }

    for (arma::uword k = 0; k < m_values.size(); k++) {
        arma::uword i = indices(2*k + 0),
                    j = indices(2*k + 1);

        g(i, j) = g(j, i) = m_values[k];
    }

    m_gdPtr.reset(new GraphDrawing);
    m_gdPtr.get()->build(&g, &points);

    // ... then bundle the edges
    bundle();

    emit linesChanged(m_lines);
    update();
}

void LinePlot::setValues(const arma::vec &values)
{
    if (m_lines.n_rows > 0
        && (values.n_elem > 0 && values.n_elem != m_lines.n_rows)) {
        return;
    }

    m_values.resize(values.n_elem);
    std::copy(values.begin(), values.end(), m_values.begin());
    emit valuesChanged(values);

    setValuesChanged(true);
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

// Q_PROPERTY's
void LinePlot::setIterations(int iterations) {
    if (m_iterations == iterations) {
        return;
    }

    m_iterations = iterations;
    emit iterationsChanged(m_iterations);

    bundle();
    update();
}

void LinePlot::setKernelSize(float kernelSize)
{
    if (m_kernelSize == kernelSize) {
        return;
    }

    m_kernelSize = kernelSize;
    emit kernelSizeChanged(m_kernelSize);

    bundle();
    update();
}

void LinePlot::setSmoothingFactor(float smoothingFactor)
{
    if (m_smoothingFactor == smoothingFactor) {
        return;
    }

    m_smoothingFactor = smoothingFactor;
    emit smoothingFactorChanged(m_smoothingFactor);

    bundle();
    update();
}

void LinePlot::setSmoothingIterations(int smoothingIterations)
{
    if (m_smoothingIterations == smoothingIterations) {
        return;
    }

    m_smoothingIterations = smoothingIterations;
    emit smoothingIterationsChanged(m_smoothingIterations);

    bundle();
    update();
}

void LinePlot::setBlockEndpoints(bool blockEndpoints)
{
    if (m_blockEndpoints == blockEndpoints) {
        return;
    }

    m_blockEndpoints = blockEndpoints;
    emit blockEndpointsChanged(m_blockEndpoints);

    bundle();
    update();
}

void LinePlot::setEndsIterations(int endsIterations)
{
    if (m_endsIterations == endsIterations) {
        return;
    }

    m_endsIterations = endsIterations;
    emit endsIterationsChanged(m_endsIterations);

    bundle();
    update();
}

void LinePlot::setEndsKernelSize(float endsKernelSize)
{
    if (m_endsKernelSize == endsKernelSize) {
        return;
    }

    m_endsKernelSize = endsKernelSize;
    emit endsKernelSizeChanged(m_endsKernelSize);

    bundle();
    update();
}

void LinePlot::setEndsSmoothingFactor(float endsSmoothingFactor)
{
    if (m_endsSmoothingFactor == endsSmoothingFactor) {
        return;
    }

    m_endsSmoothingFactor = endsSmoothingFactor;
    emit endsSmoothingFactorChanged(m_endsSmoothingFactor);

    bundle();
    update();
}

void LinePlot::setEdgeSampling(float edgeSampling)
{
    if (m_edgeSampling == edgeSampling) {
        return;
    }

    m_edgeSampling = edgeSampling;
    emit edgeSamplingChanged(m_edgeSampling);

    bundle();
    update();
}

void LinePlot::setAdvectionSpeed(float advectionSpeed)
{
    if (m_advectionSpeed == advectionSpeed) {
        return;
    }

    m_advectionSpeed = advectionSpeed;
    emit advectionSpeedChanged(m_advectionSpeed);

    bundle();
    update();
}

void LinePlot::setRelaxation(float relaxation)
{
    if (m_relaxation == relaxation) {
        return;
    }

    m_relaxation = relaxation;
    emit relaxationChanged(m_relaxation);

    bundle();
    update();
}

void LinePlot::setBundleGPU(bool bundleGPU)
{
    if (m_bundleGPU == bundleGPU) {
        return;
    }

    m_bundleGPU = bundleGPU;
    emit bundleGPUChanged(m_bundleGPU);

    bundle();
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

    void copyPolylines(const GraphDrawing *gd);

    QSize m_size;
    std::vector<float> m_points;
    const std::vector<float> *m_values, *m_cmap;
    std::vector<int> m_offsets;

    QQuickWindow *m_window; // used to reset OpenGL state (as per docs)
    QOpenGLFunctions gl;
    QOpenGLShaderProgram *m_program;
    GLuint m_VBOs[2], m_colormapTex;
    QOpenGLVertexArrayObject m_VAO;
    GLfloat m_transform[4][4];
    LinearScale<float> m_sx, m_sy;
    bool m_pointsChanged, m_valuesChanged, m_colormapChanged;
};

QQuickFramebufferObject::Renderer *LinePlot::createRenderer() const
{
    return new LinePlotRenderer;
}

LinePlotRenderer::LinePlotRenderer()
    : gl(QOpenGLContext::currentContext())
    , m_sx(0.0f, 1.0f, 0.0f, 1.0f)
    , m_sy(0.0f, 1.0f, 0.0f, 1.0f)
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
    gl.glDeleteBuffers(2, m_VBOs);
    gl.glDeleteTextures(1, &m_colormapTex);

    delete m_program;
}

QOpenGLFramebufferObject *LinePlotRenderer::createFramebufferObject(const QSize &size)
{
    m_size = size;
    GLfloat w = m_size.width(), h = m_size.height();

    GLfloat rangeOffset = Scatterplot::PADDING / w;
    m_sx.setDomain(0, w);
    m_sx.setRange(rangeOffset, 1.0f - rangeOffset);
    GLfloat sx = 2.0f * m_sx.slope();
    GLfloat tx = 2.0f * m_sx.offset() - 1.0f;

    rangeOffset = Scatterplot::PADDING / h;
    m_sy.setDomain(0, h);
    m_sy.setRange(1.0f - rangeOffset, rangeOffset); // inverted on purpose
    GLfloat sy = 2.0f * m_sy.slope();
    GLfloat ty = 2.0f * m_sy.offset() - 1.0f;

    // The transform matrix should be this (but transposed -- column major):
    // [   sx  0.0f  0.0f    tx ]
    // [ 0.0f    sy  0.0f    ty ]
    // [ 0.0f  0.0f  0.0f  0.0f ]
    // [ 0.0f  0.0f  0.0f  1.0f ]
    m_transform[0][0] = sx;
    m_transform[1][1] = sy;
    m_transform[3][0] = tx;
    m_transform[3][1] = ty;

    return QQuickFramebufferObject::Renderer::createFramebufferObject(m_size);
}

void LinePlotRenderer::render()
{
    if (!m_pointsChanged && !m_valuesChanged && !m_colormapChanged) {
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

    if (m_offsets.size() < 2) {
        // Nothing to draw
        return;
    }

    m_program->bind();

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, m_colormapTex);
    m_program->setUniformValue("colormap", 0);
    m_program->setUniformValue("transform", m_transform);

    gl.glClearColor(0, 0, 0, 0);
    gl.glClear(GL_COLOR_BUFFER_BIT);

    m_VAO.bind();
    gl.glEnable(GL_LINE_SMOOTH);
    gl.glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    gl.glEnable(GL_BLEND);
    gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 1; i < m_offsets.size(); i++) {
        gl.glDrawArrays(GL_LINE_STRIP, m_offsets[i - 1], m_offsets[i] - m_offsets[i - 1]);
    }
    gl.glDisable(GL_LINE_SMOOTH);
    gl.glDisable(GL_BLEND);
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
    m_offsets.reserve(gd->draw_order.size() + 1);
    m_offsets.push_back(0);
    for (auto &p : gd->draw_order) {
        pointsNum += p.second->size();
        m_offsets.push_back(pointsNum);
    }

    m_points.clear();
    m_points.reserve(2 * pointsNum);
    for (auto &p : gd->draw_order) {
        for (auto &point : *p.second) {
            m_points.push_back(point.x);
            m_points.push_back(point.y);
        }
    }
}

void LinePlotRenderer::synchronize(QQuickFramebufferObject *item)
{
    LinePlot *plot = static_cast<LinePlot *>(item);

    m_pointsChanged   = plot->linesChanged();
    m_valuesChanged   = plot->valuesChanged();
    m_colormapChanged = plot->colorScaleChanged();

    if (m_pointsChanged) {
        copyPolylines(plot->bundleGraphDrawing());
    }
    m_values = &(plot->values());
    m_cmap   = &(plot->colorScale());
    m_window = plot->window();

    // Reset so that we have the correct values by the next synchronize()
    plot->setLinesChanged(false);
    plot->setValuesChanged(false);
    plot->setColorScaleChanged(false);
}

void LinePlotRenderer::updatePoints()
{
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[0]);
    gl.glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(float),
            m_points.data(), GL_DYNAMIC_DRAW);

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
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_cmap->size() / 3, 1, 0, GL_RGB,
            GL_FLOAT, m_cmap->data());

    m_colormapChanged = false;
}
