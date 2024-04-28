#pragma once

#include "matchstate.h"

#include <QSharedPointer>

class QOpenGLTexture;
class QString;
class QOpenGLFunctions;
class ColoredRenderer;
class TexturedRenderer;
class CoordMap;


class UnitRenderer
{
public:
    UnitRenderer (const QString& name, const QColor& team_color);
    void loadResources (const QColor& team_color);
    void draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, TexturedRenderer& textured_renderer, const Unit& unit, quint64 clock_ns,
               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    static QImage loadUnitFromSVGTemplate (const QString& path, const QByteArray& animation_name, const QColor& player_color);
    static QSharedPointer<QOpenGLTexture> loadTexture2D (const QString& path);
    static QSharedPointer<QOpenGLTexture> loadTexture2D (const QImage& image);
    static quint64 moveAnimationPeriodNS (Unit::Type type);
    static quint64 attackAnimationPeriodNS (Unit::Type type);

private:
    QString name;
    QSharedPointer<QOpenGLTexture> standing;
    QSharedPointer<QOpenGLTexture> walking1;
    QSharedPointer<QOpenGLTexture> walking2;
    QSharedPointer<QOpenGLTexture> shooting1;
    QSharedPointer<QOpenGLTexture> shooting2;
    QSharedPointer<QOpenGLTexture> contaminator_cooldown_shade;
};
