#pragma once

#include "rectangle.h"

#include <QSharedPointer>
#include <QOpenGLFunctions>

class QOpenGLShaderProgram;


class ColoredRenderer
{
public:
    static QSharedPointer<ColoredRenderer> Create ();
    void draw (QOpenGLFunctions& gl, GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* colors, const QMatrix4x4& ortho_matrix);
    void draw (QOpenGLFunctions& gl, GLenum mode, const GLfloat* vertex_positions, const GLfloat* colors, GLsizei index_count, const GLuint* indices, const QMatrix4x4& ortho_matrix);
    void drawRectangle (QOpenGLFunctions& gl, int x, int y, int w, int h, const QColor& color, const QMatrix4x4& ortho_matrix);
    void drawRectangle (QOpenGLFunctions& gl, const QRectF& rect, const QColor& color, const QMatrix4x4& ortho_matrix);
    void fillRectangle (QOpenGLFunctions& gl, int x, int y, int w, int h, const QColor& color, const QMatrix4x4& ortho_matrix);
    void fillRectangle (QOpenGLFunctions& gl, qreal x, qreal y, qreal w, qreal h, const QColor& color, const QMatrix4x4& ortho_matrix);
    void fillRectangle (QOpenGLFunctions& gl, const QRect& rect, const QColor& color, const QMatrix4x4& ortho_matrix);
    void fillRectangle (QOpenGLFunctions& gl, const QRectF& rect, const QColor& color, const QMatrix4x4& ortho_matrix);
    void fillRectangle (QOpenGLFunctions& gl, const Rectangle& rect, const QColor& color, const QMatrix4x4& ortho_matrix);
    void drawCircle (QOpenGLFunctions& gl, qreal x, qreal y, qreal radius, const QColor& color, const QMatrix4x4& ortho_matrix);

private:
    ColoredRenderer (QSharedPointer<QOpenGLShaderProgram> program,
                     GLuint position_attr_idx,
                     GLuint color_attr_idx,
                     GLuint matrix_uniform_idx);

private:
    QSharedPointer<QOpenGLShaderProgram> program;

    GLuint position_attr_idx;
    GLuint color_attr_idx;

    GLuint matrix_uniform_idx;
};
