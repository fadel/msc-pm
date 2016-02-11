#include "voronoisplat.h"

#include <algorithm>

#include <QQuickWindow>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include "colormap.h"
#include "scatterplot.h"
#include "skelft.h"

static const float DEFAULT_ALPHA = 5.0f;
static const float DEFAULT_BETA = 20.0f;

static int nextPow2(int n)
{
    // TODO: check for overflows
    n--;
    for (int shift = 1; ((n + 1) & n); shift <<= 1) {
        n |= n >> shift;
    }
    return n + 1;
}

VoronoiSplat::VoronoiSplat(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_sx(0.0f, 1.0f, 0.0f, 1.0f)
    , m_sy(0.0f, 1.0f, 0.0f, 1.0f)
    , m_alpha(DEFAULT_ALPHA)
    , m_beta(DEFAULT_BETA)
{
    setTextureFollowsItemSize(false);
}

void VoronoiSplat::setSites(const arma::mat &points)
{
    if (points.n_rows < 1 || points.n_cols != 2) {
        return;
    }

    if (m_values.size() > 0 && m_values.size() != points.n_rows) {
        // Old values are no longer valid, clean up
        m_values.resize(points.n_rows);
        m_values.assign(points.n_rows, 0);
    }

    // Copy 'points' to internal data structure(s)
    // Coords are packed into 'm_sites' as [ x1, y1, x2, y2, ... ]
    m_sites.resize(2*points.n_rows);
    const double *col = points.colptr(0);
    for (unsigned i = 0; i < points.n_rows; i++) {
        m_sites[2*i] = col[i];
    }

    col = points.colptr(1);
    for (unsigned i = 0; i < points.n_rows; i++) {
        m_sites[2*i + 1] = col[i];
    }

    setSitesChanged(true);
    update();
}

void VoronoiSplat::setValues(const arma::vec &values)
{
    if (values.n_elem == 0
        || (m_sites.size() != 0 && values.n_elem != m_sites.size() / 2)) {
        return;
    }

    m_values.resize(values.n_elem);
    LinearScale<float> scale(values.min(), values.max(), 0, 1.0f);
    std::transform(values.begin(), values.end(), m_values.begin(), scale);
    emit valuesChanged(values);

    setValuesChanged(true);
    update();
}

void VoronoiSplat::setColorScale(const ColorScale &scale)
{
    m_cmap.resize(scale.numColors() * 3);
    scale.sample(scale.numColors(), m_cmap.begin());
    emit colorScaleChanged(scale);

    setColormapChanged(true);
    update();
}

void VoronoiSplat::setScale(const LinearScale<float> &sx,
                            const LinearScale<float> &sy)
{
    m_sx = sx;
    m_sy = sy;
    emit scaleChanged(m_sx, m_sy);
    update();
}

void VoronoiSplat::setAlpha(float alpha)
{
    m_alpha = alpha;
    emit alphaChanged(m_alpha);
    update();
}

void VoronoiSplat::setBeta(float beta)
{
    m_beta = beta;
    emit betaChanged(m_beta);
    update();
}

// ----------------------------------------------------------------------------

class VoronoiSplatRenderer
    : public QQuickFramebufferObject::Renderer
{
public:
    // 'size' must be square (and power of 2)
    VoronoiSplatRenderer();
    virtual ~VoronoiSplatRenderer();

protected:
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size);
    void render();
    void synchronize(QQuickFramebufferObject *item);

private:
    void setupShaders();
    void setupVAOs();
    void setupTextures();
    void resizeTextures();

    void updateSites();
    void updateValues();
    void updateColorScale();
    void updateTransform();
    void computeDT();

    QSize m_size;
    const std::vector<float> *m_sites, *m_values, *m_cmap;
    float m_alpha, m_beta;
    GLfloat m_transform[4][4];
    LinearScale<float> m_sx, m_sy;

    QQuickWindow *m_window; // used to reset OpenGL state (as per docs)
    QOpenGLFunctions gl;
    QOpenGLShaderProgram *m_program1, *m_program2;
    GLuint m_FBO;
    GLuint m_VBOs[3];
    GLuint m_textures[2], m_colormapTex;
    QOpenGLVertexArrayObject m_sitesVAO, m_2ndPassVAO;
    bool m_sitesChanged, m_valuesChanged, m_colormapChanged;
};

