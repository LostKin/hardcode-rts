#pragma once

#include "matchstate.h"

#include <QSharedPointer>
#include <QFont>
#include <QColor>

class QOpenGLFunctions;
class QOpenGLTexture;
class ColoredRenderer;
class ColoredTexturedRenderer;
class TexturedRenderer;
class HUD;
class QMatrix4x4;
class CoordMap;
class MatchState;
class QPaintDevice;
class UnitSetRenderer;
class EffectRenderer;


class SceneRenderer
{
public:
    SceneRenderer ();
    void draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, ColoredTexturedRenderer& colored_textured_renderer, TexturedRenderer& textured_renderer,
               MatchState& match_state, Unit::Team team, const QPoint& cursor_position, const std::optional<QPoint>& selection_start,
               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    void drawBackground (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer,
                         MatchState& match_state,
                         const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawCorpses (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer,
                      MatchState& match_state,
                      const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawUnits (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer,
                    MatchState& match_state,
                    const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawUnitSelection (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                            MatchState& match_state,
                            const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawEffects (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer, TexturedRenderer& textured_renderer,
                      MatchState& match_state,
                      const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawUnitPaths (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                        MatchState& match_state, Unit::Team team,
                        const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawUnitStats (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                        MatchState& match_state,
                        const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawSelectionBar (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                           const QPoint& cursor_position, const std::optional<QPoint>& selection_start,
                           const QMatrix4x4& ortho_matrix);
    const QPointF* getUnitTargetPosition (const Unit& unit, MatchState& match_state);
    const QPointF* getUnitTargetPosition (const UnitActionVariant& unit_action, MatchState& match_state);
    const QPointF* getUnitTargetPosition (const IntentiveActionVariant& unit_action, MatchState& match_state);
    QSharedPointer<QOpenGLTexture> loadTexture2D (const QString& path);

private:
    QColor red_player_color = QColor (0xf1, 0x4b, 0x2c);
    QColor blue_player_color = QColor (0x48, 0xc0, 0xbb);
    QSharedPointer<UnitSetRenderer> unit_set_renderer;
    QSharedPointer<EffectRenderer> effect_renderer;
    struct {
        QSharedPointer<QOpenGLTexture> ground;
    } textures;
};
