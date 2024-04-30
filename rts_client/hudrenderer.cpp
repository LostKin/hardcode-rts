#include "hudrenderer.h"

#include "coordmap.h"
#include "matchstate.h"
#include "coloredrenderer.h"
#include "coloredtexturedrenderer.h"
#include "texturedrenderer.h"
#include "hud.h"
#include "uniticonset.h"
#include "actionpanelrenderer.h"
#include "minimaprenderer.h"
#include "groupoverlayrenderer.h"
#include "selectionpanelrenderer.h"


HUDRenderer::HUDRenderer ()
{
    unit_icon_set = QSharedPointer<UnitIconSet>::create ();
    action_panel_renderer = QSharedPointer<ActionPanelRenderer>::create ();
    minimap_renderer = QSharedPointer<MinimapRenderer>::create ();
    group_overlay_renderer = QSharedPointer<GroupOverlayRenderer>::create (unit_icon_set);
    selection_panel_renderer = QSharedPointer<SelectionPanelRenderer>::create (unit_icon_set);
}

void HUDRenderer::draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, ColoredTexturedRenderer& colored_textured_renderer, TexturedRenderer& textured_renderer, QPaintDevice& device,
                        HUD& hud, MatchState& match_state, Unit::Team team,
                        const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    static const QColor stroke_color (0, 0, 0);
    static const QColor margin_color (0x2f, 0x90, 0x92);
    static const QColor panel_color (0x21, 0x2f, 0x3a);
    static const QColor panel_inner_color (0, 0xaa, 0xaa);

    int margin = hud.margin;

    {
        int area_w = hud.minimap_panel_rect.width () + margin * 2;
        int area_h = hud.minimap_panel_rect.height () + margin * 2;
        colored_renderer.fillRectangle (gl, 0, coord_map.viewport_size.height () - area_h, area_w, area_h, margin_color, ortho_matrix);
    }
    {
        int area_w = hud.action_panel_rect.width () + margin * 2;
        int area_h = hud.action_panel_rect.height () + margin * 2;
        colored_renderer.fillRectangle (gl, coord_map.viewport_size.width () - area_w, coord_map.viewport_size.height () - area_h, area_w, area_h, margin_color, ortho_matrix);
    }
    {
        colored_renderer.fillRectangle (gl,
                                        hud.minimap_panel_rect.width () + margin * 2, coord_map.viewport_size.height () - hud.selection_panel_rect.height () - margin * 2,
                                        coord_map.viewport_size.width () - hud.minimap_panel_rect.width () - hud.action_panel_rect.width () - margin * 4, hud.selection_panel_rect.height () + margin * 2,
                                        margin_color, ortho_matrix);
    }
    colored_renderer.fillRectangle (gl, hud.minimap_panel_rect, panel_color, ortho_matrix);
    minimap_renderer->draw (gl, colored_renderer,
                            hud, match_state, team,
                            ortho_matrix, coord_map);
    colored_renderer.fillRectangle (gl, hud.selection_panel_rect, panel_color, ortho_matrix);

    {
        const qreal half_stroke_width = hud.stroke_width * 0.5;

        glLineWidth (hud.stroke_width);

        const GLfloat vertices[] = {
            GLfloat (margin - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud.minimap_panel_rect.height () - margin),
            GLfloat (margin + hud.minimap_panel_rect.width () + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud.minimap_panel_rect.height () - margin),
            GLfloat (margin - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (margin + hud.minimap_panel_rect.width () + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (margin),
            GLfloat (coord_map.viewport_size.height () - hud.minimap_panel_rect.height () - margin + half_stroke_width),
            GLfloat (margin),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),
            GLfloat (margin + hud.minimap_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - hud.minimap_panel_rect.height () - margin + half_stroke_width),
            GLfloat (margin + hud.minimap_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),

            GLfloat (margin * 2 + hud.minimap_panel_rect.width () - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud.selection_panel_rect.height () - margin),
            GLfloat (margin * 2 + hud.minimap_panel_rect.width () + hud.selection_panel_rect.width () + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud.selection_panel_rect.height () - margin),
            GLfloat (margin * 2 + hud.minimap_panel_rect.width () - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (margin * 2 + hud.minimap_panel_rect.width () + hud.selection_panel_rect.width () + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (margin * 2 + hud.minimap_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - hud.selection_panel_rect.height () - margin + half_stroke_width),
            GLfloat (margin * 2 + hud.minimap_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),
            GLfloat (margin * 2 + hud.minimap_panel_rect.width () + hud.selection_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - hud.selection_panel_rect.height () - margin + half_stroke_width),
            GLfloat (margin * 2 + hud.minimap_panel_rect.width () + hud.selection_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),

            GLfloat (coord_map.viewport_size.width () - hud.action_panel_rect.width () - margin - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud.action_panel_rect.height () - margin),
            GLfloat (coord_map.viewport_size.width () - margin + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud.action_panel_rect.height () - margin),
            GLfloat (coord_map.viewport_size.width () - hud.action_panel_rect.width () - margin - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (coord_map.viewport_size.width () - margin + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (coord_map.viewport_size.width () - hud.action_panel_rect.width () - margin),
            GLfloat (coord_map.viewport_size.height () - hud.action_panel_rect.height () - margin + half_stroke_width),
            GLfloat (coord_map.viewport_size.width () - hud.action_panel_rect.width () - margin),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),
            GLfloat (coord_map.viewport_size.width () - margin),
            GLfloat (coord_map.viewport_size.height () - hud.action_panel_rect.height () - margin + half_stroke_width),
            GLfloat (coord_map.viewport_size.width () - margin),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),

            GLfloat (0),
            GLfloat (coord_map.viewport_size.height () - hud.minimap_panel_rect.height () - margin * 2),
            GLfloat (hud.minimap_panel_rect.width () + margin * 2 + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud.minimap_panel_rect.height () - margin * 2),
            GLfloat (hud.minimap_panel_rect.width () + margin * 2),
            GLfloat (coord_map.viewport_size.height () - hud.minimap_panel_rect.height () - margin * 2 + half_stroke_width),
            GLfloat (hud.minimap_panel_rect.width () + margin * 2),
            GLfloat (coord_map.viewport_size.height () - hud.selection_panel_rect.height () - margin * 2 - half_stroke_width),
            GLfloat (hud.minimap_panel_rect.width () + margin * 2 - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud.selection_panel_rect.height () - margin * 2),
            GLfloat (coord_map.viewport_size.width () - hud.action_panel_rect.width () - margin * 2 + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud.selection_panel_rect.height () - margin * 2),
            GLfloat (coord_map.viewport_size.width () - hud.action_panel_rect.width () - margin * 2),
            GLfloat (coord_map.viewport_size.height () - hud.selection_panel_rect.height () - margin * 2 - half_stroke_width),
            GLfloat (coord_map.viewport_size.width () - hud.action_panel_rect.width () - margin * 2),
            GLfloat (coord_map.viewport_size.height () - hud.action_panel_rect.height () - margin * 2 + half_stroke_width),
            GLfloat (coord_map.viewport_size.width () - hud.action_panel_rect.width () - margin * 2 - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud.action_panel_rect.height () - margin * 2),
            GLfloat (coord_map.viewport_size.width ()),
            GLfloat (coord_map.viewport_size.height () - hud.action_panel_rect.height () - margin * 2),
            GLfloat (coord_map.viewport_size.width ()),
            GLfloat (coord_map.viewport_size.height () - hud.action_panel_rect.height () - margin * 2 + half_stroke_width),
            GLfloat (coord_map.viewport_size.width ()),
            GLfloat (coord_map.viewport_size.height () - half_stroke_width),
            GLfloat (coord_map.viewport_size.width ()),
            GLfloat (coord_map.viewport_size.height ()),
            GLfloat (0),
            GLfloat (coord_map.viewport_size.height ()),
            GLfloat (0),
            GLfloat (coord_map.viewport_size.height () - half_stroke_width),
            GLfloat (0),
            GLfloat (coord_map.viewport_size.height () - hud.minimap_panel_rect.height () - margin * 2 + half_stroke_width),
        };

        const GLfloat colors[] = {
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),

            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),

            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),

            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
        };

        colored_renderer.draw (gl, GL_LINES, 40, vertices, colors, ortho_matrix);

        glLineWidth (1);
    }

    size_t selected_count = 0;
    bool contaminator_selected = false;
    qint64 cast_cooldown_left_ticks = 0x7fffffffffffffffLL;
    quint64 active_actions = 0;
    const Unit* last_selected_unit = nullptr;
    const QHash<quint32, Unit>& units = match_state.unitsRef ();
    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it) {
        if (it->selected) {
            if (std::holds_alternative<AttackAction> (it->action)) {
                active_actions |= 1 << quint64 (ActionButtonId::Attack);
            } else if (std::holds_alternative<MoveAction> (it->action)) {
                active_actions |= 1 << quint64 (ActionButtonId::Move);
            } else if (std::holds_alternative<CastAction> (it->action)) {
                switch (std::get<CastAction> (it->action).type) {
                case CastAction::Type::Pestilence:
                    active_actions |= 1 << quint64 (ActionButtonId::Pestilence);
                    break;
                case CastAction::Type::SpawnBeetle:
                    active_actions |= 1 << quint64 (ActionButtonId::Spawn);
                    break;
                default:
                    break;
                }
            } else {
                active_actions |= 1 << quint64 (ActionButtonId::Stop);
            }

            if (it->type == Unit::Type::Contaminator) {
                contaminator_selected = true;
                cast_cooldown_left_ticks = qMin (cast_cooldown_left_ticks, it->cast_cooldown_left_ticks);
            }
            ++selected_count;
            last_selected_unit = &*it;
        }
    }

    selection_panel_renderer->draw (gl, colored_textured_renderer, device, hud.selection_panel_rect, selected_count, last_selected_unit, hud, match_state, ortho_matrix);
    group_overlay_renderer->draw (gl, colored_renderer, colored_textured_renderer, device, hud, match_state, ortho_matrix);
    action_panel_renderer->draw (gl, colored_renderer, textured_renderer,
                                 hud, margin, panel_color, selected_count, active_actions, contaminator_selected, cast_cooldown_left_ticks,
                                 ortho_matrix, coord_map);
}
