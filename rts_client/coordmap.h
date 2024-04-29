#pragma once

#include <QPointF>
#include <QRectF>
#include <QPointF>


class CoordMap
{
public:
    CoordMap () = default;

    QPointF toMapCoords (const QPointF& point) const;
    QRectF toMapCoords (const QRectF& rect) const;
    QPointF toScreenCoords (const QPointF& point) const;

public:
    static constexpr qreal POINTS_PER_VIEWPORT_VERTICALLY = 20.0; // At zoom x1.0

    QSize viewport_size = {1, 1};
    QRect arena_viewport = {0, 0, 1, 1};
    QPointF arena_viewport_center = {0, 0};
    int viewport_scale_power = 0;
    qreal viewport_scale = 1.0;
    QPointF viewport_center = {0.0, 0.0};
};
