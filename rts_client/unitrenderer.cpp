#include "unitrenderer.h"

#include <QFile>
#include <QSvgRenderer>
#include <QPainter>
#include <QOpenGLTexture>


UnitRenderer::UnitRenderer (const QString& name)
    : name (name)
{
}

void UnitRenderer::loadResources (const QColor& team_color)
{
    standing = loadTexture2D (loadUnitFromSVGTemplate (":/images/units/" + name + "/unit_tmpl.svg", "standing", team_color));
    walking1 = loadTexture2D (loadUnitFromSVGTemplate (":/images/units/" + name + "/unit_tmpl.svg", "walking1", team_color));
    walking2 = loadTexture2D (loadUnitFromSVGTemplate (":/images/units/" + name + "/unit_tmpl.svg", "walking2", team_color));
    shooting1 = loadTexture2D (loadUnitFromSVGTemplate (":/images/units/" + name + "/unit_tmpl.svg", "attacking1", team_color));
    shooting2 = loadTexture2D (loadUnitFromSVGTemplate (":/images/units/" + name + "/unit_tmpl.svg", "attacking2", team_color));
}
void UnitRenderer::draw (const Unit& unit)
{
}
QImage UnitRenderer::loadUnitFromSVGTemplate (const QString& path, const QByteArray& animation_name, const QColor& player_color)
{
    static constexpr QSize texture_size (256, 256);
    QImage image (texture_size, QImage::Format_ARGB32);
    image.fill (QColor (0, 0, 0, 0));
    QFile svg_fh (path);
    if (!svg_fh.open (QIODevice::ReadOnly))
        return image;
    QByteArray svg_data = svg_fh.readAll ();
    svg_data.replace ("&animation;", animation_name);
    svg_data.replace ("&player_color;", player_color.name ().toUtf8 ());
    QSvgRenderer svg_renderer (svg_data);
    {
        QPainter p (&image);
        p.translate (texture_size.width ()*0.5, texture_size.height ()*0.5);
        p.rotate (90.0);
        p.translate (-texture_size.width ()*0.5, -texture_size.height ()*0.5);
        svg_renderer.render (&p, QRectF (QPointF (0, 0), QSizeF (texture_size)));
    }
    return image;
}
QSharedPointer<QOpenGLTexture> UnitRenderer::loadTexture2D (const QImage& image)
{
    QOpenGLTexture* texture = new QOpenGLTexture (image);
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
