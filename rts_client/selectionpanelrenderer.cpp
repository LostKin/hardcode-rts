#include "selectionpanelrenderer.h"

#include "coordmap.h"
#include "matchstate.h"
#include "coloredrenderer.h"
#include "coloredtexturedrenderer.h"
#include "hud.h"
#include "uniticonset.h"

#include <QColor>
#include <QPainter>
#include <QOpenGLTexture>


SelectionPanelRenderer::SelectionPanelRenderer (const QSharedPointer<UnitIconSet>& unit_icon_set)
    : unit_icon_set (unit_icon_set)
    , font ("NotCourier", 16)
{
    font.setStyleHint (QFont::TypeWriter);
    frame = loadTexture2D (":/images/icons/frame.png"); // TODO: Refine drawing
    tabs = loadTexture2D (":/images/icons/tabs.png"); // TODO: Refine drawing
}

void SelectionPanelRenderer::draw (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer, QPaintDevice& device,
                                   const QRect& rect, size_t selected_count, const Unit* last_selected_unit,
                                   HUD& hud, MatchState& match_state,
                                   const QMatrix4x4& ortho_matrix)
{
    if (selected_count == 1) {
        const Unit& unit = *last_selected_unit;
        const AttackDescription& primary_attack_description = match_state.unitPrimaryAttackDescription (unit.type);

        {
            int margin = rect.height () * 0.2;
            int icon_rib = rect.height () - margin * 2;
            drawIcon (gl, colored_textured_renderer, unit, rect.x () + margin, rect.y () + margin, icon_rib, icon_rib, false, ortho_matrix);
        }
        {
            int margin = rect.height () * 0.05;
            QPainter p (&device);
            p.setFont (font);
            p.setPen (QColor (0xdf, 0xdf, 0xff));
            p.drawText (rect.marginsRemoved ({margin, margin, margin, margin}), Qt::AlignHCenter,
                        unitTitle (unit.type) + "\n"
                        "\n"
                        "HP: " +
                        QString::number (unit.hp) + "/" + QString::number (match_state.unitMaxHP (unit.type)) + "\n"
                        "Attack: class = " + unitAttackClassName (primary_attack_description.type) + "; range = " + QString::number (primary_attack_description.range));
        }
    } else if (selected_count) {
        int icon_rib = hud.selection_panel_icon_rib;

        QVector<QPair<quint32, const Unit*>> selection = match_state.buildOrderedSelection ();

        if (selection.size () > 30)
            drawTabs (gl, colored_textured_renderer, hud.selection_panel_icon_grid_pos.x () - icon_rib, hud.selection_panel_icon_grid_pos.y (), icon_rib, icon_rib, ortho_matrix);

        for (size_t i = 0; i < size_t (qMin (selection.size (), 30)); ++i) {
            const Unit* unit = selection[i].second;
            size_t row = i / 10;
            size_t col = i % 10;
            drawIcon (gl, colored_textured_renderer, *unit,
                      hud.selection_panel_icon_grid_pos.x () + icon_rib * col, hud.selection_panel_icon_grid_pos.y () + icon_rib * row, icon_rib, icon_rib, true,
                      ortho_matrix);
        }
    }
}
void SelectionPanelRenderer::drawTabs (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer,
                                       int x, int y, int w, int h, const QMatrix4x4& ortho_matrix)
{
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

    static const GLuint indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    static const GLfloat texture_coords[] = {
        0.0,
        0.0,
        1.0,
        0.0,
        1.0,
        1.0,
        0.0,
        1.0,
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
    colored_textured_renderer.draw (gl, GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, &*tabs, ortho_matrix);
}
const QString& SelectionPanelRenderer::unitTitle (Unit::Type type)
{
    static const QString seal = "Seal";
    static const QString crusader = "Crusader";
    static const QString goon = "Goon";
    static const QString beetle = "Beetle";
    static const QString contaminator = "Contaminator";
    static const QString unknown = "Unknown";

    switch (type) {
    case Unit::Type::Seal:
        return seal;
    case Unit::Type::Crusader:
        return crusader;
    case Unit::Type::Goon:
        return goon;
    case Unit::Type::Beetle:
        return beetle;
    case Unit::Type::Contaminator:
        return contaminator;
    default:
        return unknown;
    }
}
const QString& SelectionPanelRenderer::unitAttackClassName (AttackDescription::Type type)
{
    static const QString immediate = "immediate";
    static const QString missile = "missile";
    static const QString splash = "splash";
    static const QString unknown = "unknown";

    switch (type) {
    case AttackDescription::Type::SealShot:
    case AttackDescription::Type::CrusaderChop:
    case AttackDescription::Type::BeetleSlice:
        return immediate;
    case AttackDescription::Type::GoonRocket:
    case AttackDescription::Type::PestilenceMissile:
        return missile;
    case AttackDescription::Type::GoonRocketExplosion:
    case AttackDescription::Type::PestilenceSplash:
        return splash;
    default:
        return unknown;
    }
}
QSharedPointer<QOpenGLTexture> SelectionPanelRenderer::loadTexture2D (const QString& path)
{
    QOpenGLTexture* texture = new QOpenGLTexture (QImage (path));
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
void SelectionPanelRenderer::drawIcon (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer,
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
void SelectionPanelRenderer::drawIcon (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer,
                                       const Unit& unit, qreal x, qreal y, qreal w, qreal h, bool framed,
                                       const QMatrix4x4& ortho_matrix)
{
    qreal hp_ratio = qreal (unit.hp) / MatchState::unitMaxHP (unit.type);
    drawIcon (gl, colored_textured_renderer, unit.type, hp_ratio, x, y, w, h, framed, ortho_matrix);
}
QColor SelectionPanelRenderer::getHPColor (qreal hp_ratio)
{
    return QColor::fromRgbF (1.0 - hp_ratio, hp_ratio, 0.0);
}
