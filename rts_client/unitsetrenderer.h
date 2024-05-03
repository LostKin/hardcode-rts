#pragma once

#include "matchstate.h"

#include <QSharedPointer>

class UnitTeamRenderer;
class QOpenGLTexture;
class QString;
class QOpenGLFunctions;
class ColoredRenderer;
class TexturedRenderer;
class CoordMap;


class UnitSetRenderer
{
public:
    UnitSetRenderer (const QColor& red_team_color, const QColor& red_blue_color);
    void draw (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Unit& unit, quint64 clock_ns,
               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawCorpse (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Corpse& corpse,
                     const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawSelection (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, const Unit& unit,
                        const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawHPBar (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, const Unit& unit,
                    const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    UnitTeamRenderer* selectUnitTeamRenderer (const Unit& unit);

private:
    QSharedPointer<UnitTeamRenderer> red;
    QSharedPointer<UnitTeamRenderer> blue;
};
