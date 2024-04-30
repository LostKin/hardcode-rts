#pragma once

#include "matchstate.h"

#include <QSharedPointer>
#include <QFont>

class QOpenGLFunctions;
class QOpenGLTexture;
class ColoredRenderer;
class ColoredTexturedRenderer;
class HUD;
class QMatrix4x4;
class CoordMap;
class MatchState;
class QPaintDevice;
class UnitIconSet;


class SelectionPanelRenderer
{
public:
    SelectionPanelRenderer (const QSharedPointer<UnitIconSet>& unit_icon_set);
    void draw (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer, QPaintDevice& device,
               const QRect& rect, size_t selected_count, const Unit* last_selected_unit,
               HUD& hud, MatchState& match_state,
               const QMatrix4x4& ortho_matrix);

private:
    static const QString& unitTitle (Unit::Type type);
    static const QString& unitAttackClassName (AttackDescription::Type type);

    QSharedPointer<QOpenGLTexture> loadTexture2D (const QString& path);
    void drawTabs (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer,
                   int x, int y, int w, int h, const QMatrix4x4& ortho_matrix);
    void drawIcon (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer,
                   const Unit::Type& unit_type, qreal hp_ratio, qreal x, qreal y, qreal w, qreal h, bool framed,
                   const QMatrix4x4& ortho_matrix);
    void drawIcon (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer,
                   const Unit& unit, qreal x, qreal y, qreal w, qreal h, bool framed,
                   const QMatrix4x4& ortho_matrix);
    QColor getHPColor (qreal hp_ratio);

private:
    QSharedPointer<UnitIconSet> unit_icon_set;
    QSharedPointer<QOpenGLTexture> frame;
    QSharedPointer<QOpenGLTexture> tabs;
    QFont font;
};
