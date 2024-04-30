#pragma once

#include "matchstate.h"

#include <QSharedPointer>
#include <QFont>

class QOpenGLFunctions;
class ColoredRenderer;
class ColoredTexturedRenderer;
class TexturedRenderer;
class HUD;
class QMatrix4x4;
class CoordMap;
class MatchState;
class QPaintDevice;
class UnitIconSet;
class ActionPanelRenderer;
class MinimapRenderer;
class GroupOverlayRenderer;
class SelectionPanelRenderer;


class HUDRenderer
{
public:
    HUDRenderer ();
    void draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, ColoredTexturedRenderer& colored_textured_renderer, TexturedRenderer& textured_renderer, QPaintDevice& device,
               HUD& hud, MatchState& match_state, Unit::Team team,
               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    QSharedPointer<UnitIconSet> unit_icon_set;
    QSharedPointer<ActionPanelRenderer> action_panel_renderer;
    QSharedPointer<MinimapRenderer> minimap_renderer;
    QSharedPointer<GroupOverlayRenderer> group_overlay_renderer;
    QSharedPointer<SelectionPanelRenderer> selection_panel_renderer;
};
