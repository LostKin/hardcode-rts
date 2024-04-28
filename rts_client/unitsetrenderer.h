#pragma once

#include "matchstate.h"

#include <QSharedPointer>

class UnitTeamRenderer;
class QOpenGLTexture;
class QString;
class QOpenGLFunctions;
class ColoredRenderer;
class TexturedRenderer;
class CoordMap;


class UnitSetRenderer
{
public:
    UnitSetRenderer (const QColor& red_team_color, const QColor& red_blue_color);
    void draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, TexturedRenderer& textured_renderer, const Unit& unit, quint64 clock_ns,
               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    QSharedPointer<UnitTeamRenderer> red;
    QSharedPointer<UnitTeamRenderer> blue;
};
