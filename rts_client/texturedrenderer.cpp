#include "texturedrenderer.h"

#include <QFile>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <math.h>


QSharedPointer<TexturedRenderer> TexturedRenderer::Create ()
{
    QSharedPointer<QOpenGLShaderProgram> program (new QOpenGLShaderProgram ());
    {
        QFile shader_fh (":/gpu_programs/textured/main.vert");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return nullptr;
        QByteArray shader_data = shader_fh.readAll ();
        program->addShaderFromSourceCode (QOpenGLShader::Vertex, shader_data);
    }
    {
        QFile shader_fh (":/gpu_programs/textured/main.frag");
        if (!shader_fh.open (QIODevice::ReadOnly))
            return nullptr;
        QByteArray shader_data = shader_fh.readAll ();
        program->addShaderFromSourceCode (QOpenGLShader::Fragment, shader_data);
    }
    if (!program->link ())
        return nullptr;
    int position_attr_idx = program->attributeLocation ("position_attr");
    int texture_coords_attr_idx = program->attributeLocation ("texture_coords_attr");
    int matrix_uniform_idx = program->uniformLocation ("ortho_matrix");
    int texture_mode_rect_uniform_idx = program->uniformLocation ("texture_mode_rect");
    int texture_2d_uniform_idx = program->uniformLocation ("texture_2d");
    int texture_2d_rect_uniform_idx = program->uniformLocation ("texture_2d_rect");
    if (position_attr_idx < 0 ||
        texture_coords_attr_idx < 0 ||
        matrix_uniform_idx < 0 ||
        texture_mode_rect_uniform_idx < 0 ||
        texture_2d_uniform_idx < 0 ||
        texture_2d_rect_uniform_idx < 0)
        return nullptr;
    return QSharedPointer<TexturedRenderer> (new TexturedRenderer (
        program,
        position_attr_idx,
        texture_coords_attr_idx,
        matrix_uniform_idx,
        texture_mode_rect_uniform_idx,
        texture_2d_uniform_idx,
        texture_2d_rect_uniform_idx
    ));
}

TexturedRenderer::TexturedRenderer (QSharedPointer<QOpenGLShaderProgram> program,
                                    GLuint position_attr_idx,
                                    GLuint texture_coords_attr_idx,
                                    GLuint matrix_uniform_idx,
                                    GLuint texture_mode_rect_uniform_idx,
                                    GLuint texture_2d_uniform_idx,
                                    GLuint texture_2d_rect_uniform_idx)
    : program (program)
    , position_attr_idx (position_attr_idx)
    , texture_coords_attr_idx (texture_coords_attr_idx)
    , matrix_uniform_idx (matrix_uniform_idx)
    , texture_mode_rect_uniform_idx (texture_mode_rect_uniform_idx)
    , texture_2d_uniform_idx (texture_2d_uniform_idx)
    , texture_2d_rect_uniform_idx (texture_2d_rect_uniform_idx)
{
}

void TexturedRenderer::draw (QOpenGLFunctions& gl, GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* texture_coords,
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
    gl.glVertexAttribPointer (texture_coords_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);

    gl.glEnableVertexAttribArray (position_attr_idx);
    gl.glEnableVertexAttribArray (texture_coords_attr_idx);

    gl.glDrawArrays (mode, 0, vertex_count);

    gl.glDisableVertexAttribArray (texture_coords_attr_idx);
    gl.glDisableVertexAttribArray (position_attr_idx);

    texture->release ();
    program->release ();

    gl.glDisable (GL_BLEND);
}
void TexturedRenderer::draw (QOpenGLFunctions& gl, GLenum mode, const GLfloat* vertex_positions, const GLfloat* texture_coords, GLsizei index_count, const GLuint* indices,
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
    gl.glVertexAttribPointer (texture_coords_attr_idx, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);

    gl.glEnableVertexAttribArray (position_attr_idx);
    gl.glEnableVertexAttribArray (texture_coords_attr_idx);

    gl.glDrawElements (mode, index_count, GL_UNSIGNED_INT, indices);

    gl.glDisableVertexAttribArray (texture_coords_attr_idx);
    gl.glDisableVertexAttribArray (position_attr_idx);

    texture->release ();
    program->release ();

    gl.glDisable (GL_BLEND);
}
void TexturedRenderer::fillRectangle (QOpenGLFunctions& gl, int x, int y, QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix)
{
    int w = texture->width ();
    int h = texture->height ();
    const GLfloat vertices[] = {
        GLfloat (x),
        GLfloat (y),
        GLfloat (x + w),
        GLfloat (y),
        GLfloat (x + w),
        GLfloat (y + h),
        GLfloat (x),
        GLfloat (y + h),
    };

    const GLfloat texture_coords[] = {
        0,
        0,
        GLfloat (w),
        0,
        GLfloat (w),
        GLfloat (h),
        0,
        GLfloat (h),
    };

    const GLuint indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    draw (gl, GL_TRIANGLES, vertices, texture_coords, 6, indices, texture, ortho_matrix);
}
void TexturedRenderer::fillRectangle (QOpenGLFunctions& gl, int x, int y, int w, int h, QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix)
{
    const GLfloat vertices[] = {
        GLfloat (x),
        GLfloat (y),
        GLfloat (x + w),
        GLfloat (y),
        GLfloat (x + w),
        GLfloat (y + h),
        GLfloat (x),
        GLfloat (y + h),
    };

    const GLfloat texture_coords[] = {
        0,
        0,
        GLfloat (w),
        0,
        GLfloat (w),
        GLfloat (h),
        0,
        GLfloat (h),
    };

    const GLuint indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    draw (gl, GL_TRIANGLES, vertices, texture_coords, 6, indices, texture, ortho_matrix);
}
