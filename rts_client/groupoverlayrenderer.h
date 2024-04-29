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


class GroupOverlayRenderer
{
public:
    GroupOverlayRenderer (const QSharedPointer<UnitIconSet>& unit_icon_set);
    void draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, ColoredTexturedRenderer& colored_textured_renderer, QPaintDevice& device /* Can it be fixed? */,
               HUD& hud, MatchState& match_state, Unit::Team team,
               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    static QColor getHPColor (qreal hp_ratio);

    QSharedPointer<QOpenGLTexture> loadTexture2D (const QString& path);
    void drawIcon (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, ColoredTexturedRenderer& colored_textured_renderer,
                   const Unit::Type& unit_type, qreal hp_ratio, qreal x, qreal y, qreal w, qreal h, bool framed,
                   const QMatrix4x4& ortho_matrix);
    void drawIcon (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, ColoredTexturedRenderer& colored_textured_renderer,
                   const Unit& unit, qreal x, qreal y, qreal w, qreal h, bool framed,
                   const QMatrix4x4& ortho_matrix);

private:
    static constexpr qint64 GROUP_COUNT = 20;

    QSharedPointer<UnitIconSet> unit_icon_set;
    QSharedPointer<QOpenGLTexture> frame;
    QFont group_number_font;
    QFont group_size_font;
};
