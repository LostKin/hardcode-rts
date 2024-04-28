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

void UnitTeamRenderer::draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, TexturedRenderer& textured_renderer, const Unit& unit, quint64 clock_ns,
                             const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    switch (unit.type) {
    case Unit::Type::Seal:
        seal->draw (gl, colored_renderer, textured_renderer, unit, clock_ns, ortho_matrix, coord_map);
        break;
    case Unit::Type::Crusader:
        crusader->draw (gl, colored_renderer, textured_renderer, unit, clock_ns, ortho_matrix, coord_map);
        break;
    case Unit::Type::Goon:
        goon->draw (gl, colored_renderer, textured_renderer, unit, clock_ns, ortho_matrix, coord_map);
        break;
    case Unit::Type::Beetle:
        beetle->draw (gl, colored_renderer, textured_renderer, unit, clock_ns, ortho_matrix, coord_map);
        break;
    case Unit::Type::Contaminator:
        contaminator->draw (gl, colored_renderer, textured_renderer, unit, clock_ns, ortho_matrix, coord_map);
        break;
    default:
        break;
    }
}
