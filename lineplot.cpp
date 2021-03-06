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

static const float EPSILON = 1e-5f;
static const int SAMPLES = 128;

LinePlot::LinePlot(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_sx(0, 1, 0, 1)
    , m_sy(0, 1, 0, 1)
    , m_brushedItem(-1)
    , m_anySelected(false)
    , m_linesChanged(false)
    , m_valuesChanged(false)
    , m_colorScaleChanged(false)
    , m_updateOffsets(false)

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
    , m_lineWidth(2.0)
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

void LinePlot::relax()
{
    m_gdFinal = m_gdBundle;
    m_gdFinal.interpolate(*m_gdPtr.get(), m_relaxation);

    setLinesChanged(true);
}

void LinePlot::buildGraph()
{
    Graph g(m_Y.n_rows);
    PointSet points;

    m_sx.setRange(0, width());
    m_sy.setRange(0, height());
    for (arma::uword i = 0; i < m_Y.n_rows; i++) {
        points.push_back(Point2d(m_sx(m_Y(i, 0)), m_sy(m_Y(i, 1))));
    }

    for (arma::uword k = 0; k < m_indices.n_elem; k += 2) {
        arma::uword i = m_indices[k + 0],
                    j = m_indices[k + 1];
        g(i, j) = g(j, i) = m_values[k];
    }

    m_gdPtr.reset(new GraphDrawing);
    m_gdPtr.get()->build(&g, &points);
}

void LinePlot::bundle()
{
    m_gdBundle = *m_gdPtr.get();

    CPUBundling bundling(std::min(width(), height()));
    bundling.setInput(&m_gdBundle);

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

    if (m_bundleGPU) {
        bundling.bundleGPU();
    } else {
        bundling.bundleCPU();
    }
    m_updateOffsets = true;

    relax();
}

void LinePlot::setLines(const arma::uvec &indices, const arma::mat &Y)
{
    if (indices.n_elem % 2 != 0 || Y.n_cols != 2) {
        return;
    }

    m_indices = indices;
    emit indicesChanged(m_indices);

    m_Y = Y;
    emit pointsChanged(m_Y);

    // Clear current selection
    m_anySelected = false;
    m_selection.assign(m_Y.n_rows, false);
    emit selectionChanged(m_selection);

    // Build the line plot's internal representation: a graph where each
    // endpoint of a line is a node, each line is a link...
    buildGraph();

    // ... then bundle the edges
    bundle();

    update();
}

void LinePlot::setValues(const arma::vec &values)
{
    m_values.resize(values.n_elem);
    std::transform(values.begin(), values.end(), m_values.begin(), [](float v) {
        return std::max(v, EPSILON);
    });
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

void LinePlot::brushItem(int item)
{
    m_brushedItem = item;
    m_updateOffsets = true;
    update();
}

void LinePlot::setSelection(const std::vector<bool> &selection)
{
    m_selection = selection;
    m_anySelected = std::any_of(m_selection.cbegin(),
                                m_selection.cend(),
                                [](bool b) { return b; });
    emit selectionChanged(m_selection);

    // XXX: *possibly* needed; doesn't seem to make much of a difference
    //bundle();
    m_updateOffsets = true;
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

    relax();
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

void LinePlot::setLineWidth(float lineWidth)
{
    if (m_lineWidth == lineWidth) {
        return;
    }

    m_lineWidth = lineWidth;
    emit lineWidthChanged(m_lineWidth);

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

    void copyPolylines(const LinePlot *plot);
    void computeOffsets(const LinePlot *plot);
    void computeBrushOffsets(const LinePlot *plot);
    void computeAllOffsets(const LinePlot *plot);

    QSize m_size;
    std::vector<float> m_points;
    const std::vector<float> *m_values, *m_cmap;
    float m_lineWidth;
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
    , m_pointsChanged(false)
    , m_valuesChanged(false)
    , m_colormapChanged(false)
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
    gl.glLineWidth(m_lineWidth);
    for (int i = 0; i < m_offsets.size(); i += 2) {
        gl.glDrawArrays(GL_LINE_STRIP, m_offsets[i], m_offsets[i + 1]);
    }
    gl.glDisable(GL_LINE_SMOOTH);
    gl.glDisable(GL_BLEND);
    m_VAO.release();

    m_program->release();
    m_window->resetOpenGLState();
}

void LinePlotRenderer::synchronize(QQuickFramebufferObject *item)
{
    LinePlot *plot = static_cast<LinePlot *>(item);

    m_pointsChanged   = plot->m_linesChanged;
    m_valuesChanged   = plot->m_valuesChanged;
    m_colormapChanged = plot->m_colorScaleChanged;

    m_values    = &(plot->values());
    m_cmap      = &(plot->colorScale());
    m_lineWidth = plot->m_lineWidth;
    m_window    = plot->window();

    if (plot->m_updateOffsets) {
        plot->m_updateOffsets = false;
        computeOffsets(plot);
    }

    if (m_pointsChanged) {
        copyPolylines(plot);
    }

    plot->m_linesChanged      = false;
    plot->m_valuesChanged     = false;
    plot->m_colorScaleChanged = false;
}

void LinePlotRenderer::computeOffsets(const LinePlot *plot)
{
    if (plot->m_brushedItem >= 0) {
        computeBrushOffsets(plot);
    } else {
        computeAllOffsets(plot);
    }
}

void LinePlotRenderer::computeBrushOffsets(const LinePlot *plot)
{
    int offset = 0;
    m_offsets.clear();
    for (auto &p : plot->graphDrawing()->draw_order) {
        int pointsNum = p.second->size();
        //int item1 = plot->m_indices[2*i];
        //int item2 = plot->m_indices[2*i + 1];
        //if (item1 == plot->m_brushedItem
        //    || item2 == plot->m_brushedItem) {
            m_offsets.push_back(offset);
            m_offsets.push_back(pointsNum);
        //}

        offset += pointsNum;
    }
}

void LinePlotRenderer::computeAllOffsets(const LinePlot *plot)
{
    int offset = 0;
    m_offsets.clear();
    m_offsets.reserve(2 * plot->graphDrawing()->draw_order.size());
    for (auto &p : plot->graphDrawing()->draw_order) {
        int pointsNum = p.second->size();
        m_offsets.push_back(offset);
        m_offsets.push_back(pointsNum);

        offset += pointsNum;
    }
}

void LinePlotRenderer::copyPolylines(const LinePlot *plot)
{
    const GraphDrawing *gd = plot->graphDrawing();
    if (!gd || gd->draw_order.empty()) {
        return;
    }

    m_points.clear();
    //m_points.reserve(2 * totalPoints);
    for (auto &p : gd->draw_order) {
        for (auto &point : *p.second) {
            m_points.push_back(point.x);
            m_points.push_back(point.y);
        }
    }
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
