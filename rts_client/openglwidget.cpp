#include "openglwidget.h"

#include <QFile>
#include <QPainter>
#include <math.h>

OpenGLWidget::OpenGLWidget (QWidget* parent)
    : QOpenGLWidget (parent)
{
    setMinimumSize (512, 512);
    setUpdateBehavior (QOpenGLWidget::NoPartialUpdate);
    connect (this, SIGNAL (frameSwapped ()), this, SLOT (update ()));
}
OpenGLWidget::~OpenGLWidget ()
{
}

void OpenGLWidget::initializeGL ()
{
    initializeOpenGLFunctions ();

    glClearColor (0, 0, 0, 1);

    initResources ();
}
void OpenGLWidget::resizeGL (int w, int h)
{
    const qreal retinaScale = devicePixelRatio ();
    pixels_w = w * retinaScale;
    pixels_h = h * retinaScale;
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
    QOpenGLTexture* texture = new QOpenGLTexture (QImage (path));
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
QSharedPointer<QOpenGLTexture> OpenGLWidget::loadTexture2D (const QImage& image)
{
    QOpenGLTexture* texture = new QOpenGLTexture (image);
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
QSharedPointer<QOpenGLTexture> OpenGLWidget::loadTexture2DRectangle (const QString& path)
{
    QImage image = QImage (path).convertToFormat (QImage::Format_RGBA8888);
    QOpenGLTexture* texture = new QOpenGLTexture (QOpenGLTexture::TargetRectangle);
    texture->setFormat (QOpenGLTexture::QOpenGLTexture::RGBAFormat);
    texture->setSize (image.width (), image.height ());
    texture->allocateStorage ();
    texture->setData (QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, image.bits ());
    texture->setLevelOfDetailRange (0, 0);
    texture->setMinificationFilter (QOpenGLTexture::Nearest);
    texture->setMagnificationFilter (QOpenGLTexture::Nearest);
    return QSharedPointer<QOpenGLTexture> (texture);
}