QQuickFramebufferObject::Renderer *VoronoiSplat::createRenderer() const
{
    return new VoronoiSplatRenderer;
}

VoronoiSplatRenderer::VoronoiSplatRenderer()
    : m_sx(0.0f, 1.0f, 0.0f, 1.0f)
    , m_sy(0.0f, 1.0f, 0.0f, 1.0f)
    , gl(QOpenGLContext::currentContext())
{
    std::fill(&m_transform[0][0], &m_transform[0][0] + 16, 0.0f);
    m_transform[3][3] = 1.0f;

    gl.glGenFramebuffers(1, &m_FBO);

    setupShaders();
    setupVAOs();
    setupTextures();
}

void VoronoiSplatRenderer::setupShaders()
{
    m_program1 = new QOpenGLShaderProgram;
    m_program1->addShaderFromSourceCode(QOpenGLShader::Vertex,
R"EOF(#version 440

uniform float rad_blur;
uniform float rad_max;
uniform mat4 transform;

in vec2 vert;
in float scalar;

out float value;

void main() {
  gl_PointSize = 2.0 * (rad_max + rad_blur);
  gl_Position = transform * vec4(vert, 0.0, 1.0);
  value = scalar;
}
)EOF");
    m_program1->addShaderFromSourceCode(QOpenGLShader::Fragment,
R"EOF(#version 440

uniform float rad_blur;
uniform float rad_max;
uniform sampler2D siteDT;

in float value;

layout (location = 0) out vec4 fragColor;

void main() {
  float dt = texelFetch(siteDT, ivec2(gl_FragCoord.xy), 0).r;
  if (dt > rad_max)
    discard;
  else {
    float r = 2.0 * distance(gl_PointCoord, vec2(0.5, 0.5)) * (rad_max + rad_blur);
    float r2 = r * r;
    float rad = dt + rad_blur;
    float rad2 = rad * rad;
    if (r2 > rad2)
      discard;
    else {
      float w = exp(-5.0 * r2 / rad2);
      fragColor = vec4(w * value, w, 0.0, 0.0);
    }
  }
}
)EOF");
    m_program1->link();

    m_program2 = new QOpenGLShaderProgram;
    m_program2->addShaderFromSourceCode(QOpenGLShader::Vertex,
R"EOF(#version 440

in vec2 vert;

void main() {
  gl_Position = vec4(vert, 0.0, 1.0);
}
)EOF");
    m_program2->addShaderFromSourceCode(QOpenGLShader::Fragment,
R"EOF(
#version 440

uniform sampler2D siteDT;
uniform sampler2D accumTex;
uniform sampler2D colorScale;
uniform float rad_max;

layout (location = 0) out vec4 fragColor;

vec3 getRGB(float value) {
  return texture(colorScale, vec2(mix(0.005, 0.995, value), 0)).rgb;
}

void main() {
  float dt = texelFetch(siteDT, ivec2(gl_FragCoord.xy), 0).r;
  if (dt > rad_max)
    discard;
  else {
    vec4 accum = texelFetch(accumTex, ivec2(gl_FragCoord.xy), 0);
    float value = (accum.g > 1.0) ? (accum.r - 1.0) / (accum.g - 1.0) : 0.0;
    fragColor = vec4(getRGB(value), 1.0 - dt / rad_max);
  }
}
)EOF");
    m_program2->link();
}

