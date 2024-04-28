#include "coloredtexturedrenderer.h"

#include <QFile>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <math.h>


QSharedPointer<ColoredTexturedRenderer> ColoredTexturedRenderer::Create ()
{
    QSharedPointer<QOpenGLShaderProgram> program (new QOpenGLShaderProgram ());
    {
        QFile shader_fh (":/gpu_programs/colored_textured/main.vert");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return nullptr;
        QByteArray shader_data = shader_fh.readAll ();
        program->addShaderFromSourceCode (QOpenGLShader::Vertex, shader_data);
    }
    {
        QFile shader_fh (":/gpu_programs/colored_textured/main.frag");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return nullptr;
        QByteArray shader_data = shader_fh.readAll ();
        program->addShaderFromSourceCode (QOpenGLShader::Fragment, shader_data);
    }
    if (!program->link ())
        return nullptr;
    int position_attr_idx = program->attributeLocation ("position_attr");
    int color_attr_idx = program->attributeLocation ("color_attr");
    int texture_coords_attr_idx = program->attributeLocation ("texture_coords_attr");
    int matrix_uniform_idx = program->uniformLocation ("ortho_matrix");
    int texture_mode_rect_uniform_idx = program->uniformLocation ("texture_mode_rect");
    int texture_2d_uniform_idx = program->uniformLocation ("texture_2d");
    int texture_2d_rect_uniform_idx = program->uniformLocation ("texture_2d_rect");
    if (position_attr_idx < 0 ||
        color_attr_idx < 0 ||
        texture_coords_attr_idx < 0 ||
        matrix_uniform_idx < 0 ||
        texture_mode_rect_uniform_idx < 0 ||
        texture_2d_uniform_idx < 0 ||
        texture_2d_rect_uniform_idx < 0)
        return nullptr;
    return QSharedPointer<ColoredTexturedRenderer> (new ColoredTexturedRenderer (
        program,
        position_attr_idx,
        color_attr_idx,
        texture_coords_attr_idx,
        matrix_uniform_idx,
        texture_mode_rect_uniform_idx,
        texture_2d_uniform_idx,
        texture_2d_rect_uniform_idx
    ));
}

ColoredTexturedRenderer::ColoredTexturedRenderer (QSharedPointer<QOpenGLShaderProgram> program,
                                    GLuint position_attr_idx,
                                    GLuint color_attr_idx,
                                    GLuint texture_coords_attr_idx,
                                    GLuint matrix_uniform_idx,
                                    GLuint texture_mode_rect_uniform_idx,
                                    GLuint texture_2d_uniform_idx,
                                    GLuint texture_2d_rect_uniform_idx)
    : program (program)
    , position_attr_idx (position_attr_idx)
    , color_attr_idx (color_attr_idx)
    , texture_coords_attr_idx (texture_coords_attr_idx)
    , matrix_uniform_idx (matrix_uniform_idx)
    , texture_mode_rect_uniform_idx (texture_mode_rect_uniform_idx)
    , texture_2d_uniform_idx (texture_2d_uniform_idx)
    , texture_2d_rect_uniform_idx (texture_2d_rect_uniform_idx)
{
}

void ColoredTexturedRenderer::draw (QOpenGLFunctions& gl, GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* colors, const GLfloat* texture_coords,
                                    QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix)
{
    gl.glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.glEnable (GL_BLEND);

    program->bind ();
    program->setUniformValue (matrix_uniform_idx, ortho_matrix);
    program->setUniformValue (texture_2d_rect_uniform_idx, 0);
    program->setUniformValue (texture_2d_uniform_idx, 0);
    program->setUniformValue (texture_mode_rect_uniform_idx, (texture->target () == QOpenGLTexture::TargetRectangle) ? 1.0f : 0.0f);

    texture->bind (0);

    gl.glVertexAttribPointer (position_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, vertex_positions);
    gl.glVertexAttribPointer (color_attr_idx, 4, GL_FLOAT, GL_FALSE, 0, colors);
    gl.glVertexAttribPointer (texture_coords_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);

    gl.glEnableVertexAttribArray (position_attr_idx);
    gl.glEnableVertexAttribArray (color_attr_idx);
    gl.glEnableVertexAttribArray (texture_coords_attr_idx);

    gl.glDrawArrays (mode, 0, vertex_count);

    gl.glDisableVertexAttribArray (texture_coords_attr_idx);
    gl.glDisableVertexAttribArray (color_attr_idx);
    gl.glDisableVertexAttribArray (position_attr_idx);

    texture->release ();
    program->release ();

    gl.glDisable (GL_BLEND);
}
void ColoredTexturedRenderer::draw (QOpenGLFunctions& gl, GLenum mode, const GLfloat* vertex_positions, const GLfloat* colors, const GLfloat* texture_coords, GLsizei index_count, const GLuint* indices,
                                    QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix)
{
    gl.glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.glEnable (GL_BLEND);

    program->bind ();
    program->setUniformValue (matrix_uniform_idx, ortho_matrix);
    program->setUniformValue (texture_2d_rect_uniform_idx, 0);
    program->setUniformValue (texture_2d_uniform_idx, 0);
    program->setUniformValue (texture_mode_rect_uniform_idx, (texture->target () == QOpenGLTexture::TargetRectangle) ? 1.0f : 0.0f);

    texture->bind (0);

    gl.glVertexAttribPointer (position_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, vertex_positions);
    gl.glVertexAttribPointer (color_attr_idx, 4, GL_FLOAT, GL_FALSE, 0, colors);
    gl.glVertexAttribPointer (texture_coords_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);

    gl.glEnableVertexAttribArray (position_attr_idx);
    gl.glEnableVertexAttribArray (color_attr_idx);
    gl.glEnableVertexAttribArray (texture_coords_attr_idx);

    gl.glDrawElements (mode, index_count, GL_UNSIGNED_INT, indices);

    gl.glDisableVertexAttribArray (texture_coords_attr_idx);
    gl.glDisableVertexAttribArray (color_attr_idx);
    gl.glDisableVertexAttribArray (position_attr_idx);

    texture->release ();
    program->release ();

    gl.glDisable (GL_BLEND);
}
