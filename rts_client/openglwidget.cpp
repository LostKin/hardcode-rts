#include "openglwidget.h"

#include <QFile>


OpenGLWidget::OpenGLWidget (QWidget* parent)
    : QOpenGLWidget (parent)
{
    setMinimumSize (512, 512);
}
OpenGLWidget::~OpenGLWidget ()
{
}

void OpenGLWidget::initializeGL ()
{
    initializeOpenGLFunctions ();

    glClearColor (0, 0, 0, 1);

    initPrograms ();
    initTextures ();
}
void OpenGLWidget::initPrograms ()
{
    initColoredProgram ();
    initColoredTexturedProgram ();
    initTexturedProgram ();
}
void OpenGLWidget::initTextures ()
{
    grass_texture = QSharedPointer<QOpenGLTexture> (new QOpenGLTexture(QImage(":/images/grass.png")));
    cat_texture = QSharedPointer<QOpenGLTexture> (new QOpenGLTexture(QImage(":/images/cat.png")));
}
void OpenGLWidget::initColoredProgram ()
{
    colored_program.program = new QOpenGLShaderProgram (this);
    {
        QFile shader_fh (":/gpu_programs/colored/main.vert");
        if (!shader_fh.open(QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll();
        colored_program.program->addShaderFromSourceCode (QOpenGLShader::Vertex, shader_data);
    }
    {
        QFile shader_fh (":/gpu_programs/colored/main.frag");
        if (!shader_fh.open(QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll();
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
        if (!shader_fh.open(QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll();
        colored_textured_program.program->addShaderFromSourceCode (QOpenGLShader::Vertex, shader_data);
    }
    {
        QFile shader_fh (":/gpu_programs/colored_textured/main.frag");
        if (!shader_fh.open(QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll();
        colored_textured_program.program->addShaderFromSourceCode (QOpenGLShader::Fragment, shader_data);
    }
    colored_textured_program.program->link ();
    colored_textured_program.position_attr_idx = colored_textured_program.program->attributeLocation ("position_attr");
    colored_textured_program.color_attr_idx = colored_textured_program.program->attributeLocation ("color_attr");
    colored_textured_program.texture_coords_attr_idx = colored_textured_program.program->attributeLocation ("texture_coords_attr");
    colored_textured_program.matrix_uniform_idx = colored_textured_program.program->uniformLocation ("ortho_matrix");
    colored_textured_program.texture_uniform_idx = colored_textured_program.program->uniformLocation ("texture_matrix");
}
void OpenGLWidget::initTexturedProgram ()
{
    textured_program.program = new QOpenGLShaderProgram (this);
    {
        QFile shader_fh (":/gpu_programs/textured/main.vert");
        if (!shader_fh.open(QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll();
        textured_program.program->addShaderFromSourceCode (QOpenGLShader::Vertex, shader_data);
    }
    {
        QFile shader_fh (":/gpu_programs/textured/main.frag");
        if (!shader_fh.open(QIODevice::ReadOnly))
            return;
        QByteArray shader_data = shader_fh.readAll();
        textured_program.program->addShaderFromSourceCode (QOpenGLShader::Fragment, shader_data);
    }
    textured_program.program->link ();
    textured_program.position_attr_idx = textured_program.program->attributeLocation ("position_attr");
    textured_program.texture_coords_attr_idx = textured_program.program->attributeLocation ("texture_coords_attr");
    textured_program.matrix_uniform_idx = textured_program.program->uniformLocation ("ortho_matrix");
    textured_program.texture_uniform_idx = textured_program.program->uniformLocation ("texture_matrix");
}
void OpenGLWidget::resizeGL (int w, int h)
{
    const qreal retinaScale = devicePixelRatio ();
    pixels_w = w * retinaScale;
    pixels_h = h * retinaScale;
    ortho_matrix.setToIdentity ();
    ortho_matrix.ortho (0, pixels_w, pixels_h, 0, -1, 1);
}
void OpenGLWidget::paintGL ()
{
    glViewport (0, 0, pixels_w, pixels_h);
    glClear (GL_COLOR_BUFFER_BIT);

    // Background
    {
        const GLfloat vertices[] = {
            0, 0,
            GLfloat (pixels_w), 0,
            GLfloat (pixels_w), GLfloat (pixels_h),
            0, GLfloat (pixels_h),
        };

        // Wrapped texture
        GLfloat texture_coords_w = GLfloat (pixels_w)/grass_texture->width ();
        GLfloat texture_coords_h = GLfloat (pixels_h)/grass_texture->height ();
        const GLfloat texture_coords[] = {
            0, 0,
            texture_coords_w, 0,
            texture_coords_w, texture_coords_h,
            0, texture_coords_h,
        };

        const GLuint indices[] = {
            0, 1, 2,
            0, 2, 3,
        };

        drawTextured (GL_TRIANGLES, vertices, texture_coords, 6, indices, grass_texture.get());
    }

    // Triangle
    {
        const GLfloat vertices[] = {
            128, 10,
            246, 246,
            10, 246,
        };

        const GLfloat colors[] = {
            1, 0, 0, 0,
            0, 1, 0, 1,
            0, 0, 1, 1,
        };

        drawColored (GL_TRIANGLES, 3, vertices, colors);
    }

    // Some pop art
    {
        {
            const GLfloat vertices[] = {
                0, 256,
                128, 256,
                128, 384,
                0, 384,
            };

            const GLfloat colors[] = {
                1, 0, 0, 1,
                1, 0, 0, 1,
                1, 0, 0, 1,
                1, 0, 0, 1,
            };

            const GLfloat texture_coords[] = {
                0, 0,
                1, 0,
                1, 1,
                0, 1,
            };

            const GLuint indices[] = {
                0, 1, 2,
                0, 2, 3,
            };

            drawColoredTextured (GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, cat_texture.get());
        }
        {
            const GLfloat vertices[] = {
                128, 256,
                256, 256,
                256, 384,
                128, 384,
            };

            const GLfloat colors[] = {
                0, 1, 0, 1,
                0, 1, 0, 1,
                0, 1, 0, 1,
                0, 1, 0, 1,
            };

            const GLfloat texture_coords[] = {
                0, 0,
                1, 0,
                1, 1,
                0, 1,
            };

            const GLuint indices[] = {
                0, 1, 2,
                0, 2, 3,
            };

            drawColoredTextured (GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, cat_texture.get());
        }
        {
            const GLfloat vertices[] = {
                256, 256,
                384, 256,
                384, 384,
                256, 384,
            };

            const GLfloat colors[] = {
                0, 0, 1, 1,
                0, 0, 1, 1,
                0, 0, 1, 1,
                0, 0, 1, 1,
            };

            const GLfloat texture_coords[] = {
                0, 0,
                1, 0,
                1, 1,
                0, 1,
            };

            const GLuint indices[] = {
                0, 1, 2,
                0, 2, 3,
            };

            drawColoredTextured (GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, cat_texture.get());
        }
        {
            const GLfloat vertices[] = {
                0, 384,
                384, 384,
                384, 512,
                0, 512,
            };

            // Wrapped texture
            const GLfloat texture_coords[] = {
                0, 0,
                3, 0,
                3, 1,
                0, 1,
            };

            const GLuint indices[] = {
                0, 1, 2,
                0, 2, 3,
            };

            drawTextured (GL_TRIANGLES, vertices, texture_coords, 6, indices, cat_texture.get());
        }
    }

    // Lines
    {
        const GLfloat vertices[] = {
            256.5, 0.5,
            383.5, 0.5,
            383.5, 255.5,
            256.5, 255.5,
        };

        const GLfloat colors[] = {
            0, 1, 0, 1,
            0, 1, 0, 1,
            0, 1, 0, 1,
            0, 1, 0, 1,
        };

        const GLuint indices[] = {
            0, 1,
            1, 2,
            2, 3,
            3, 0,
        };

        drawColored (GL_LINES, vertices, colors, 8, indices);
    }

    // Points
    {
        size_t count = 50;

        QVector<GLfloat> vertices (count*2);
        QVector<GLfloat> colors (count*4);
        for (size_t i = 0; i < count; ++i) {
            vertices[i*2] = 258.5 + i;
            vertices[i*2 + 1] = 2.5 + i;

            colors[i*4] = 0;
            colors[i*4 + 1] = 1;
            colors[i*4 + 2] = 0;
            colors[i*4 + 3] = 1;
        }

        drawColored (GL_POINTS, count, vertices.constData (), colors.constData ());
    }
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
    colored_textured_program.program->setUniformValue (colored_textured_program.texture_uniform_idx, 0);

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
    colored_textured_program.program->setUniformValue (colored_textured_program.texture_uniform_idx, 0);

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
    textured_program.program->setUniformValue (textured_program.texture_uniform_idx, 0);

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
    textured_program.program->setUniformValue (textured_program.texture_uniform_idx, 0);

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
