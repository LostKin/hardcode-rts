#include "actionpanelrenderer.h"

#include "coordmap.h"
#include "matchstate.h"
#include "coloredrenderer.h"
#include "texturedrenderer.h"
#include "hud.h"

#include <QOpenGLTexture>


ActionPanelRenderer::ActionPanelRenderer ()
{
    textures.basic.move = loadTexture2D (":/images/actions/move.png");
    textures.basic.stop = loadTexture2D (":/images/actions/stop.png");
    textures.basic.hold = loadTexture2D (":/images/actions/hold.png");
    textures.basic.attack = loadTexture2D (":/images/actions/attack.png");
    textures.basic.pestilence = loadTexture2D (":/images/actions/pestilence.png");
    textures.basic.spawn = loadTexture2D (":/images/actions/spawn.png");

    textures.active.move = loadTexture2D (":/images/actions/move-active.png");
    textures.active.stop = loadTexture2D (":/images/actions/stop-active.png");
    textures.active.hold = loadTexture2D (":/images/actions/hold-active.png");
    textures.active.attack = loadTexture2D (":/images/actions/attack-active.png");
    textures.active.pestilence = loadTexture2D (":/images/actions/pestilence-active.png");
    textures.active.spawn = loadTexture2D (":/images/actions/spawn-active.png");
}

