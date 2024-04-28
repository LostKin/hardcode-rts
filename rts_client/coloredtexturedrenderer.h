#pragma once

#include <QSharedPointer>
#include <QOpenGLFunctions>

class QOpenGLShaderProgram;
class QOpenGLTexture;


class ColoredTexturedRenderer
{
public:
    static QSharedPointer<ColoredTexturedRenderer> Create ();
    void draw (QOpenGLFunctions& gl, GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* colors, const GLfloat* texture_coords,
               QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix);
    void draw (QOpenGLFunctions& gl, GLenum mode, const GLfloat* vertex_positions, const GLfloat* colors, const GLfloat* texture_coords, GLsizei index_count, const GLuint* indices,
               QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix);

private:
    ColoredTexturedRenderer (QSharedPointer<QOpenGLShaderProgram> program,
                             GLuint position_attr_idx,
                             GLuint color_attr_idx,
                             GLuint texture_coords_attr_idx,
                             GLuint matrix_uniform_idx,
                             GLuint texture_mode_rect_uniform_idx,
                             GLuint texture_2d_uniform_idx,
                             GLuint texture_2d_rect_uniform_idx);

private:
    QSharedPointer<QOpenGLShaderProgram> program;

    GLuint position_attr_idx;
    GLuint color_attr_idx;
    GLuint texture_coords_attr_idx;

    GLuint matrix_uniform_idx;
    GLuint texture_mode_rect_uniform_idx;
    GLuint texture_2d_uniform_idx;
    GLuint texture_2d_rect_uniform_idx;
};
