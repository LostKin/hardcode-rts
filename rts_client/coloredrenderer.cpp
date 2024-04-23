#include "coloredrenderer.h"

#include <QFile>
#include <QPainter>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QOpenGLShaderProgram>
#include <math.h>


QSharedPointer<ColoredRenderer> ColoredRenderer::Create ()
{
    QSharedPointer<QOpenGLShaderProgram> program (new QOpenGLShaderProgram ());
    {
        QFile shader_fh (":/gpu_programs/colored/main.vert");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return nullptr;
        QByteArray shader_data = shader_fh.readAll ();
        program->addShaderFromSourceCode (QOpenGLShader::Vertex, shader_data);
    }
    {
        QFile shader_fh (":/gpu_programs/colored/main.frag");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return nullptr;
        QByteArray shader_data = shader_fh.readAll ();
        program->addShaderFromSourceCode (QOpenGLShader::Fragment, shader_data);
    }
    if (!program->link ())
        return nullptr;
    int position_attr_idx = program->attributeLocation ("position_attr");
    int color_attr_idx = program->attributeLocation ("color_attr");
    int matrix_uniform_idx = program->uniformLocation ("ortho_matrix");
    if (position_attr_idx < 0 ||
        color_attr_idx < 0 ||
        matrix_uniform_idx < 0)
        return nullptr;
    return QSharedPointer<ColoredRenderer> (new ColoredRenderer (
        program,
        position_attr_idx,
        color_attr_idx,
        matrix_uniform_idx
    ));
}

ColoredRenderer::ColoredRenderer (QSharedPointer<QOpenGLShaderProgram> program,
                                  GLuint position_attr_idx,
                                  GLuint color_attr_idx,
                                  GLuint matrix_uniform_idx)
    : program (program)
    , position_attr_idx (position_attr_idx)
    , color_attr_idx (color_attr_idx)
    , matrix_uniform_idx (matrix_uniform_idx)
{
}

