#include "unitteamrenderer.h"

#include "unitrenderer.h"


UnitTeamRenderer::UnitTeamRenderer (const QColor& team_color)
{
    seal = QSharedPointer<UnitRenderer> (new UnitRenderer (Unit::Type::Seal, team_color));
    crusader = QSharedPointer<UnitRenderer> (new UnitRenderer (Unit::Type::Crusader, team_color));
    goon = QSharedPointer<UnitRenderer> (new UnitRenderer (Unit::Type::Goon, team_color));
    beetle = QSharedPointer<UnitRenderer> (new UnitRenderer (Unit::Type::Beetle, team_color));
    contaminator = QSharedPointer<UnitRenderer> (new UnitRenderer (Unit::Type::Contaminator, team_color));
}

void UnitTeamRenderer::draw (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Unit& unit, quint64 clock_ns,
                             const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    if (UnitRenderer* unit_renderer = selectUnitRenderer (unit))
        unit_renderer->draw (gl, textured_renderer, unit, clock_ns, ortho_matrix, coord_map);
}
void UnitTeamRenderer::drawSelection (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, const Unit& unit,
                                      const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    if (UnitRenderer* unit_renderer = selectUnitRenderer (unit))
        unit_renderer->drawSelection (gl, colored_renderer, unit, ortho_matrix, coord_map);
}
void UnitTeamRenderer::drawHPBar (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, const Unit& unit,
                                  const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    if (UnitRenderer* unit_renderer = selectUnitRenderer (unit))
        unit_renderer->drawHPBar (gl, colored_renderer, unit, ortho_matrix, coord_map);
}
UnitRenderer* UnitTeamRenderer::selectUnitRenderer (const Unit& unit)
{
    switch (unit.type) {
    case Unit::Type::Seal:
        return &*seal;
    case Unit::Type::Crusader:
        return &*crusader;
    case Unit::Type::Goon:
        return &*goon;
    case Unit::Type::Beetle:
        return &*beetle;
    case Unit::Type::Contaminator:
        return &*contaminator;
    default:
        return nullptr;
    }
}
