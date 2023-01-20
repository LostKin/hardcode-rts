#include "openglwidget.h"

#include <QFile>


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
QSharedPointer<QOpenGLTexture> OpenGLWidget::loadTexture2D (const QString& path)
{
    return QSharedPointer<QOpenGLTexture> (new QOpenGLTexture (QImage (path)));
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
void OpenGLWidget::drawRectangle (int x, int y, QOpenGLTexture* texture)
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
void OpenGLWidget::drawRectangle (int x, int y, int w, int h, QOpenGLTexture* texture)
{
    int tex_w = texture->width ();
    int tex_h = texture->height ();
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
