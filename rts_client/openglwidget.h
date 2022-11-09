#pragma once

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QOpenGLTexture>


class OpenGLWidget: public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    OpenGLWidget (QWidget* parent = nullptr);
    ~OpenGLWidget ();

protected:
    void initializeGL () override;
    void resizeGL (int w, int h) override;
    void paintGL () override;

private:
    void initPrograms ();
    void initColoredProgram ();
    void initColoredTexturedProgram ();
    void initTexturedProgram ();
    void initTextures ();
    void drawColored (GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* colors);
    void drawColored (GLenum mode, const GLfloat* vertex_positions, const GLfloat* colors, GLsizei index_count, const GLuint* indices);
    void drawColoredTextured (GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* colors, const GLfloat* texture_coords, QOpenGLTexture* texture);
    void drawColoredTextured (GLenum mode, const GLfloat* vertex_positions, const GLfloat* colors, const GLfloat* texture_coords, GLsizei index_count, const GLuint* indices, QOpenGLTexture* texture);
    void drawTextured (GLenum mode, GLsizei vertex_count, const GLfloat* vertex_positions, const GLfloat* texture_coords, QOpenGLTexture* texture);
    void drawTextured (GLenum mode, const GLfloat* vertex_positions, const GLfloat* texture_coords, GLsizei index_count, const GLuint* indices, QOpenGLTexture* texture);

private:
    struct ColoredProgram
    {
        QOpenGLShaderProgram* program;

        GLuint position_attr_idx;
        GLuint color_attr_idx;

        GLuint matrix_uniform_idx;
    };
    struct ColoredTexturedProgram
    {
        QOpenGLShaderProgram* program;

        GLuint position_attr_idx;
        GLuint color_attr_idx;
        GLuint texture_coords_attr_idx;

        GLuint matrix_uniform_idx;
        GLuint texture_uniform_idx;
    };
    struct TexturedProgram
    {
        QOpenGLShaderProgram* program;

        GLuint position_attr_idx;
        GLuint texture_coords_attr_idx;

        GLuint matrix_uniform_idx;
        GLuint texture_uniform_idx;
    };

    int pixels_w = 1;
    int pixels_h = 1;
    QMatrix4x4 ortho_matrix;

    ColoredProgram colored_program;
    ColoredTexturedProgram colored_textured_program;
    TexturedProgram textured_program;
    QSharedPointer<QOpenGLTexture> grass_texture;
    QSharedPointer<QOpenGLTexture> cat_texture;
};
