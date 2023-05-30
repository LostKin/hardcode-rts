#include "openglwidget.h"

#include <QFile>
#include <QPainter>
#include <math.h>


OpenGLWidget::OpenGLWidget (QWidget* parent)
    : QOpenGLWidget (parent)
{
    setMinimumSize (512, 512);
    setUpdateBehavior (QOpenGLWidget::NoPartialUpdate);
    connect (this, SIGNAL (frameSwapped()), this, SLOT (update ()));
}
OpenGLWidget::~OpenGLWidget ()
{
}

void OpenGLWidget::initializeGL ()
{
    initializeOpenGLFunctions ();

    glClearColor (0, 0, 0, 1);

    initPrograms ();

    initResources ();
}
void OpenGLWidget::initPrograms ()
{
    initColoredProgram ();
    initColoredTexturedProgram ();
    initTexturedProgram ();
}
void OpenGLWidget::initColoredProgram ()
{
    colored_program.program = new QOpenGLShaderProgram (this);
    {
        QFile shader_fh (":/gpu_programs/colored/main.vert");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll ();
        colored_program.program->addShaderFromSourceCode (QOpenGLShader::Vertex, shader_data);
    }
    {
        QFile shader_fh (":/gpu_programs/colored/main.frag");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll ();
        colored_program.program->addShaderFromSourceCode (QOpenGLShader::Fragment, shader_data);
    }
    colored_program.program->link ();
    colored_program.position_attr_idx = colored_program.program->attributeLocation ("position_attr");
    colored_program.color_attr_idx = colored_program.program->attributeLocation ("color_attr");
    colored_program.matrix_uniform_idx = colored_program.program->uniformLocation ("ortho_matrix");
}
void OpenGLWidget::initColoredTexturedProgram ()
{
    colored_textured_program.program = new QOpenGLShaderProgram (this);
    {
        QFile shader_fh (":/gpu_programs/colored_textured/main.vert");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll ();
        colored_textured_program.program->addShaderFromSourceCode (QOpenGLShader::Vertex, shader_data);
    }
    {
        QFile shader_fh (":/gpu_programs/colored_textured/main.frag");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll ();
        colored_textured_program.program->addShaderFromSourceCode (QOpenGLShader::Fragment, shader_data);
    }
    colored_textured_program.program->link ();
    colored_textured_program.position_attr_idx = colored_textured_program.program->attributeLocation ("position_attr");
    colored_textured_program.color_attr_idx = colored_textured_program.program->attributeLocation ("color_attr");
    colored_textured_program.texture_coords_attr_idx = colored_textured_program.program->attributeLocation ("texture_coords_attr");
    colored_textured_program.matrix_uniform_idx = colored_textured_program.program->uniformLocation ("ortho_matrix");
    colored_textured_program.texture_mode_rect_uniform_idx = colored_textured_program.program->uniformLocation ("texture_mode_rect");
    colored_textured_program.texture_2d_uniform_idx = colored_textured_program.program->uniformLocation ("texture_2d");
    colored_textured_program.texture_2d_rect_uniform_idx = colored_textured_program.program->uniformLocation ("texture_2d_rect");
}
void OpenGLWidget::initTexturedProgram ()
{
    textured_program.program = new QOpenGLShaderProgram (this);
    {
        QFile shader_fh (":/gpu_programs/textured/main.vert");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll ();
        textured_program.program->addShaderFromSourceCode (QOpenGLShader::Vertex, shader_data);
    }
    {
        QFile shader_fh (":/gpu_programs/textured/main.frag");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll ();
        textured_program.program->addShaderFromSourceCode (QOpenGLShader::Fragment, shader_data);
    }
    textured_program.program->link ();
    textured_program.position_attr_idx = textured_program.program->attributeLocation ("position_attr");
    textured_program.texture_coords_attr_idx = textured_program.program->attributeLocation ("texture_coords_attr");
    textured_program.matrix_uniform_idx = textured_program.program->uniformLocation ("ortho_matrix");
    textured_program.texture_mode_rect_uniform_idx = colored_textured_program.program->uniformLocation ("texture_mode_rect");
    textured_program.texture_2d_uniform_idx = colored_textured_program.program->uniformLocation ("texture_2d");
    textured_program.texture_2d_rect_uniform_idx = colored_textured_program.program->uniformLocation ("texture_2d_rect");
}
void OpenGLWidget::resizeGL (int w, int h)
{
    const qreal retinaScale = devicePixelRatio ();
    pixels_w = w*retinaScale;
    pixels_h = h*retinaScale;
    ortho_matrix.setToIdentity ();
    ortho_matrix.ortho (0, pixels_w, pixels_h, 0, -1, 1);
    updateSize (pixels_w, pixels_h);
}
void OpenGLWidget::paintGL ()
{
    glViewport (0, 0, pixels_w, pixels_h);
    glClear (GL_COLOR_BUFFER_BIT);

    draw ();
}
QSize OpenGLWidget::pixelsSize ()
{
    return QSize (pixels_w, pixels_h);
}
QSharedPointer<QOpenGLTexture> OpenGLWidget::loadTexture2D (const QString& path, bool autogenerate_margin)
{
    if (autogenerate_margin) {
        QImage source_image (path);
        QImage image (source_image.width ()*2, source_image.height ()*2, QImage::Format_ARGB32);
        image.fill (QColor (0, 0, 0, 0));
        {
            QPainter p (&image);
            p.drawImage (source_image.width ()/2, source_image.height ()/2, source_image);
        }
        QOpenGLTexture* texture = new QOpenGLTexture (image);
        texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
        texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
        return QSharedPointer<QOpenGLTexture> (texture);
    } else {
        QOpenGLTexture* texture = new QOpenGLTexture (QImage (path));
        texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
        texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
        return QSharedPointer<QOpenGLTexture> (texture);
    }
}
QSharedPointer<QOpenGLTexture> OpenGLWidget::loadTexture2DRectangle (const QString& path)
{
    QOpenGLTexture* texture = new QOpenGLTexture (QOpenGLTexture::TargetRectangle);
    texture->setData (QImage (path), QOpenGLTexture::DontGenerateMipMaps);
    texture->setMinificationFilter (QOpenGLTexture::Nearest);
    texture->setMagnificationFilter (QOpenGLTexture::Nearest);
    return QSharedPointer<QOpenGLTexture> (texture);
}
void OpenGLWidget::drawColored (GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* colors)
{
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_BLEND);

    colored_program.program->bind ();
    colored_program.program->setUniformValue (colored_program.matrix_uniform_idx, ortho_matrix);

    glVertexAttribPointer (colored_program.position_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, vertex_positions);
    glVertexAttribPointer (colored_program.color_attr_idx, 4, GL_FLOAT, GL_FALSE, 0, colors);

    glEnableVertexAttribArray (0);
    glEnableVertexAttribArray (1);

    glDrawArrays (mode, 0, vertex_count);

    glDisableVertexAttribArray (1);
    glDisableVertexAttribArray (0);

    colored_program.program->release ();

    glDisable (GL_BLEND);
}
void OpenGLWidget::drawColored (GLenum mode, const GLfloat* vertex_positions, const GLfloat* colors, GLsizei index_count, const GLuint* indices)
{
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_BLEND);

    colored_program.program->bind ();
    colored_program.program->setUniformValue (colored_program.matrix_uniform_idx, ortho_matrix);

    glVertexAttribPointer (colored_program.position_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, vertex_positions);
    glVertexAttribPointer (colored_program.color_attr_idx, 4, GL_FLOAT, GL_FALSE, 0, colors);

    glEnableVertexAttribArray (0);
    glEnableVertexAttribArray (1);

    glDrawElements (mode, index_count, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray (1);
    glDisableVertexAttribArray (0);

    colored_program.program->release ();

    glDisable (GL_BLEND);
}
void OpenGLWidget::drawColoredTextured (GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* colors, const GLfloat* texture_coords, QOpenGLTexture* texture)
{
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_BLEND);

    colored_textured_program.program->bind ();
    colored_textured_program.program->setUniformValue (colored_textured_program.matrix_uniform_idx, ortho_matrix);
    if (texture->target () == QOpenGLTexture::Target2DArray) {
        colored_textured_program.program->setUniformValue (colored_textured_program.texture_mode_rect_uniform_idx, 1.0f);
        colored_textured_program.program->setUniformValue (colored_textured_program.texture_2d_rect_uniform_idx, 0);
    } else {
        colored_textured_program.program->setUniformValue (colored_textured_program.texture_mode_rect_uniform_idx, 0.0f);
        colored_textured_program.program->setUniformValue (colored_textured_program.texture_2d_uniform_idx, 0);
    }

    texture->bind (0);

    glVertexAttribPointer (colored_textured_program.position_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, vertex_positions);
    glVertexAttribPointer (colored_textured_program.color_attr_idx, 4, GL_FLOAT, GL_FALSE, 0, colors);
    glVertexAttribPointer (colored_textured_program.texture_coords_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);

    glEnableVertexAttribArray (0);
    glEnableVertexAttribArray (1);
    glEnableVertexAttribArray (2);

    glDrawArrays (mode, 0, vertex_count);

    glDisableVertexAttribArray (2);
    glDisableVertexAttribArray (1);
    glDisableVertexAttribArray (0);

    texture->release();
    colored_textured_program.program->release ();

    glDisable (GL_BLEND);
}
void OpenGLWidget::drawColoredTextured (GLenum mode, const GLfloat* vertex_positions, const GLfloat* colors, const GLfloat* texture_coords, GLsizei index_count, const GLuint* indices, QOpenGLTexture* texture)
{
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_BLEND);

    colored_textured_program.program->bind ();
    colored_textured_program.program->setUniformValue (colored_textured_program.matrix_uniform_idx, ortho_matrix);
    colored_textured_program.program->setUniformValue (colored_textured_program.texture_2d_rect_uniform_idx, 0);
    colored_textured_program.program->setUniformValue (colored_textured_program.texture_2d_uniform_idx, 0);
    colored_textured_program.program->setUniformValue (colored_textured_program.texture_mode_rect_uniform_idx, (texture->target () == QOpenGLTexture::TargetRectangle) ? 1.0f : 0.0f);

    texture->bind (0);

    glVertexAttribPointer (colored_textured_program.position_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, vertex_positions);
    glVertexAttribPointer (colored_textured_program.color_attr_idx, 4, GL_FLOAT, GL_FALSE, 0, colors);
    glVertexAttribPointer (colored_textured_program.texture_coords_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);

    glEnableVertexAttribArray (colored_textured_program.position_attr_idx);
    glEnableVertexAttribArray (colored_textured_program.color_attr_idx);
    glEnableVertexAttribArray (colored_textured_program.texture_coords_attr_idx);

    glDrawElements (mode, index_count, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray (colored_textured_program.texture_coords_attr_idx);
    glDisableVertexAttribArray (colored_textured_program.color_attr_idx);
    glDisableVertexAttribArray (colored_textured_program.position_attr_idx);

    texture->release();
    colored_textured_program.program->release ();

    glDisable (GL_BLEND);
}
void OpenGLWidget::drawTextured (GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* texture_coords, QOpenGLTexture* texture)
{
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_BLEND);

    textured_program.program->bind ();
    textured_program.program->setUniformValue (textured_program.matrix_uniform_idx, ortho_matrix);
    textured_program.program->setUniformValue (textured_program.texture_2d_rect_uniform_idx, 0);
    textured_program.program->setUniformValue (textured_program.texture_2d_uniform_idx, 0);
    textured_program.program->setUniformValue (textured_program.texture_mode_rect_uniform_idx, (texture->target () == QOpenGLTexture::TargetRectangle) ? 1.0f : 0.0f);

    texture->bind (0);

    glVertexAttribPointer (textured_program.position_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, vertex_positions);
    glVertexAttribPointer (textured_program.texture_coords_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);

    glEnableVertexAttribArray (textured_program.position_attr_idx);
    glEnableVertexAttribArray (textured_program.texture_coords_attr_idx);

    glDrawArrays (mode, 0, vertex_count);

    glDisableVertexAttribArray (textured_program.texture_coords_attr_idx);
    glDisableVertexAttribArray (textured_program.position_attr_idx);

    texture->release();
    textured_program.program->release ();

    glDisable (GL_BLEND);
}
void OpenGLWidget::drawTextured (GLenum mode, const GLfloat* vertex_positions, const GLfloat* texture_coords, GLsizei index_count, const GLuint* indices, QOpenGLTexture* texture)
{
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_BLEND);

    textured_program.program->bind ();
    textured_program.program->setUniformValue (textured_program.matrix_uniform_idx, ortho_matrix);
    textured_program.program->setUniformValue (textured_program.texture_2d_rect_uniform_idx, 0);
    textured_program.program->setUniformValue (textured_program.texture_2d_uniform_idx, 0);
    textured_program.program->setUniformValue (textured_program.texture_mode_rect_uniform_idx, (texture->target () == QOpenGLTexture::TargetRectangle) ? 1.0f : 0.0f);

    texture->bind (0);

    glVertexAttribPointer (textured_program.position_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, vertex_positions);
    glVertexAttribPointer (textured_program.texture_coords_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);

    glEnableVertexAttribArray (textured_program.position_attr_idx);
    glEnableVertexAttribArray (textured_program.texture_coords_attr_idx);

    glDrawElements (mode, index_count, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray (textured_program.texture_coords_attr_idx);
    glDisableVertexAttribArray (textured_program.position_attr_idx);

    texture->release();
    textured_program.program->release ();

    glDisable (GL_BLEND);
}
void OpenGLWidget::drawRectangle (int x, int y, int w, int h, const QColor& color)
{
    const GLfloat vertices[] = {
        GLfloat (x + 0.5), GLfloat (y + 0.5),
        GLfloat (x + 0.5 + w), GLfloat (y + 0.5),
        GLfloat (x + 0.5 + w), GLfloat (y + 0.5 + h),
        GLfloat (x + 0.5), GLfloat (y + 0.5 + h),
    };

    GLfloat r = color.redF ();
    GLfloat g = color.greenF ();
    GLfloat b = color.blueF ();
    GLfloat a = color.alphaF ();

    const GLfloat colors[] = {
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
    };

    drawColored (GL_LINE_LOOP, 4, vertices, colors);
}
void OpenGLWidget::fillRectangle (int x, int y, QOpenGLTexture* texture)
{
    int w = texture->width ();
    int h = texture->height ();
    const GLfloat vertices[] = {
        GLfloat (x), GLfloat (y),
        GLfloat (x + w), GLfloat (y),
        GLfloat (x + w), GLfloat (y + h),
        GLfloat (x), GLfloat (y + h),
    };

    const GLfloat texture_coords[] = {
        0, 0,
        GLfloat (w), 0,
        GLfloat (w), GLfloat (h),
        0, GLfloat (h),
    };

    const GLuint indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    drawTextured (GL_TRIANGLES, vertices, texture_coords, 6, indices, texture);
}
void OpenGLWidget::fillRectangle (int x, int y, int w, int h, QOpenGLTexture* texture)
{
    const GLfloat vertices[] = {
        GLfloat (x), GLfloat (y),
        GLfloat (x + w), GLfloat (y),
        GLfloat (x + w), GLfloat (y + h),
        GLfloat (x), GLfloat (y + h),
    };

    const GLfloat texture_coords[] = {
        0, 0,
        GLfloat (w), 0,
        GLfloat (w), GLfloat (h),
        0, GLfloat (h),
    };

    const GLuint indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    drawTextured (GL_TRIANGLES, vertices, texture_coords, 6, indices, texture);
}
void OpenGLWidget::fillRectangle (int x, int y, int w, int h, const QColor& color)
{
    const GLfloat vertices[] = {
        GLfloat (x), GLfloat (y),
        GLfloat (x + w), GLfloat (y),
        GLfloat (x), GLfloat (y + h),
        GLfloat (x + w), GLfloat (y + h),
    };

    GLfloat r = color.redF ();
    GLfloat g = color.greenF ();
    GLfloat b = color.blueF ();
    GLfloat a = color.alphaF ();

    const GLfloat colors[] = {
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
    };

    drawColored (GL_TRIANGLE_STRIP, 4, vertices, colors);
}
void OpenGLWidget::fillRectangle (qreal x, qreal y, qreal w, qreal h, const QColor& color)
{
    const GLfloat vertices[] = {
        GLfloat (x), GLfloat (y),
        GLfloat (x + w), GLfloat (y),
        GLfloat (x), GLfloat (y + h),
        GLfloat (x + w), GLfloat (y + h),
    };

    GLfloat r = color.redF ();
    GLfloat g = color.greenF ();
    GLfloat b = color.blueF ();
    GLfloat a = color.alphaF ();

    const GLfloat colors[] = {
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
    };

    drawColored (GL_TRIANGLE_STRIP, 4, vertices, colors);
}
void OpenGLWidget::fillRectangle (const QRect& rect, const QColor& color)
{
    fillRectangle (rect.x (), rect.y (), rect.width (), rect.height (), color);
}
void OpenGLWidget::fillRectangle (const QRectF& rect, const QColor& color)
{
    fillRectangle (rect.x (), rect.y (), rect.width (), rect.height (), color);
}
void OpenGLWidget::drawCircle (qreal x, qreal y, qreal radius, const QColor& color)
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
        float angle = i*(M_PI*2.0)/count;
        float dx, dy;
        sincosf (angle, &dy, &dx);
        vertices.append (x_f + radius*dx);
        vertices.append (y_f + radius*dy);
        colors.append (r);
        colors.append (g);
        colors.append (b);
        colors.append (a);
    }

    glEnable (GL_LINE_SMOOTH);
    drawColored (GL_LINE_LOOP, count, vertices.data (), colors.data ());
    glDisable (GL_LINE_SMOOTH);
}
