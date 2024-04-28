#pragma once

#include "matchstate.h"

#include <QSharedPointer>

class UnitRenderer;
class QOpenGLTexture;
class QString;
class QOpenGLFunctions;
class ColoredRenderer;
class TexturedRenderer;
class CoordMap;


class UnitTeamRenderer
{
public:
    UnitTeamRenderer (const QColor& team_color);
    void draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, TexturedRenderer& textured_renderer, const Unit& unit, quint64 clock_ns,
               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    QSharedPointer<UnitRenderer> seal;
    QSharedPointer<UnitRenderer> crusader;
    QSharedPointer<UnitRenderer> goon;
    QSharedPointer<UnitRenderer> beetle;
    QSharedPointer<UnitRenderer> contaminator;
};
