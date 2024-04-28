#include "unitsetrenderer.h"

#include "unitteamrenderer.h"


UnitSetRenderer::UnitSetRenderer (const QColor& red_team_color, const QColor& blue_team_color)
{
    red = QSharedPointer<UnitTeamRenderer> (new UnitTeamRenderer (red_team_color));
    blue = QSharedPointer<UnitTeamRenderer> (new UnitTeamRenderer (blue_team_color));
}

void UnitSetRenderer::draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, TexturedRenderer& textured_renderer, const Unit& unit, quint64 clock_ns,
                            const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    switch (unit.team) {
    case Unit::Team::Red:
        red->draw (gl, colored_renderer, textured_renderer, unit, clock_ns, ortho_matrix, coord_map);
        break;
    case Unit::Team::Blue:
        blue->draw (gl, colored_renderer, textured_renderer, unit, clock_ns, ortho_matrix, coord_map);
        break;
    default:
        break;
    }
}
