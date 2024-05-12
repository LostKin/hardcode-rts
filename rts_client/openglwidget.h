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
    virtual ~OpenGLWidget ();

protected:
    void initializeGL () override;
    void resizeGL (int w, int h) override;
    void paintGL () override;

protected:
    virtual void initResources () = 0;
    virtual void updateSize (int w, int h) = 0;
    virtual void draw () = 0;

protected:
    QSize pixelsSize ();
    QSharedPointer<QOpenGLTexture> loadTexture2DRectangle (const QString& path);

private:
    int pixels_w = 1;
    int pixels_h = 1;

protected:
    QMatrix4x4 ortho_matrix; // TODO: Fix this
};
