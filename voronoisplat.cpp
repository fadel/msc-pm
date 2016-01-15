#include "voronoisplat.h"

#include <algorithm>

#include <QQuickWindow>
#include <QOpenGLFramebufferObject>

#include "scale.h"
#include "skelft.h"

static const float RAD_BLUR = 5.0f;
static const int COLORMAP_SAMPLES = 128;
static const float PADDING = 10.0f; // in screen pixels

static int nextPow2(int n)
{
    // TODO: check for overflows
    n--;
    for (int shift = 1; ((n + 1) & n); shift <<= 1) {
        n |= n >> shift;
    }
    return n + 1;
}

VoronoiSplatTexture::VoronoiSplatTexture(const QSize &size)
    : gl(QOpenGLContext::currentContext())
    , m_cmap(3*COLORMAP_SAMPLES)
{
    int baseSize = nextPow2(std::min(size.width(), size.height()));
    m_size.setWidth(baseSize);
    m_size.setHeight(baseSize);

    gl.glGenFramebuffers(1, &m_FBO);
    setupShaders();
    setupVAOs();
    setupTextures();
}

void VoronoiSplatTexture::setupShaders()
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
uniform sampler2D colormap;
uniform float rad_max;

layout (location = 0) out vec4 fragColor;

vec3 getRGB(float value) {
  return texture(colormap, vec2(mix(0.005, 0.995, value), 0)).rgb;
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

void VoronoiSplatTexture::setupVAOs()
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

void VoronoiSplatTexture::setupTextures()
{
    gl.glGenTextures(2, m_textures);

    // textures[0] stores the DT values for each pixel
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_size.width(),
            m_size.height(), 0, GL_RG, GL_FLOAT, 0);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // textures[1] is the result of the first pass
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_size.width(),
            m_size.height(), 0, GL_RGBA, GL_FLOAT, 0);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Used for colormap lookup in the frag shader
    // (using a 2D texture for compatibility; used to be a 1D texture)
    gl.glGenTextures(1, &m_colormapTex);
    gl.glBindTexture(GL_TEXTURE_2D, m_colormapTex);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, COLORMAP_SAMPLES, 1, 0, GL_RGB,
            GL_FLOAT, 0);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // The output texture
    gl.glGenTextures(1, &m_tex);
    gl.glBindTexture(GL_TEXTURE_2D, m_tex);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_size.width(),
            m_size.height(), 0, GL_RGBA, GL_FLOAT, 0);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

VoronoiSplatTexture::~VoronoiSplatTexture()
{
    gl.glDeleteBuffers(3, m_VBOs);
    gl.glDeleteTextures(2, m_textures);
    gl.glDeleteTextures(1, &m_colormapTex);
    gl.glDeleteTextures(1, &m_tex);

    gl.glDeleteFramebuffers(1, &m_FBO);

    delete m_program1;
    delete m_program2;
}

void VoronoiSplatTexture::bind()
{
    gl.glBindTexture(GL_TEXTURE_2D, m_tex);
}

bool VoronoiSplatTexture::updateTexture()
{
    if (!m_sitesChanged && !m_valuesChanged && !m_colormapChanged) {
        // Texture is already updated
        return false;
    }

    // Update OpenGL buffers and textures as needed
    if (m_sitesChanged) {
        updateSites();
    }
    if (m_valuesChanged) {
        updateValues();
    }
    if (m_colormapChanged) {
        updateColormap();
    }

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
    gl.glDrawArrays(GL_POINTS, 0, m_values.size());
    m_sitesVAO.release();

    m_program1->release();

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
    m_program2->setUniformValue("colormap", 2);

    gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // We now render to the output texture
    gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_tex, 0);

    gl.glClearColor(0, 0, 0, 0);
    gl.glClear(GL_COLOR_BUFFER_BIT);

    m_2ndPassVAO.bind();
    gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_2ndPassVAO.release();

    m_program2->release();

    return true;
}

void VoronoiSplatTexture::updateSites()
{
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[0]);
    gl.glBufferData(GL_ARRAY_BUFFER, m_sites.size() * sizeof(float),
            m_sites.data(), GL_DYNAMIC_DRAW);

    // Compute DT values for the new positions
    computeDT();

    m_sitesChanged = false;
}

void VoronoiSplatTexture::updateValues()
{
    gl.glBindBuffer(GL_ARRAY_BUFFER, m_VBOs[1]);
    gl.glBufferData(GL_ARRAY_BUFFER, m_values.size() * sizeof(float),
            m_values.data(), GL_DYNAMIC_DRAW);

    m_valuesChanged = false;
}

void VoronoiSplatTexture::updateColormap()
{
    gl.glBindTexture(GL_TEXTURE_2D, m_colormapTex);
    gl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, COLORMAP_SAMPLES, 1, GL_RGB,
            GL_FLOAT, m_cmap.data());

    m_colormapChanged = false;
}

void VoronoiSplatTexture::computeDT()
{
    int w = m_size.width(), h = m_size.height();

    // Compute FT of the sites
    std::vector<float> buf(w*h);
    for (unsigned i = 0; i < m_sites.size(); i += 2) {
        buf[int(m_sites[i + 1])*h + int(m_sites[i])] = (float) i/2 + 1;
    }
    skelft2DFT(0, buf.data(), 0, 0, w, h, w);

    // Compute DT of the sites (from the resident FT)
    skelft2DDT(buf.data(), 0, 0, w, h);

    std::vector<GLfloat> dtTexData(2*w*h, 0.0f);
    for (unsigned i = 0; i < buf.size(); i++) {
        dtTexData[2*i] = buf[i];
    }

    // Upload result to lookup texture
    gl.glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    gl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RG, GL_FLOAT,
            dtTexData.data());
}

void VoronoiSplatTexture::setSites(const arma::mat &points)
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
    LinearScale<float> sx(minX, maxX, PADDING, m_size.width() - PADDING);
    const double *col = points.colptr(0);
    for (unsigned i = 0; i < points.n_rows; i++) {
        m_sites[2*i] = sx(col[i]);
    }

    col = points.colptr(1);
    LinearScale<float> sy(minY, maxY, m_size.height() - PADDING, PADDING);
    for (unsigned i = 0; i < points.n_rows; i++) {
        m_sites[2*i + 1] = sy(col[i]);
    }

    m_sitesChanged = true;
}

void VoronoiSplatTexture::setValues(const arma::vec &values)
{
    if (values.n_elem == 0
        || (m_sites.size() != 0 && values.n_elem != m_sites.size() / 2)) {
        return;
    }

    m_values.resize(values.n_elem);
    LinearScale<float> scale(values.min(), values.max(), 0, 1.0f);
    std::transform(values.begin(), values.end(), m_values.begin(), scale);

    m_valuesChanged = true;
}

void VoronoiSplatTexture::setColormap(const ColorScale *scale)
{
    if (!scale) {
        return;
    }

    float t = scale->min();
    float step = (scale->max() - scale->min()) / COLORMAP_SAMPLES;
    qreal r, g, b;
    for (unsigned i = 0; i < m_cmap.size(); i += 3) {
        scale->color(t).getRgbF(&r, &g, &b);
        m_cmap[i + 0] = r;
        m_cmap[i + 1] = g;
        m_cmap[i + 2] = b;

        t += step;
    }

    m_colormapChanged = true;
}