void ColoredRenderer::draw (QOpenGLFunctions& gl, GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* colors, const QMatrix4x4& ortho_matrix)
{
    gl.glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.glEnable (GL_BLEND);

    program->bind ();
    program->setUniformValue (matrix_uniform_idx, ortho_matrix);

    gl.glVertexAttribPointer (position_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, vertex_positions);
    gl.glVertexAttribPointer (color_attr_idx, 4, GL_FLOAT, GL_FALSE, 0, colors);

    gl.glEnableVertexAttribArray (0);
    gl.glEnableVertexAttribArray (1);

    gl.glDrawArrays (mode, 0, vertex_count);

    gl.glDisableVertexAttribArray (1);
    gl.glDisableVertexAttribArray (0);

    program->release ();

    gl.glDisable (GL_BLEND);
}
void ColoredRenderer::draw (QOpenGLFunctions& gl, GLenum mode, const GLfloat* vertex_positions, const GLfloat* colors, GLsizei index_count, const GLuint* indices, const QMatrix4x4& ortho_matrix)
{
    gl.glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.glEnable (GL_BLEND);

    program->bind ();
    program->setUniformValue (matrix_uniform_idx, ortho_matrix);

    gl.glVertexAttribPointer (position_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, vertex_positions);
    gl.glVertexAttribPointer (color_attr_idx, 4, GL_FLOAT, GL_FALSE, 0, colors);

    gl.glEnableVertexAttribArray (0);
    gl.glEnableVertexAttribArray (1);

    gl.glDrawElements (mode, index_count, GL_UNSIGNED_INT, indices);

    gl.glDisableVertexAttribArray (1);
    gl.glDisableVertexAttribArray (0);

    program->release ();

    gl.glDisable (GL_BLEND);
}
void ColoredRenderer::drawRectangle (QOpenGLFunctions& gl, int x, int y, int w, int h, const QColor& color, const QMatrix4x4& ortho_matrix)
{
    const GLfloat vertices[] = {
        GLfloat (x + 0.5),
        GLfloat (y + 0.5),
        GLfloat (x - 0.5 + w),
        GLfloat (y + 0.5),
        GLfloat (x - 0.5 + w),
        GLfloat (y - 0.5 + h),
        GLfloat (x + 0.5),
        GLfloat (y - 0.5 + h),
    };

    GLfloat r = color.redF ();
    GLfloat g = color.greenF ();
    GLfloat b = color.blueF ();
    GLfloat a = color.alphaF ();

    const GLfloat colors[] = {
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
    };

    draw (gl, GL_LINE_LOOP, 4, vertices, colors, ortho_matrix);
}
void ColoredRenderer::drawRectangle (QOpenGLFunctions& gl, const QRectF& rect, const QColor& color, const QMatrix4x4& ortho_matrix)
{
    qreal x = rect.x ();
    qreal y = rect.y ();
    qreal w = rect.width ();
    qreal h = rect.height ();
    const GLfloat vertices[] = {
        GLfloat (x + 0.5),
        GLfloat (y + 0.5),
        GLfloat (x - 0.5 + w),
        GLfloat (y + 0.5),
        GLfloat (x - 0.5 + w),
        GLfloat (y - 0.5 + h),
        GLfloat (x + 0.5),
        GLfloat (y - 0.5 + h),
    };

    GLfloat r = color.redF ();
    GLfloat g = color.greenF ();
    GLfloat b = color.blueF ();
    GLfloat a = color.alphaF ();

    const GLfloat colors[] = {
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
    };

    draw (gl, GL_LINE_LOOP, 4, vertices, colors, ortho_matrix);
}
void ColoredRenderer::fillRectangle (QOpenGLFunctions& gl, int x, int y, int w, int h, const QColor& color, const QMatrix4x4& ortho_matrix)
{
    const GLfloat vertices[] = {
        GLfloat (x),
        GLfloat (y),
        GLfloat (x + w),
        GLfloat (y),
        GLfloat (x),
        GLfloat (y + h),
        GLfloat (x + w),
        GLfloat (y + h),
    };

    GLfloat r = color.redF ();
    GLfloat g = color.greenF ();
    GLfloat b = color.blueF ();
    GLfloat a = color.alphaF ();

    const GLfloat colors[] = {
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
    };

    draw (gl, GL_TRIANGLE_STRIP, 4, vertices, colors, ortho_matrix);
}
void ColoredRenderer::fillRectangle (QOpenGLFunctions& gl, qreal x, qreal y, qreal w, qreal h, const QColor& color, const QMatrix4x4& ortho_matrix)
{
    const GLfloat vertices[] = {
        GLfloat (x),
        GLfloat (y),
        GLfloat (x + w),
        GLfloat (y),
        GLfloat (x),
        GLfloat (y + h),
        GLfloat (x + w),
        GLfloat (y + h),
    };

    GLfloat r = color.redF ();
    GLfloat g = color.greenF ();
    GLfloat b = color.blueF ();
    GLfloat a = color.alphaF ();

    const GLfloat colors[] = {
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
        r,
        g,
        b,
        a,
    };

    draw (gl, GL_TRIANGLE_STRIP, 4, vertices, colors, ortho_matrix);
}
void ColoredRenderer::fillRectangle (QOpenGLFunctions& gl, const QRect& rect, const QColor& color, const QMatrix4x4& ortho_matrix)
{
    fillRectangle (gl, rect.x (), rect.y (), rect.width (), rect.height (), color, ortho_matrix);
}
void ColoredRenderer::fillRectangle (QOpenGLFunctions& gl, const QRectF& rect, const QColor& color, const QMatrix4x4& ortho_matrix)
{
    fillRectangle (gl, rect.x (), rect.y (), rect.width (), rect.height (), color, ortho_matrix);
}
void ColoredRenderer::drawCircle (QOpenGLFunctions& gl, qreal x, qreal y, qreal radius, const QColor& color, const QMatrix4x4& ortho_matrix)
{
    const GLfloat x_f = x;
    const GLfloat y_f = y;
    const GLfloat r = color.redF ();
    const GLfloat g = color.greenF ();
    const GLfloat b = color.blueF ();
    const GLfloat a = color.alphaF ();

    QVector<GLfloat> vertices;
    QVector<GLfloat> colors;

    size_t count = 128;
    for (size_t i = 0; i < count; ++i) {
        float angle = i * (M_PI * 2.0) / count;
        float dx, dy;
        sincosf (angle, &dy, &dx);
        vertices.append (x_f + radius * dx);
        vertices.append (y_f + radius * dy);
        colors.append (r);
        colors.append (g);
        colors.append (b);
        colors.append (a);
    }

    gl.glEnable (GL_LINE_SMOOTH);
    draw (gl, GL_LINE_LOOP, count, vertices.data (), colors.data (), ortho_matrix);
    gl.glDisable (GL_LINE_SMOOTH);
}
