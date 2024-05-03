#include "unitsetrenderer.h"

#include "unitteamrenderer.h"


UnitSetRenderer::UnitSetRenderer (const QColor& red_team_color, const QColor& blue_team_color)
{
    red = QSharedPointer<UnitTeamRenderer> (new UnitTeamRenderer (red_team_color));
    blue = QSharedPointer<UnitTeamRenderer> (new UnitTeamRenderer (blue_team_color));
}

void UnitSetRenderer::draw (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Unit& unit, quint64 clock_ns,
                            const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    if (UnitTeamRenderer* unit_team_renderer = selectUnitTeamRenderer (unit))
        unit_team_renderer->draw (gl, textured_renderer, unit, clock_ns, ortho_matrix, coord_map);
}
void UnitSetRenderer::drawCorpse (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Corpse& corpse,
                                  const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    if (UnitTeamRenderer* unit_team_renderer = selectUnitTeamRenderer (corpse.unit))
        unit_team_renderer->drawCorpse (gl, textured_renderer, corpse, ortho_matrix, coord_map);
}
void UnitSetRenderer::drawSelection (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, const Unit& unit,
                                     const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    if (UnitTeamRenderer* unit_team_renderer = selectUnitTeamRenderer (unit))
        unit_team_renderer->drawSelection (gl, colored_renderer, unit, ortho_matrix, coord_map);
}
void UnitSetRenderer::drawHPBar (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, const Unit& unit,
                                 const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    if (UnitTeamRenderer* unit_team_renderer = selectUnitTeamRenderer (unit))
        unit_team_renderer->drawHPBar (gl, colored_renderer, unit, ortho_matrix, coord_map);
}
UnitTeamRenderer* UnitSetRenderer::selectUnitTeamRenderer (const Unit& unit)
{
    switch (unit.team) {
    case Unit::Team::Red:
        return &*red;
    case Unit::Team::Blue:
        return &*blue;
    default:
        return nullptr;
    }
}
