#pragma once

#include "matchstate.h"

#include <QSharedPointer>

class UnitRenderer;
class QOpenGLTexture;
class QString;
class QOpenGLFunctions;
class ColoredRenderer;
class TexturedRenderer;
class ColoredTexturedRenderer;
class CoordMap;


class UnitTeamRenderer
{
public:
    UnitTeamRenderer (const QColor& team_color);
    void draw (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, ColoredTexturedRenderer& colored_textured_renderer, const Unit& unit, quint64 clock_ns,
               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawCorpse (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Corpse& corpse,
                     const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawSelection (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, const Unit& unit,
                        const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawHPBar (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, const Unit& unit,
                    const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    UnitRenderer* selectUnitRenderer (const Unit& unit);

private:
    QSharedPointer<UnitRenderer> seal;
    QSharedPointer<UnitRenderer> crusader;
    QSharedPointer<UnitRenderer> goon;
    QSharedPointer<UnitRenderer> beetle;
    QSharedPointer<UnitRenderer> contaminator;
};
