#include "voronoisplat.h"

#include <algorithm>

#include <QQuickWindow>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include "colormap.h"
#include "scale.h"
#include "scatterplot.h"
#include "skelft.h"

static const float RAD_BLUR = 5.0f;

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
    , m_cmap(3*Colormap::SAMPLES)
{
    setTextureFollowsItemSize(false);
}

void VoronoiSplat::setSites(const arma::mat &points)
{
    if (points.n_rows < 1 || points.n_cols != 2
        || (m_values.size() != 0 && points.n_rows != m_values.size())) {
        return;
    }

    // Copy 'points' to internal data structure(s)
    double minX = points.col(0).min();
    double maxX = points.col(0).max();
    double minY = points.col(1).min();
    double maxY = points.col(1).max();

    // Coords are packed into 'm_sites' as [ x1, y1, x2, y2, ... ]
    m_sites.resize(2*points.n_rows);
    LinearScale<float> sx(minX, maxX, Scatterplot::PADDING, width() - Scatterplot::PADDING);
    const double *col = points.colptr(0);
    for (unsigned i = 0; i < points.n_rows; i++) {
        m_sites[2*i] = sx(col[i]);
    }

    col = points.colptr(1);
    LinearScale<float> sy(minY, maxY, height() - Scatterplot::PADDING, Scatterplot::PADDING);
    for (unsigned i = 0; i < points.n_rows; i++) {
        m_sites[2*i + 1] = sy(col[i]);
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
    scale.sample(Colormap::SAMPLES, m_cmap.begin());
    emit colorScaleChanged(scale);

    setColorScaleChanged(true);
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
    void computeDT();

    QSize m_size;
    const std::vector<float> *m_sites, *m_values, *m_cmap;

    QQuickWindow *m_window; // used to reset OpenGL state (as per docs)
    QOpenGLFunctions gl;
    QOpenGLShaderProgram *m_program1, *m_program2;
    GLuint m_FBO;
    GLuint m_VBOs[3];
    GLuint m_textures[2], m_colormapTex;
    QOpenGLVertexArrayObject m_sitesVAO, m_2ndPassVAO;
    bool m_sitesChanged, m_valuesChanged, m_colorScaleChanged;
};

QQuickFramebufferObject::Renderer *VoronoiSplat::createRenderer() const
{
    return new VoronoiSplatRenderer;
}

VoronoiSplatRenderer::VoronoiSplatRenderer()
    : gl(QOpenGLContext::currentContext())
{
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
uniform float fb_size;

in vec2 vert;
in float scalar;

out float value;

void main() {
  gl_PointSize = (rad_max + rad_blur) * 2.0;
  gl_Position = vec4(2.0 * vert / fb_size - 1.0, 0, 1.0);
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
    float r = distance(gl_PointCoord, vec2(0.5, 0.5)) * (rad_max + rad_blur) * 2.0;
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
  gl_Position = vec4(vert, 0, 1);
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
  return texture(colorScale, vec2(value, 0)).rgb;
}

void main() {
  float dt = texelFetch(siteDT, ivec2(gl_FragCoord.xy), 0).r;
  if (dt > rad_max)
    discard;
  else {
    vec4 accum = texelFetch(accumTex, ivec2(gl_FragCoord.xy), 0);
    float value = (accum.g == 0.0) ? 0.0 : accum.r / accum.g;
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
    GLfloat verts[] = {-1.0f, -1.0f, -1.0f, 1.0f,
                        1.0f, -1.0f,  1.0f, 1.0f};
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
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Colormap::SAMPLES, 1, 0, GL_RGB,
            GL_FLOAT, 0);
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
    if (m_colorScaleChanged) {
        updateColorScale();
    }

    int originalFBO;
    gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &originalFBO);

    gl.glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    // First pass
    m_program1->bind();
    m_program1->setUniformValue("rad_max", 20.0f);
    m_program1->setUniformValue("rad_blur", RAD_BLUR);
    m_program1->setUniformValue("fb_size", float(m_size.width()));

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    m_program1->setUniformValue("siteDT", 0);

    gl.glEnable(GL_POINT_SPRITE);
    gl.glEnable(GL_PROGRAM_POINT_SIZE);
    gl.glEnable(GL_BLEND);
    gl.glBlendFunc(GL_ONE, GL_ZERO);

    // First, we draw to an intermediate texture, which is used as input for the
    // second pass
    gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_textures[1], 0);

    gl.glClearColor(0, 0, 0, 1);
    gl.glClear(GL_COLOR_BUFFER_BIT);

    m_sitesVAO.bind();
    gl.glDrawArrays(GL_POINTS, 0, m_values->size());
    m_sitesVAO.release();

    m_program1->release();

    // For some reason this call makes the splat circle of the correct size
    //m_window->resetOpenGLState();

    // Second pass
    m_program2->bind();
    m_program2->setUniformValue("rad_max", 20.0f);

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
    m_colorScaleChanged = splat->colorScaleChanged();

    // Reset these so that by the next synchronize() we have the correct values
    splat->setSitesChanged(false);
    splat->setValuesChanged(false);
    splat->setColorScaleChanged(false);

    m_sites           = &(splat->sites());
    m_values          = &(splat->values());
    m_cmap            = &(splat->colorScale());
    m_window          = splat->window();
}

void VoronoiSplatRenderer::updateSites()
{
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[0]);
    gl.glBufferData(GL_ARRAY_BUFFER, m_sites->size() * sizeof(float),
            m_sites->data(), GL_DYNAMIC_DRAW);

    // Compute DT values for the new positions
    computeDT();

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
    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, m_colormapTex);
    //gl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Colormap::SAMPLES, 1, GL_RGB,
    //        GL_FLOAT, m_cmap->data());
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Colormap::SAMPLES, 1, 0, GL_RGB,
            GL_FLOAT, m_cmap->data());

    m_colorScaleChanged = false;
}

void VoronoiSplatRenderer::computeDT()
{
    int w = m_size.width(), h = m_size.height();

    // Compute FT of the sites
    std::vector<float> buf(w*h);
    const std::vector<float> &sites = *m_sites;
    for (unsigned i = 0; i < sites.size(); i += 2) {
        buf[int(sites[i + 1])*h + int(sites[i])] = i/2.0f + 1.0f;
    }
    skelft2DFT(0, buf.data(), 0, 0, w, h, w);

    // Compute DT of the sites (from the resident FT)
    skelft2DDT(buf.data(), 0, 0, w, h);

    // Upload result to lookup texture
    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    //gl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_FLOAT,
    //        buf.data());
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RED, GL_FLOAT,
            buf.data());
}
