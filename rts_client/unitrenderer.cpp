#include "unitrenderer.h"

#include "coordmap.h"
#include "coloredrenderer.h"
#include "texturedrenderer.h"

#include <QFile>
#include <QSvgRenderer>
#include <QPainter>
#include <QOpenGLTexture>


static constexpr qreal SQRT_2 = 1.4142135623731;
static constexpr qreal SQRT_1_2 = 0.70710678118655;
static constexpr qreal PI_X_3_4 = 3.0 / 4.0 * M_PI;
static constexpr qreal PI_X_1_4 = 1.0 / 4.0 * M_PI;


UnitRenderer::UnitRenderer (Unit::Type type, const QColor& team_color)
{
    QString name;
    switch (type) {
    case Unit::Type::Seal:
        name = "seal";
        break;
    case Unit::Type::Crusader:
        name = "crusader";
        break;
    case Unit::Type::Goon:
        name = "goon";
        break;
    case Unit::Type::Beetle:
        name = "beetle";
        break;
    case Unit::Type::Contaminator:
        name = "contaminator";
        break;
    default:
        return;
    }
    standing = loadTexture2D (loadUnitFromSVGTemplate (":/images/units/" + name + "/unit_tmpl.svg", "standing", team_color));
    walking1 = loadTexture2D (loadUnitFromSVGTemplate (":/images/units/" + name + "/unit_tmpl.svg", "walking1", team_color));
    walking2 = loadTexture2D (loadUnitFromSVGTemplate (":/images/units/" + name + "/unit_tmpl.svg", "walking2", team_color));
    shooting1 = loadTexture2D (loadUnitFromSVGTemplate (":/images/units/" + name + "/unit_tmpl.svg", "attacking1", team_color));
    shooting2 = loadTexture2D (loadUnitFromSVGTemplate (":/images/units/" + name + "/unit_tmpl.svg", "attacking2", team_color));
    contaminator_cooldown_shade = loadTexture2D (":/images/units/contaminator/cooldown-shade.png");
}

