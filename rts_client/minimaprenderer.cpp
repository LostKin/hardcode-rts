#include "minimaprenderer.h"

#include "coordmap.h"
#include "matchstate.h"
#include "coloredrenderer.h"
#include "hud.h"

#include <QColor>


MinimapRenderer::MinimapRenderer ()
{
}

void MinimapRenderer::draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                            HUD& hud, MatchState& match_state, Unit::Team team,
                            const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    const Rectangle& area = match_state.areaRef ();
    Scale area_to_minimap_scale (hud.minimap_screen_area.width () / area.width (), hud.minimap_screen_area.height () / area.height ());
    colored_renderer.fillRectangle (gl, hud.minimap_screen_area, QColor (), ortho_matrix);
    const std::map<quint32, Unit>& units = match_state.unitsRef ();
    for (std::map<quint32, Unit>::const_iterator it = units.cbegin (); it != units.cend (); ++it) {
        const Unit& unit = it->second;
        Position pos = hud.minimap_screen_area.topLeft () + (unit.position - area.topLeft ()) * area_to_minimap_scale;
        QColor color = (team == unit.team) ? QColor (0, 0xff, 0) : QColor (0xff, 0, 0);
        if (unit.type == Unit::Type::Contaminator)
            colored_renderer.fillRectangle (gl, pos.x () - 1.5, pos.y () - 1.5, 3.0, 3.0, color, ortho_matrix);
        else
            colored_renderer.fillRectangle (gl, pos.x () - 1.0, pos.y () - 1.0, 2.0, 2.0, color, ortho_matrix);
    }
    QColor color (0xdf, 0xdf, 0xff);
    qreal scale = coord_map.viewport_scale * coord_map.arena_viewport.height () / coord_map.POINTS_PER_VIEWPORT_VERTICALLY;
    Position viewport_center_minimap = hud.minimap_screen_area.topLeft () + (coord_map.viewport_center - area.topLeft ()) * area_to_minimap_scale;
    Offset s = Offset (coord_map.arena_viewport.width ()/scale, coord_map.arena_viewport.height ()/scale) * area_to_minimap_scale;
    gl.glScissor (hud.minimap_screen_area.left (), coord_map.viewport_size.height () - (hud.minimap_screen_area.top () + hud.minimap_screen_area.height ()),
                  hud.minimap_screen_area.width (), hud.minimap_screen_area.height ());
    gl.glEnable (GL_SCISSOR_TEST);
    colored_renderer.drawRectangle (gl, viewport_center_minimap.x () - s.dX () * 0.5, viewport_center_minimap.y () - s.dY () * 0.5, s.dX (), s.dY (), color, ortho_matrix);
    gl.glDisable (GL_SCISSOR_TEST);
}
