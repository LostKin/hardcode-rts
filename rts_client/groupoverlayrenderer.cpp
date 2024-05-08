#include "groupoverlayrenderer.h"

#include "coordmap.h"
#include "matchstate.h"
#include "coloredrenderer.h"
#include "coloredtexturedrenderer.h"
#include "hud.h"
#include "uniticonset.h"

#include <QColor>
#include <QPainter>
#include <QOpenGLTexture>


GroupOverlayRenderer::GroupOverlayRenderer (const QSharedPointer<UnitIconSet>& unit_icon_set)
    : unit_icon_set (unit_icon_set)
    , group_number_font ("NotCourier", 10)
    , group_size_font ("NotCourier", 8)
{
    frame = loadTexture2D (":/images/icons/frame.png"); // TODO: Refine drawing
}

void GroupOverlayRenderer::draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, ColoredTexturedRenderer& colored_textured_renderer, QPaintDevice& device,
                                 HUD& hud, MatchState& match_state,
                                 const QMatrix4x4& ortho_matrix)
{
    qint64 group_sizes[GROUP_COUNT] = {};
    Unit::Type group_unit_counts[GROUP_COUNT];
    for (qint64 i = 0; i < GROUP_COUNT; ++i)
        group_unit_counts[i] = Unit::Type::Beetle;
    const std::map<quint32, Unit>& units = match_state.unitsRef ();
    for (std::map<quint32, Unit>::const_iterator it = units.cbegin (); it != units.cend (); ++it) {
        const Unit& unit = it->second;
        quint64 groups = unit.groups;
        for (qint64 i = 0; i < GROUP_COUNT; ++i) {
            if (groups & (1 << (i + 1))) {
                group_sizes[i]++;
                group_unit_counts[i] = qMax (group_unit_counts[i], unit.type);
            }
        }
    }
    static constexpr QColor group_info_rect_color (0, 0x44, 0);
    static constexpr QColor group_number_rect_color (0x44, 0, 0);
    static constexpr QColor group_border_color (0, 0xff, 0);
    static constexpr QColor group_shade_color (0, 0, 0, 0x22);
    static constexpr QColor group_text_color (0xdf, 0xdf, 0xff);
    int icon_rib = hud.selection_panel_icon_rib;
    QSizeF info_rect_size (icon_rib*0.8, icon_rib*0.4);
    QSizeF number_rect_size (icon_rib*0.6, icon_rib*0.3);
    int group_icon_rib = hud.margin*5/3;
    QSize group_icon_size (group_icon_rib, group_icon_rib);
    {
        for (qint64 col = 0; col < GROUP_COUNT; ++col) {
            qreal number_rect_y = hud.selection_panel_rect.y () - hud.margin*0.5;
            qreal x_center = hud.selection_panel_icon_grid_pos.x () + icon_rib*(col - 5 + 0.5);
            qreal number_rect_y_center = number_rect_y - number_rect_size.height ()*0.5;
            QRectF info_rect = {x_center - info_rect_size.width ()*0.5, number_rect_y_center - info_rect_size.height () + 1, info_rect_size.width (), info_rect_size.height ()};
            QRectF number_rect = {x_center - number_rect_size.width ()*0.5, number_rect_y_center, number_rect_size.width (), number_rect_size.height ()};
            // TODO: Try avoid using colored_renderer to minimize program switches
            if (group_sizes[col]) {
                colored_renderer.fillRectangle (gl, info_rect, group_info_rect_color, ortho_matrix);
                colored_renderer.fillRectangle (gl, number_rect, group_number_rect_color, ortho_matrix);
                colored_renderer.drawRectangle (gl, info_rect, group_border_color, ortho_matrix);
                colored_renderer.drawRectangle (gl, number_rect, group_border_color, ortho_matrix);
                drawIcon (gl, colored_textured_renderer, group_unit_counts[col], 1.0, info_rect.x (), info_rect.y (), group_icon_size.width (), group_icon_size.height (), false, ortho_matrix);
            } else {
                colored_renderer.fillRectangle (gl, info_rect, group_shade_color, ortho_matrix);
            }
        }
    }
    {
        int icon_rib = hud.selection_panel_icon_rib;

        QPainter p (&device);
        p.setPen (group_text_color);
        for (qint64 col = 0; col < GROUP_COUNT; ++col) {
            if (group_sizes[col]) {
                qreal number_rect_y = hud.selection_panel_rect.y () - hud.margin*0.5;
                qreal info_rect_w = icon_rib*0.8;
                qreal info_rect_h = info_rect_w*0.5;
                qreal number_rect_w = icon_rib*0.6;
                qreal number_rect_h = number_rect_w*0.5;
                qreal x_center = hud.selection_panel_icon_grid_pos.x () + icon_rib * (col - 5 + 0.5);
                qreal number_rect_y_center = number_rect_y - number_rect_h*0.5;
                QRectF info_rect = QRectF (x_center - info_rect_w*0.5, number_rect_y_center - info_rect_h + 1, info_rect_w, info_rect_h).marginsRemoved ({3, 3, 3, 3});
                QRectF number_rect = {x_center - number_rect_w*0.5, number_rect_y_center, number_rect_w, number_rect_h};
                p.setFont (group_size_font);
                p.drawText (info_rect, Qt::AlignRight | Qt::AlignVCenter, QString::number (group_sizes[col]));
                p.setFont (group_number_font);
                p.drawText (number_rect, Qt::AlignHCenter | Qt::AlignVCenter, QString::number (col + 1));
            }
        }
    }
}
QColor GroupOverlayRenderer::getHPColor (qreal hp_ratio)
{
    return QColor::fromRgbF (1.0 - hp_ratio, hp_ratio, 0.0);
}
QSharedPointer<QOpenGLTexture> GroupOverlayRenderer::loadTexture2D (const QString& path)
{
    QOpenGLTexture* texture = new QOpenGLTexture (QImage (path));
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
void GroupOverlayRenderer::drawIcon (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer,
                                     const Unit::Type& unit_type, qreal hp_ratio, qreal x, qreal y, qreal w, qreal h, bool framed,
                                     const QMatrix4x4& ortho_matrix)
{
    qreal sprite_scale;
    QOpenGLTexture* texture;
    switch (unit_type) {
    case Unit::Type::Seal:
        sprite_scale = 0.8;
        texture = unit_icon_set->seal.get ();
        break;
    case Unit::Type::Crusader:
        sprite_scale = 1.1;
        texture = unit_icon_set->crusader.get ();
        break;
    case Unit::Type::Goon:
        sprite_scale = 0.9;
        texture = unit_icon_set->goon.get ();
        break;
    case Unit::Type::Beetle:
        sprite_scale = 0.6;
        texture = unit_icon_set->beetle.get ();
        break;
    case Unit::Type::Contaminator:
        sprite_scale = 1.2;
        texture = unit_icon_set->contaminator.get ();
        break;
    default:
        return;
    }

    const GLfloat vertices[] = {
        GLfloat (x + w*(0.5 - 0.5*sprite_scale)),
        GLfloat (y + h*(0.5 + 0.5*sprite_scale)),
        GLfloat (x + w*(0.5 - 0.5*sprite_scale)),
        GLfloat (y + h*(0.5 - 0.5*sprite_scale)),
        GLfloat (x + w*(0.5 + 0.5*sprite_scale)),
        GLfloat (y + h*(0.5 - 0.5*sprite_scale)),
        GLfloat (x + w*(0.5 + 0.5*sprite_scale)),
        GLfloat (y + h*(0.5 + 0.5*sprite_scale)),
    };

    static const GLfloat texture_coords[] = {
        GLfloat (0),
        GLfloat (0),
        GLfloat (1),
        GLfloat (0),
        GLfloat (1),
        GLfloat (1),
        GLfloat (0),
        GLfloat (1),
    };

    QColor color = getHPColor (hp_ratio);

    const GLfloat colors[] = {
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
    };

    static const GLuint indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    if (framed) {
        const GLfloat vertices[] = {
            GLfloat (x),
            GLfloat (y + h),
            GLfloat (x),
            GLfloat (y),
            GLfloat (x + w),
            GLfloat (y),
            GLfloat (x + w),
            GLfloat (y + h),
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
        QColor color = QColor::fromRgbF (1.0, 1.0, 1.0, 1.0);
        const GLfloat colors[] = {
            GLfloat (color.redF ()),
            GLfloat (color.greenF ()),
            GLfloat (color.blueF ()),
            GLfloat (color.alphaF ()),
            GLfloat (color.redF ()),
            GLfloat (color.greenF ()),
            GLfloat (color.blueF ()),
            GLfloat (color.alphaF ()),
            GLfloat (color.redF ()),
            GLfloat (color.greenF ()),
            GLfloat (color.blueF ()),
            GLfloat (color.alphaF ()),
            GLfloat (color.redF ()),
            GLfloat (color.greenF ()),
            GLfloat (color.blueF ()),
            GLfloat (color.alphaF ()),
        };
        colored_textured_renderer.draw (gl, GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, frame.get (), ortho_matrix);
    }

    colored_textured_renderer.draw (gl, GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, texture, ortho_matrix);
}
void GroupOverlayRenderer::drawIcon (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer,
                                     const Unit& unit, qreal x, qreal y, qreal w, qreal h, bool framed,
                                     const QMatrix4x4& ortho_matrix)
{
    qreal hp_ratio = qreal (unit.hp) / MatchState::unitMaxHP (unit.type);
    drawIcon (gl, colored_textured_renderer, unit.type, hp_ratio, x, y, w, h, framed, ortho_matrix);
}
