#pragma once

#include "matchstate.h"

#include <QSharedPointer>

class QOpenGLFunctions;
class ColoredRenderer;
class HUD;
class QMatrix4x4;
class CoordMap;
class MatchState;


class MinimapRenderer
{
public:
    MinimapRenderer ();
    void draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
               HUD& hud, MatchState& match_state, Unit::Team team,
               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
};
