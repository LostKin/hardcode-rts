#pragma once

#include "matchstate.h"

#include <QSharedPointer>

class QOpenGLTexture;
class QString;
class QOpenGLFunctions;
class ColoredRenderer;
class TexturedRenderer;
class ColoredTexturedRenderer;
class CoordMap;


class UnitRenderer
{
public:
    UnitRenderer (Unit::Type type, const QColor& team_color);
    void draw (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, ColoredTexturedRenderer& colored_textured_renderer,
               const Unit& unit, quint64 clock_ns, const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawCorpse (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Corpse& corpse,
                     const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawSelection (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, const Unit& unit,
                        const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawHPBar (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, const Unit& unit,
                    const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    static QSharedPointer<QOpenGLTexture> loadTexture2D (const QString& path);
    static QSharedPointer<QOpenGLTexture> loadTexture2D (const QImage& image);
    static quint64 moveAnimationPeriodNS (Unit::Type type);
    static quint64 attackAnimationPeriodNS (Unit::Type type);
    static QColor getHPColor (qreal hp_ratio);

    void drawBody (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Unit& unit, quint64 clock_ns,
                   const QMatrix4x4& ortho_matrix, const CoordMap& coord_map, bool alive = true);
    void drawPestilenceDisease (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer, const Unit& unit, quint64 clock_ns,
                                const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawCooldownShade (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Unit& unit,
                            const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    QSharedPointer<QOpenGLTexture> standing;
    QSharedPointer<QOpenGLTexture> walking1;
    QSharedPointer<QOpenGLTexture> walking2;
    QSharedPointer<QOpenGLTexture> shooting1;
    QSharedPointer<QOpenGLTexture> shooting2;
    QSharedPointer<QOpenGLTexture> corpse;
    QSharedPointer<QOpenGLTexture> contaminator_cooldown_shade;
    QSharedPointer<QOpenGLTexture> pestilence_disease1;
    QSharedPointer<QOpenGLTexture> pestilence_disease2;
};