void UnitRenderer::draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, TexturedRenderer& textured_renderer, const Unit& unit, quint64 clock_ns,
                         const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    qreal sprite_scale;
    switch (unit.type) {
    case Unit::Type::Seal:
        sprite_scale = 0.8;
        break;
    case Unit::Type::Crusader:
        sprite_scale = 1.0;
        break;
    case Unit::Type::Goon:
        sprite_scale = 0.8;
        break;
    case Unit::Type::Beetle:
        sprite_scale = 0.8;
        break;
    case Unit::Type::Contaminator:
        sprite_scale = 0.8;
        break;
    default:
        return;
    }

    QOpenGLTexture* texture;
    if (unit.attack_remaining_ticks) {
        quint64 period = attackAnimationPeriodNS (unit.type);
        quint64 phase = (clock_ns + unit.phase_offset) % period;
        texture = (phase < period / 2) ? shooting1.get () : shooting2.get ();
    } else if (std::holds_alternative<MoveAction> (unit.action) || std::holds_alternative<AttackAction> (unit.action)) {
        quint64 period = moveAnimationPeriodNS (unit.type);
        quint64 phase = (clock_ns + unit.phase_offset) % period;
        texture = (phase < period / 2) ? walking1.get () : walking2.get ();
    } else {
        texture = standing.get ();
    }

    QPointF center = coord_map.toScreenCoords (unit.position);

    qreal a1_sin, a1_cos;
    qreal a2_sin, a2_cos;
    qreal a3_sin, a3_cos;
    qreal a4_sin, a4_cos;
    sincos (unit.orientation + PI_X_3_4, &a1_sin, &a1_cos);
    sincos (unit.orientation + PI_X_1_4, &a2_sin, &a2_cos);
    sincos (unit.orientation - PI_X_1_4, &a3_sin, &a3_cos);
    sincos (unit.orientation - PI_X_3_4, &a4_sin, &a4_cos);
    qreal map_to_screen_factor = coord_map.arena_viewport.height () / coord_map.POINTS_PER_VIEWPORT_VERTICALLY;
    qreal scale = coord_map.viewport_scale * sprite_scale * MatchState::unitDiameter (unit.type) * SQRT_2 * map_to_screen_factor;

    const GLfloat vertices[] = {
        GLfloat (center.x () + scale * a1_cos),
        GLfloat (center.y () + scale * a1_sin),
        GLfloat (center.x () + scale * a2_cos),
        GLfloat (center.y () + scale * a2_sin),
        GLfloat (center.x () + scale * a3_cos),
        GLfloat (center.y () + scale * a3_sin),
        GLfloat (center.x () + scale * a4_cos),
        GLfloat (center.y () + scale * a4_sin),
    };

    static const GLfloat texture_coords[] = {
        0,
        1,
        1,
        1,
        1,
        0,
        0,
        0,
    };

    static const GLuint indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    textured_renderer.draw (gl, GL_TRIANGLES, vertices, texture_coords, 6, indices, texture, ortho_matrix);
    if (unit.type == Unit::Type::Contaminator && unit.cast_cooldown_left_ticks) {
        qreal max_cooldown_ticks = qMax (MatchState::effectAttackDescription (AttackDescription::Type::PestilenceMissile).cooldown_ticks,
                                         MatchState::effectAttackDescription (AttackDescription::Type::SpawnBeetle).cooldown_ticks);
        qreal remaining = qreal (unit.cast_cooldown_left_ticks) / max_cooldown_ticks;

        qreal scale = coord_map.viewport_scale * sprite_scale * MatchState::unitDiameter (unit.type) * coord_map.arena_viewport.height () / coord_map.POINTS_PER_VIEWPORT_VERTICALLY;

        QVector<GLfloat> vertices;
        QVector<GLfloat> texture_coords;

        QPointF top_left = {center.x () - scale, center.y () - scale};
        QPointF top_right = {center.x () + scale, center.y () - scale};
        QPointF bottom_left = {center.x () - scale, center.y () + scale};
        QPointF bottom_right = {center.x () + scale, center.y () + scale};

        qreal angle = M_PI * remaining;

        do {
            if (angle < M_PI * 0.25) {
                qreal off_x = -scale * qTan (angle);
                qreal off_y = -scale;
                qreal toff_x = -0.5 * qTan (angle);
                qreal toff_y = -0.5;
                vertices.append ({GLfloat (center.x ()), GLfloat (center.y ()),
                                  GLfloat (center.x () + off_x), GLfloat (center.y () + off_y),
                                  GLfloat (center.x () - off_x), GLfloat (center.y () + off_y)});
                texture_coords.append ({GLfloat (0.5), GLfloat (0.5),
                                        GLfloat (0.5 + toff_x), GLfloat (0.5 + toff_y),
                                        GLfloat (0.5 - toff_x), GLfloat (0.5 + toff_y)});
                break;
            } else {
                vertices.append ({GLfloat (center.x ()), GLfloat (center.y ()),
                                  GLfloat (top_right.x ()), GLfloat (top_right.y ()),
                                  GLfloat (top_left.x ()), GLfloat (top_left.y ())});
                texture_coords.append ({0.5, 0.5,
                                        1, 0,
                                        0, 0});
            }

            if (angle < M_PI * 0.75) {
                qreal off_x = -scale;
                qreal off_y = -scale / qTan (angle);
                qreal toff_x = -0.5;
                qreal toff_y = -0.5 / qTan (angle);
                vertices.append ({GLfloat (top_left.x ()), GLfloat (top_left.y ()),
                                  GLfloat (center.x ()), GLfloat (center.y ()),
                                  GLfloat (center.x () + off_x), GLfloat (center.y () + off_y),
                                  GLfloat (top_right.x ()), GLfloat (top_right.y ()),
                                  GLfloat (center.x ()), GLfloat (center.y ()),
                                  GLfloat (center.x () - off_x), GLfloat (center.y () + off_y)});
                texture_coords.append ({GLfloat (0), GLfloat (0),
                                        GLfloat (0.5), GLfloat (0.5),
                                        GLfloat (0.5 + toff_x), GLfloat (0.5 + toff_y),
                                        GLfloat (1), GLfloat (0),
                                        GLfloat (0.5), GLfloat (0.5),
                                        GLfloat (0.5 - toff_x), GLfloat (0.5 + toff_y)});
                break;
            } else {
                vertices.append ({GLfloat (top_left.x ()), GLfloat (top_left.y ()),
                                  GLfloat (center.x ()), GLfloat (center.y ()),
                                  GLfloat (bottom_left.x ()), GLfloat (bottom_left.y ()),
                                  GLfloat (top_right.x ()), GLfloat (top_right.y ()),
                                  GLfloat (center.x ()), GLfloat (center.y ()),
                                  GLfloat (bottom_right.x ()), GLfloat (bottom_right.y ())});
                texture_coords.append ({0, 0,
                                        0.5, 0.5,
                                        0, 1,
                                        1, 0,
                                        0.5, 0.5,
                                        1, 1});
            }

            {
                qreal off_x = scale * qTan (angle);
                qreal off_y = scale;
                qreal toff_x = 0.5 * qTan (angle);
                qreal toff_y = 0.5;
                vertices.append ({GLfloat (center.x ()), GLfloat (center.y ()),
                                  GLfloat (bottom_left.x ()), GLfloat (bottom_left.y ()),
                                  GLfloat (center.x () + off_x), GLfloat (center.y () + off_y),
                                  GLfloat (center.x ()), GLfloat (center.y ()),
                                  GLfloat (bottom_right.x ()), GLfloat (bottom_right.y ()),
                                  GLfloat (center.x () - off_x), GLfloat (center.y () + off_y)});
                texture_coords.append ({GLfloat (0.5), GLfloat (0.5),
                                        GLfloat (0), GLfloat (1),
                                        GLfloat (0.5 + toff_x), GLfloat (0.5 + toff_y),
                                        GLfloat (0.5), GLfloat (0.5),
                                        GLfloat (1), GLfloat (1),
                                        GLfloat (0.5 - toff_x), GLfloat (0.5 + toff_y)});
            }
        } while (0);

        textured_renderer.draw (gl, GL_TRIANGLES, vertices.size () / 2, vertices.data (), texture_coords.data (), contaminator_cooldown_shade.get (), ortho_matrix);
    }

    if (unit.selected)
        colored_renderer.drawCircle (gl, center.x (), center.y (), scale * 0.5, {0, 255, 0}, ortho_matrix);
}
QImage UnitRenderer::loadUnitFromSVGTemplate (const QString& path, const QByteArray& animation_name, const QColor& player_color)
{
    static constexpr QSize texture_size (256, 256);
    QImage image (texture_size, QImage::Format_ARGB32);
    image.fill (QColor (0, 0, 0, 0));
    QFile svg_fh (path);
    if (!svg_fh.open (QIODevice::ReadOnly))
        return image;
    QByteArray svg_data = svg_fh.readAll ();
    svg_data.replace ("&animation;", animation_name);
    svg_data.replace ("&player_color;", player_color.name ().toUtf8 ());
    QSvgRenderer svg_renderer (svg_data);
    {
        QPainter p (&image);
        p.translate (texture_size.width ()*0.5, texture_size.height ()*0.5);
        p.rotate (90.0);
        p.translate (-texture_size.width ()*0.5, -texture_size.height ()*0.5);
        svg_renderer.render (&p, QRectF (QPointF (0, 0), QSizeF (texture_size)));
    }
    return image;
}
QSharedPointer<QOpenGLTexture> UnitRenderer::loadTexture2D (const QString& path)
{
    QOpenGLTexture* texture = new QOpenGLTexture (QImage (path));
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
QSharedPointer<QOpenGLTexture> UnitRenderer::loadTexture2D (const QImage& image)
{
    QOpenGLTexture* texture = new QOpenGLTexture (image);
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
quint64 UnitRenderer::moveAnimationPeriodNS (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return 500'000'000;
    case Unit::Type::Crusader:
        return 300'000'000;
    case Unit::Type::Goon:
        return 500'000'000;
    case Unit::Type::Beetle:
        return 300'000'000;
    case Unit::Type::Contaminator:
        return 800'000'000;
    default:
        return 0;
    }
}
quint64 UnitRenderer::attackAnimationPeriodNS (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return 100'000'000;
    case Unit::Type::Crusader:
        return 700'000'000;
    case Unit::Type::Goon:
        return 200'000'000;
    case Unit::Type::Beetle:
        return 700'000'000;
    case Unit::Type::Contaminator:
        return 700'000'000;
    default:
        return 0;
    }
}
