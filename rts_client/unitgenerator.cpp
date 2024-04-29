#include "unitgenerator.h"

#include <QFile>
#include <QSvgRenderer>
#include <QPainter>


QImage UnitGenerator::loadUnitFromSVGTemplate (const QString& path, const QByteArray& animation_name, const QColor& player_color)
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