void VoronoiSplatRenderer::setupVAOs()
{
    gl.glGenBuffers(3, m_VBOs);

    // sitesVAO: VBOs 0 & 1 are for sites & their values (init'd later)
    m_sitesVAO.create();
    m_sitesVAO.bind();
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[0]);
    int vertAttrib = m_program1->attributeLocation("vert");
    gl.glVertexAttribPointer(vertAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    gl.glEnableVertexAttribArray(vertAttrib);

    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[1]);
    int valueAttrib = m_program1->attributeLocation("scalar");
    gl.glVertexAttribPointer(valueAttrib, 1, GL_FLOAT, GL_FALSE, 0, 0);
    gl.glEnableVertexAttribArray(valueAttrib);
    m_sitesVAO.release();

    // 2ndPassVAO: VBO 2 is a quad mapping the final texture to the framebuffer
    m_2ndPassVAO.create();
    m_2ndPassVAO.bind();
    GLfloat verts[] = { -1.0f, -1.0f, -1.0f,  1.0f,
                         1.0f, -1.0f,  1.0f,  1.0f };
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[2]);
    gl.glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    vertAttrib = m_program2->attributeLocation("vert");
    gl.glVertexAttribPointer(vertAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    gl.glEnableVertexAttribArray(vertAttrib);
    m_2ndPassVAO.release();
}

void VoronoiSplatRenderer::setupTextures()
{
    gl.glGenTextures(2, m_textures);

    // Used for colorScale lookup in the frag shader
    // (2D texture for compatibility; used to be a 1D texture)
    gl.glGenTextures(1, &m_colormapTex);
    gl.glBindTexture(GL_TEXTURE_2D, m_colormapTex);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

VoronoiSplatRenderer::~VoronoiSplatRenderer()
{
    gl.glDeleteBuffers(3, m_VBOs);
    gl.glDeleteTextures(2, m_textures);
    gl.glDeleteTextures(1, &m_colormapTex);

    gl.glDeleteFramebuffers(1, &m_FBO);

    delete m_program1;
    delete m_program2;

    skelft2DDeinitialization();
}

void VoronoiSplatRenderer::resizeTextures()
{
    // textures[0] stores the DT values for each pixel
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_size.width(),
            m_size.height(), 0, GL_RED, GL_FLOAT, 0);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // textures[1] is the result of the first pass
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_size.width(),
            m_size.height(), 0, GL_RGBA, GL_FLOAT, 0);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

QOpenGLFramebufferObject *VoronoiSplatRenderer::createFramebufferObject(const QSize &size)
{
    int baseSize = nextPow2(std::min(size.width(), size.height()));
    m_size.setWidth(baseSize);
    m_size.setHeight(baseSize);
    resizeTextures();

    skelft2DInitialization(m_size.width());

    return QQuickFramebufferObject::Renderer::createFramebufferObject(m_size);
}

void VoronoiSplatRenderer::render()
{
    // Update OpenGL buffers and textures as needed
    if (m_sitesChanged) {
        updateSites();
    }
    if (m_valuesChanged) {
        updateValues();
    }
    if (m_colormapChanged) {
        updateColorScale();
    }

    int originalFBO;
    gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &originalFBO);

    gl.glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    // First pass
    m_program1->bind();
    m_program1->setUniformValue("rad_max", m_beta);
    m_program1->setUniformValue("rad_blur", m_alpha);
    m_program1->setUniformValue("transform", m_transform);

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    m_program1->setUniformValue("siteDT", 0);

    gl.glEnable(GL_POINT_SPRITE);
    gl.glEnable(GL_PROGRAM_POINT_SIZE);
    gl.glEnable(GL_BLEND);
    gl.glBlendFunc(GL_ONE, GL_ONE);

    // First, we draw to an intermediate texture, which is used as input for the
    // second pass
    gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_textures[1], 0);

    gl.glClearColor(1, 1, 1, 1);
    gl.glClear(GL_COLOR_BUFFER_BIT);

    m_sitesVAO.bind();
    gl.glDrawArrays(GL_POINTS, 0, m_values->size());
    m_sitesVAO.release();

    m_program1->release();

    // For some reason this call makes the splat circle of the correct size
    //m_window->resetOpenGLState();

    // Second pass
    m_program2->bind();
    m_program2->setUniformValue("rad_max", m_beta);

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    m_program2->setUniformValue("siteDT", 0);
    gl.glActiveTexture(GL_TEXTURE1);
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    m_program2->setUniformValue("accumTex", 1);
    gl.glActiveTexture(GL_TEXTURE2);
    gl.glBindTexture(GL_TEXTURE_2D, m_colormapTex);
    m_program2->setUniformValue("colorScale", 2);

    gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // We now render to the QQuickFramebufferObject's FBO
    gl.glBindFramebuffer(GL_FRAMEBUFFER, originalFBO);

    gl.glClearColor(0, 0, 0, 0);
    gl.glClear(GL_COLOR_BUFFER_BIT);

    m_2ndPassVAO.bind();
    gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_2ndPassVAO.release();

    m_program2->release();

    m_window->resetOpenGLState();
}