void ActionPanelRenderer::draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, TexturedRenderer& textured_renderer,
                                HUD& hud, int margin, const QColor& panel_color, int selected_count, quint64 active_actions, bool contaminator_selected, qint64 cast_cooldown_left_ticks,
                                const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    int area_w = hud.action_panel_rect.width ();
    int area_h = hud.action_panel_rect.height ();
    colored_renderer.fillRectangle (gl, coord_map.viewport_size.width () - area_w - margin, coord_map.viewport_size.height () - area_h - margin, area_w, area_h, QColor (panel_color), ortho_matrix);

    if (selected_count > 0) {
        drawActionButton (gl, colored_renderer, textured_renderer,
                          {coord_map.viewport_size.width () - area_w - margin, coord_map.viewport_size.height () - area_h - margin,
                           hud.action_button_size.width (), hud.action_button_size.height ()},
                          hud.pressed_action_button == ActionButtonId::Move,
                          (active_actions & (1 << quint64 (ActionButtonId::Move))) ? textures.active.move.get () : textures.basic.move.get (),
                          ortho_matrix);
        drawActionButton (gl, colored_renderer, textured_renderer,
                          {coord_map.viewport_size.width () - area_w - margin + hud.action_button_size.width (), coord_map.viewport_size.height () - area_h - margin,
                           hud.action_button_size.width (), hud.action_button_size.height ()},
                          hud.pressed_action_button == ActionButtonId::Stop,
                          (active_actions & (1 << quint64 (ActionButtonId::Stop))) ? textures.active.stop.get () : textures.basic.stop.get (),
                          ortho_matrix);
        drawActionButton (gl, colored_renderer, textured_renderer,
                          {coord_map.viewport_size.width () - area_w - margin + hud.action_button_size.width () * 2, coord_map.viewport_size.height () - area_h - margin,
                           hud.action_button_size.width (), hud.action_button_size.height ()},
                          hud.pressed_action_button == ActionButtonId::Hold,
                          (active_actions & (1 << quint64 (ActionButtonId::Hold))) ? textures.active.hold.get () : textures.basic.hold.get (),
                          ortho_matrix);
        drawActionButton (gl, colored_renderer, textured_renderer,
                          {coord_map.viewport_size.width () - area_w - margin + hud.action_button_size.width () * 4, coord_map.viewport_size.height () - area_h - margin,
                           hud.action_button_size.width (), hud.action_button_size.height ()},
                          hud.pressed_action_button == ActionButtonId::Attack,
                          (active_actions & (1 << quint64 (ActionButtonId::Attack))) ? textures.active.attack.get () : textures.basic.attack.get (),
                          ortho_matrix);

        if (contaminator_selected) {
            QRect pestilence_button_rect = {
                coord_map.viewport_size.width () - area_w - margin, coord_map.viewport_size.height () - area_h - margin + hud.action_button_size.height () * 2,
                hud.action_button_size.width (), hud.action_button_size.height (),
            };
            QRect spawn_button_rect = {
                coord_map.viewport_size.width () - area_w - margin + hud.action_button_size.width (), coord_map.viewport_size.height () - area_h - margin + hud.action_button_size.height () * 2,
                hud.action_button_size.width (), hud.action_button_size.height (),
            };
            drawActionButton (gl, colored_renderer, textured_renderer,
                              pestilence_button_rect, hud.pressed_action_button == ActionButtonId::Pestilence,
                              (active_actions & (1 << quint64 (ActionButtonId::Pestilence))) ? textures.active.pestilence.get () : textures.basic.pestilence.get (),
                              ortho_matrix);
            drawActionButton (gl, colored_renderer, textured_renderer,
                              spawn_button_rect, hud.pressed_action_button == ActionButtonId::Spawn,
                              (active_actions & (1 << quint64 (ActionButtonId::Spawn))) ? textures.active.spawn.get () : textures.basic.spawn.get (),
                              ortho_matrix);
            if (cast_cooldown_left_ticks) {
                qreal max_cooldown_ticks = qMax (MatchState::effectAttackDescription (AttackDescription::Type::PestilenceMissile).cooldown_ticks,
                                                 MatchState::effectAttackDescription (AttackDescription::Type::SpawnBeetle).cooldown_ticks);
                qreal remaining = qreal (cast_cooldown_left_ticks) / max_cooldown_ticks;
                drawActionButtonShade (gl, colored_renderer,
                                       pestilence_button_rect, hud.pressed_action_button == ActionButtonId::Pestilence, remaining,
                                       ortho_matrix);
                drawActionButtonShade (gl, colored_renderer,
                                       spawn_button_rect, hud.pressed_action_button == ActionButtonId::Spawn, remaining,
                                       ortho_matrix);
            }
        }
    }
}
void ActionPanelRenderer::drawActionButton (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, TexturedRenderer& textured_renderer,
                                            const QRect& rect, bool pressed, QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix)
{
    static const QColor bright (0x50, 0x0f, 0x94);
    static const QColor light (0xa8, 0x72, 0xe0);
    static const QColor dark (0x31, 0x1e, 0x44);
    static const QColor gray (0x5b, 0x46, 0x72);

    qreal outer_factor = pressed ? 0.2 : 0.1;
    qreal inner_factor = pressed ? (1.0 / 3.0) : 0.25;

    QPointF o1 (rect.x () + rect.width () * (outer_factor * 0.5), rect.y () + rect.height () * (outer_factor * 0.5));
    QPointF o2 (rect.x () + rect.width () * (1 - outer_factor * 0.5), rect.y () + rect.height () * (outer_factor * 0.5));
    QPointF o3 (rect.x () + rect.width () * (outer_factor * 0.5), rect.y () + rect.height () * (1 - outer_factor * 0.5));
    QPointF o4 (rect.x () + rect.width () * (1 - outer_factor * 0.5), rect.y () + rect.height () * (1 - outer_factor * 0.5));

    QPointF i1 (rect.x () + rect.width () * (inner_factor * 0.5), rect.y () + rect.height () * (inner_factor * 0.5));
    QPointF i2 (rect.x () + rect.width () * (1 - inner_factor * 0.5), rect.y () + rect.height () * (inner_factor * 0.5));
    QPointF i3 (rect.x () + rect.width () * (inner_factor * 0.5), rect.y () + rect.height () * (1 - inner_factor * 0.5));
    QPointF i4 (rect.x () + rect.width () * (1 - inner_factor * 0.5), rect.y () + rect.height () * (1 - inner_factor * 0.5));

    {
        const GLfloat vertices[] = {
            GLfloat (i1.x ()),
            GLfloat (i1.y ()),
            GLfloat (i2.x ()),
            GLfloat (i2.y ()),
            GLfloat (i3.x ()),
            GLfloat (i3.y ()),
            GLfloat (i4.x ()),
            GLfloat (i4.y ()),

            GLfloat (o1.x ()),
            GLfloat (o1.y ()),
            GLfloat (o2.x ()),
            GLfloat (o2.y ()),
            GLfloat (i1.x ()),
            GLfloat (i1.y ()),
            GLfloat (i2.x ()),
            GLfloat (i2.y ()),

            GLfloat (i3.x ()),
            GLfloat (i3.y ()),
            GLfloat (i4.x ()),
            GLfloat (i4.y ()),
            GLfloat (o3.x ()),
            GLfloat (o3.y ()),
            GLfloat (o4.x ()),
            GLfloat (o4.y ()),

            GLfloat (o1.x ()),
            GLfloat (o1.y ()),
            GLfloat (i1.x ()),
            GLfloat (i1.y ()),
            GLfloat (o3.x ()),
            GLfloat (o3.y ()),
            GLfloat (i3.x ()),
            GLfloat (i3.y ()),

            GLfloat (i2.x ()),
            GLfloat (i2.y ()),
            GLfloat (o2.x ()),
            GLfloat (o2.y ()),
            GLfloat (i4.x ()),
            GLfloat (i4.y ()),
            GLfloat (o4.x ()),
            GLfloat (o4.y ()),
        };

        const GLfloat colors[] = {
            GLfloat (bright.redF ()),
            GLfloat (bright.greenF ()),
            GLfloat (bright.blueF ()),
            GLfloat (1),
            GLfloat (bright.redF ()),
            GLfloat (bright.greenF ()),
            GLfloat (bright.blueF ()),
            GLfloat (1),
            GLfloat (bright.redF ()),
            GLfloat (bright.greenF ()),
            GLfloat (bright.blueF ()),
            GLfloat (1),
            GLfloat (bright.redF ()),
            GLfloat (bright.greenF ()),
            GLfloat (bright.blueF ()),
            GLfloat (1),

            GLfloat (gray.redF ()),
            GLfloat (gray.greenF ()),
            GLfloat (gray.blueF ()),
            GLfloat (1),
            GLfloat (gray.redF ()),
            GLfloat (gray.greenF ()),
            GLfloat (gray.blueF ()),
            GLfloat (1),
            GLfloat (gray.redF ()),
            GLfloat (gray.greenF ()),
            GLfloat (gray.blueF ()),
            GLfloat (1),
            GLfloat (gray.redF ()),
            GLfloat (gray.greenF ()),
            GLfloat (gray.blueF ()),
            GLfloat (1),

            GLfloat (dark.redF ()),
            GLfloat (dark.greenF ()),
            GLfloat (dark.blueF ()),
            GLfloat (1),
            GLfloat (dark.redF ()),
            GLfloat (dark.greenF ()),
            GLfloat (dark.blueF ()),
            GLfloat (1),
            GLfloat (dark.redF ()),
            GLfloat (dark.greenF ()),
            GLfloat (dark.blueF ()),
            GLfloat (1),
            GLfloat (dark.redF ()),
            GLfloat (dark.greenF ()),
            GLfloat (dark.blueF ()),
            GLfloat (1),

            GLfloat (light.redF ()),
            GLfloat (light.greenF ()),
            GLfloat (light.blueF ()),
            GLfloat (1),
            GLfloat (light.redF ()),
            GLfloat (light.greenF ()),
            GLfloat (light.blueF ()),
            GLfloat (1),
            GLfloat (light.redF ()),
            GLfloat (light.greenF ()),
            GLfloat (light.blueF ()),
            GLfloat (1),
            GLfloat (light.redF ()),
            GLfloat (light.greenF ()),
            GLfloat (light.blueF ()),
            GLfloat (1),

            GLfloat (light.redF ()),
            GLfloat (light.greenF ()),
            GLfloat (light.blueF ()),
            GLfloat (1),
            GLfloat (light.redF ()),
            GLfloat (light.greenF ()),
            GLfloat (light.blueF ()),
            GLfloat (1),
            GLfloat (light.redF ()),
            GLfloat (light.greenF ()),
            GLfloat (light.blueF ()),
            GLfloat (1),
            GLfloat (light.redF ()),
            GLfloat (light.greenF ()),
            GLfloat (light.blueF ()),
            GLfloat (1),
        };

        static const GLuint indices[] = {
            0,
            1,
            3,
            0,
            3,
            2,
            4,
            5,
            7,
            4,
            7,
            6,
            8,
            9,
            11,
            8,
            11,
            10,
            12,
            13,
            15,
            12,
            15,
            14,
            16,
            17,
            19,
            16,
            19,
            18,
        };

        colored_renderer.draw (gl, GL_TRIANGLES, vertices, colors, sizeof (indices) / sizeof (indices[0]), indices, ortho_matrix);
    }

    {
        const GLfloat vertices[] = {
            GLfloat (i1.x ()),
            GLfloat (i1.y ()),
            GLfloat (i2.x ()),
            GLfloat (i2.y ()),
            GLfloat (i3.x ()),
            GLfloat (i3.y ()),
            GLfloat (i4.x ()),
            GLfloat (i4.y ()),
        };

        static const GLfloat texture_coords[] = {
            0,
            0,
            1,
            0,
            0,
            1,
            1,
            1,
        };

        static const GLuint indices[] = {
            0,
            1,
            3,
            0,
            3,
            2,
        };

        textured_renderer.draw (gl, GL_TRIANGLES, vertices, texture_coords, 6, indices, texture, ortho_matrix);
    }
}
void ActionPanelRenderer::drawActionButtonShade (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                                                 const QRect& rect, bool pressed, qreal remaining, const QMatrix4x4& ortho_matrix)
{
    if (remaining <= 0.0)
        return;

    static const QColor color (0, 0, 0, 0xcc);

    qreal outer_factor = pressed ? 0.2 : 0.1;

    QPointF center = QRectF (rect).center ();
    QPointF top_left = {rect.x () + rect.width () * (outer_factor * 0.5), rect.y () + rect.height () * (outer_factor * 0.5)};
    QPointF top_right = {rect.x () + rect.width () * (1 - outer_factor * 0.5), rect.y () + rect.height () * (outer_factor * 0.5)};
    QPointF bottom_left = {rect.x () + rect.width () * (outer_factor * 0.5), rect.y () + rect.height () * (1 - outer_factor * 0.5)};
    QPointF bottom_right = {rect.x () + rect.width () * (1 - outer_factor * 0.5), rect.y () + rect.height () * (1 - outer_factor * 0.5)};

    QVector<GLfloat> vertices;
    QVector<GLfloat> colors;
    for (size_t i = 0; i < 15; ++i)
        colors.append ({
                GLfloat (color.redF ()),
                GLfloat (color.greenF ()),
                GLfloat (color.blueF ()),
                GLfloat (color.alphaF ()),
            });

    qreal angle = M_PI * remaining;

    do {
        if (angle < M_PI * 0.25) {
            qreal scale = rect.height () * (1 - outer_factor) * 0.5;
            qreal off_x = -scale * qTan (angle);
            qreal off_y = -scale;
            vertices.append ({GLfloat (center.x ()), GLfloat (center.y ()),
                    GLfloat (center.x () + off_x), GLfloat (center.y () + off_y),
                    GLfloat (center.x () - off_x), GLfloat (center.y () + off_y)});
            break;
        } else {
            vertices.append ({GLfloat (center.x ()), GLfloat (center.y ()),
                    GLfloat (top_right.x ()), GLfloat (top_right.y ()),
                    GLfloat (top_left.x ()), GLfloat (top_left.y ())});
        }

        if (angle < M_PI * 0.75) {
            qreal scale = rect.height () * (1 - outer_factor) * 0.5;
            qreal off_x = -scale;
            qreal off_y = -scale / qTan (angle);
            vertices.append ({GLfloat (top_left.x ()), GLfloat (top_left.y ()),
                    GLfloat (center.x ()), GLfloat (center.y ()),
                    GLfloat (center.x () + off_x), GLfloat (center.y () + off_y),
                    GLfloat (top_right.x ()), GLfloat (top_right.y ()),
                    GLfloat (center.x ()), GLfloat (center.y ()),
                    GLfloat (center.x () - off_x), GLfloat (center.y () + off_y)});
            break;
        } else {
            vertices.append ({GLfloat (top_left.x ()), GLfloat (top_left.y ()),
                    GLfloat (center.x ()), GLfloat (center.y ()),
                    GLfloat (bottom_left.x ()), GLfloat (bottom_left.y ()),
                    GLfloat (top_right.x ()), GLfloat (top_right.y ()),
                    GLfloat (center.x ()), GLfloat (center.y ()),
                    GLfloat (bottom_right.x ()), GLfloat (bottom_right.y ())});
        }

        {
            qreal scale = rect.height () * (1 - outer_factor) * 0.5;
            qreal off_x = scale * qTan (angle);
            qreal off_y = scale;
            vertices.append ({GLfloat (center.x ()), GLfloat (center.y ()),
                    GLfloat (bottom_left.x ()), GLfloat (bottom_left.y ()),
                    GLfloat (center.x () + off_x), GLfloat (center.y () + off_y),
                    GLfloat (center.x ()), GLfloat (center.y ()),
                    GLfloat (bottom_right.x ()), GLfloat (bottom_right.y ()),
                    GLfloat (center.x () - off_x), GLfloat (center.y () + off_y)});
        }
    } while (0);

    colored_renderer.draw (gl, GL_TRIANGLES, vertices.size () / 2, vertices.data (), colors.data (), ortho_matrix);
}
QSharedPointer<QOpenGLTexture> ActionPanelRenderer::loadTexture2D (const QString& path)
{
    QOpenGLTexture* texture = new QOpenGLTexture (QImage (path));
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
