#pragma once

#include <QSharedPointer>
#include <QOpenGLFunctions>

class QOpenGLShaderProgram;
class QOpenGLTexture;


class TexturedRenderer
{
public:
    static QSharedPointer<TexturedRenderer> Create ();
    void draw (QOpenGLFunctions& gl, GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* texture_coords, QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix);
    void draw (QOpenGLFunctions& gl, GLenum mode, const GLfloat* vertex_positions, const GLfloat* texture_coords, GLsizei index_count, const GLuint* indices, QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix);
    void fillRectangle (QOpenGLFunctions& gl, int x, int y, QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix);
    void fillRectangle (QOpenGLFunctions& gl, int x, int y, int w, int h, QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix);

private:
    TexturedRenderer (QSharedPointer<QOpenGLShaderProgram> program,
                      GLuint position_attr_idx,
                      GLuint texture_coords_attr_idx,
                      GLuint matrix_uniform_idx,
                      GLuint texture_mode_rect_uniform_idx,
                      GLuint texture_2d_uniform_idx,
                      GLuint texture_2d_rect_uniform_idx);

private:
    QSharedPointer<QOpenGLShaderProgram> program;

    GLuint position_attr_idx;
    GLuint texture_coords_attr_idx;

    GLuint matrix_uniform_idx;
    GLuint texture_mode_rect_uniform_idx;
    GLuint texture_2d_uniform_idx;
    GLuint texture_2d_rect_uniform_idx;
};
