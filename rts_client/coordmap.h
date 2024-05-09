#pragma once

#include "position.h"
#include "rectangle.h"

#include <QRect>
#include <QSize>


class CoordMap
{
public:
    CoordMap () = default;

    Position toMapCoords (const Position& point) const;
    Position toMapCoords (const QPoint& point) const;
    Rectangle toMapCoords (const Rectangle& rect) const;
    Rectangle toMapCoords (const QRect& rect) const;
    Position toScreenCoords (const Position& point) const;
    Position toScreenCoords (const QPoint& point) const;

public:
    static constexpr qreal POINTS_PER_VIEWPORT_VERTICALLY = 20.0; // At zoom x1.0

    QSize viewport_size = {1, 1};
    QRect arena_viewport = {0, 0, 1, 1};
    Position arena_viewport_center = {0, 0};
    int viewport_scale_power = 0;
    qreal viewport_scale = 1.0;
    Position viewport_center = {0.0, 0.0};
};
