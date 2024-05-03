#include "scenerenderer.h"

#include "coordmap.h"
#include "matchstate.h"
#include "coloredrenderer.h"
#include "coloredtexturedrenderer.h"
#include "texturedrenderer.h"
#include "unitsetrenderer.h"
#include "effectrenderer.h"

#include <QOpenGLTexture>


SceneRenderer::SceneRenderer ()
{
    unit_set_renderer = QSharedPointer<UnitSetRenderer>::create (red_player_color, blue_player_color);
    effect_renderer = QSharedPointer<EffectRenderer>::create ();
    textures.ground = loadTexture2D (":/images/ground.png");
}

void SceneRenderer::draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, ColoredTexturedRenderer& colored_textured_renderer, TexturedRenderer& textured_renderer,
                          MatchState& match_state, Unit::Team team, const QPoint& cursor_position, const std::optional<QPoint>& selection_start,
                          const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    drawBackground (gl, textured_renderer, match_state, ortho_matrix, coord_map);
    drawCorpses (gl, textured_renderer, match_state, ortho_matrix, coord_map);
    drawUnits (gl, textured_renderer, match_state, ortho_matrix, coord_map);
    drawUnitSelection (gl, colored_renderer, match_state, ortho_matrix, coord_map);
    drawEffects (gl, colored_textured_renderer, textured_renderer, match_state, ortho_matrix, coord_map);
    drawUnitPaths (gl, colored_renderer, match_state, team, ortho_matrix, coord_map);
    drawUnitStats (gl, colored_renderer, match_state, ortho_matrix, coord_map);
    drawSelectionBar (gl, colored_renderer, cursor_position, selection_start, ortho_matrix);
}
void SceneRenderer::drawBackground (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer,
                                    MatchState& match_state,
                                    const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    const QRectF& area = match_state.areaRef ();

    qreal scale = coord_map.viewport_scale * coord_map.arena_viewport.height () / coord_map.POINTS_PER_VIEWPORT_VERTICALLY;
    QPointF center = coord_map.arena_viewport_center - coord_map.viewport_center*scale;
    const GLfloat vertices[] = {
        GLfloat (center.x () + scale * area.left ()),
        GLfloat (center.y () + scale * area.top ()),
        GLfloat (center.x () + scale * area.right ()),
        GLfloat (center.y () + scale * area.top ()),
        GLfloat (center.x () + scale * area.right ()),
        GLfloat (center.y () + scale * area.bottom ()),
        GLfloat (center.x () + scale * area.left ()),
        GLfloat (center.y () + scale * area.bottom ()),
    };

    static const GLfloat texture_coords[] = {
        0,
        0,
        1,
        0,
        1,
        1,
        0,
        1,
    };

    static const GLuint indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    textured_renderer.draw (gl, GL_TRIANGLES, vertices, texture_coords, 6, indices, textures.ground.get (), ortho_matrix);
}
void SceneRenderer::drawCorpses (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer,
                                 MatchState& match_state,
                                 const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    const QHash<quint32, Corpse>& corpses = match_state.corpsesRef ();
    for (QHash<quint32, Corpse>::const_iterator it = corpses.constBegin (); it != corpses.constEnd (); ++it)
        unit_set_renderer->drawCorpse (gl, textured_renderer, it.value (), ortho_matrix, coord_map);
}
void SceneRenderer::drawUnits (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer,
                               MatchState& match_state,
                               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    const QHash<quint32, Unit>& units = match_state.unitsRef ();
    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it)
        unit_set_renderer->draw (gl, textured_renderer, it.value (), match_state.clockNS (), ortho_matrix, coord_map);
}
void SceneRenderer::drawUnitSelection (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                                       MatchState& match_state,
                                       const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    const QHash<quint32, Unit>& units = match_state.unitsRef ();
    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it)
        unit_set_renderer->drawSelection (gl, colored_renderer, it.value (), ortho_matrix, coord_map);
}
void SceneRenderer::drawEffects (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer, TexturedRenderer& textured_renderer,
                                 MatchState& match_state,
                                 const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    const QHash<quint32, Missile>& missiles = match_state.missilesRef ();
    for (QHash<quint32, Missile>::const_iterator it = missiles.constBegin (); it != missiles.constEnd (); ++it)
        effect_renderer->drawMissile (gl, textured_renderer, it.value (), match_state.clockNS (), ortho_matrix, coord_map);

    const QHash<quint32, Explosion>& explosions = match_state.explosionsRef ();
    for (QHash<quint32, Explosion>::const_iterator it = explosions.constBegin (); it != explosions.constEnd (); ++it)
        effect_renderer->drawExplosion (gl, colored_textured_renderer, it.value (), match_state.clockNS (), ortho_matrix, coord_map);
}
void SceneRenderer::drawUnitPaths (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                                   MatchState& match_state, Unit::Team team,
                                   const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    QVector<GLfloat> vertices;
    QVector<GLfloat> colors;
    const QHash<quint32, Unit>& units = match_state.unitsRef ();
    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it) {
        const Unit& unit = *it;
        const QPointF* target_position;
        if (unit.team == team && (target_position = getUnitTargetPosition (unit, match_state))) {
            QPointF current = coord_map.toScreenCoords (unit.position);
            QPointF target = coord_map.toScreenCoords (*target_position);
            vertices.append ({
                    GLfloat (current.x ()),
                    GLfloat (current.y ()),
                    GLfloat (target.x ()),
                    GLfloat (target.y ()),
                });
            if (std::holds_alternative<AttackAction> (unit.action))
                colors.append ({1, 0, 0, 1, 1, 0, 0, 1});
            else
                colors.append ({0, 1, 0, 1, 0, 1, 0, 1});
        }
    }
    if (vertices.size ()) {
        glEnable (GL_LINE_SMOOTH);
        colored_renderer.draw (gl, GL_LINES, vertices.size ()/2, vertices.data (), colors.data (), ortho_matrix);
        glDisable (GL_LINE_SMOOTH);
    }
}
void SceneRenderer::drawUnitStats (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                                   MatchState& match_state,
                                   const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    const QHash<quint32, Unit>& units = match_state.unitsRef ();
    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it)
        unit_set_renderer->drawHPBar (gl, colored_renderer, it.value (), ortho_matrix, coord_map);
}
void SceneRenderer::drawSelectionBar (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                                      const QPoint& cursor_position, const std::optional<QPoint>& selection_start,
                                      const QMatrix4x4& ortho_matrix)
{
    if (selection_start.has_value () && *selection_start != cursor_position) {
        colored_renderer.drawRectangle (
            gl,
            qMin (selection_start->x (), cursor_position.x ()), qMin (selection_start->y (), cursor_position.y ()),
            qAbs (selection_start->x () - cursor_position.x ()), qAbs (selection_start->y () - cursor_position.y ()),
            QColor (0, 255, 0, 255),
            ortho_matrix
        );
    }
}
const QPointF* SceneRenderer::getUnitTargetPosition (const Unit& unit, MatchState& match_state)
{
    if (std::holds_alternative<MoveAction> (unit.action)) {
        const std::variant<QPointF, quint32>& action_target = std::get<MoveAction> (unit.action).target;
        if (std::holds_alternative<QPointF> (action_target)) {
            return &std::get<QPointF> (action_target);
        } else if (std::holds_alternative<quint32> (action_target)) {
            quint32 target_unit_id = std::get<quint32> (action_target);
            const QHash<quint32, Unit>& units = match_state.unitsRef ();
            QHash<quint32, Unit>::const_iterator target_unit_it = units.find (target_unit_id);
            if (target_unit_it != units.end ()) {
                const Unit& target_unit = *target_unit_it;
                return &target_unit.position;
            }
        }
    } else if (std::holds_alternative<AttackAction> (unit.action)) {
        const std::variant<QPointF, quint32>& action_target = std::get<AttackAction> (unit.action).target;
        if (std::holds_alternative<QPointF> (action_target)) {
            return &std::get<QPointF> (action_target);
        } else if (std::holds_alternative<quint32> (action_target)) {
            quint32 target_unit_id = std::get<quint32> (action_target);
            const QHash<quint32, Unit>& units = match_state.unitsRef ();
            QHash<quint32, Unit>::const_iterator target_unit_it = units.find (target_unit_id);
            if (target_unit_it != units.end ()) {
                const Unit& target_unit = *target_unit_it;
                return &target_unit.position;
            }
        }
    }
    return nullptr;
}
QSharedPointer<QOpenGLTexture> SceneRenderer::loadTexture2D (const QString& path)
{
    QOpenGLTexture* texture = new QOpenGLTexture (QImage (path));
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