void VoronoiSplatRenderer::synchronize(QQuickFramebufferObject *item)
{
    VoronoiSplat *splat = static_cast<VoronoiSplat *>(item);

    m_sitesChanged    = splat->sitesChanged();
    m_valuesChanged   = splat->valuesChanged();
    m_colormapChanged = splat->colormapChanged();

    // Reset so that we have the correct values by the next synchronize()
    splat->setSitesChanged(false);
    splat->setValuesChanged(false);
    splat->setColormapChanged(false);

    m_sites  = &(splat->sites());
    m_values = &(splat->values());
    m_cmap   = &(splat->colorScale());
    m_sx     = splat->scaleX();
    m_sy     = splat->scaleY();
    m_alpha  = splat->alpha();
    m_beta   = splat->beta();
    m_window = splat->window();
}

void VoronoiSplatRenderer::updateTransform()
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

void VoronoiSplatRenderer::updateSites()
{
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[0]);
    gl.glBufferData(GL_ARRAY_BUFFER, m_sites->size() * sizeof(float),
            m_sites->data(), GL_DYNAMIC_DRAW);

    // Compute DT values for the new positions
    computeDT();

    // Update transform used when drawing sites
    updateTransform();

    m_sitesChanged = false;
}

void VoronoiSplatRenderer::updateValues()
{
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[1]);
    gl.glBufferData(GL_ARRAY_BUFFER, m_values->size() * sizeof(float),
            m_values->data(), GL_DYNAMIC_DRAW);

    m_valuesChanged = false;
}

void VoronoiSplatRenderer::updateColorScale()
{
    gl.glBindTexture(GL_TEXTURE_2D, m_colormapTex);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_cmap->size() / 3, 1, 0, GL_RGB,
            GL_FLOAT, m_cmap->data());

    m_colormapChanged = false;
}

void VoronoiSplatRenderer::computeDT()
{
    int w = m_size.width(), h = m_size.height();

    // Compute FT of the sites
    m_sx.setRange(Scatterplot::PADDING, w - Scatterplot::PADDING);
    m_sy.setRange(h - Scatterplot::PADDING, Scatterplot::PADDING);
    const std::vector<float> &sites = *m_sites;
    std::vector<float> buf(w*h);
    for (unsigned i = 0; i < sites.size(); i += 2) {
        unsigned bufIndex = unsigned(m_sy(sites[i + 1]))*h + unsigned(m_sx(sites[i]));
        if (bufIndex > buf.size()) {
            // points outside our scale
            continue;
        }

        buf[bufIndex] = i/2.0f + 1.0f;
    }
    skelft2DFT(0, buf.data(), 0, 0, w, h, w);

    // Compute DT of the sites (from the resident FT)
    skelft2DDT(buf.data(), 0, 0, w, h);

    // Upload result to lookup texture
    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    gl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_FLOAT,
            buf.data());
}
