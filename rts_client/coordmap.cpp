#include "coordmap.h"


CoordMap::CoordMap (const QRect& arena_viewport,
                    const QPointF& arena_viewport_center,
                    const qreal& viewport_scale,
                    const QPointF& viewport_center)
    : arena_viewport (arena_viewport)
    , arena_viewport_center (arena_viewport_center)
    , viewport_scale (viewport_scale)
    , viewport_center (viewport_center)
{
}

QPointF CoordMap::toMapCoords (const QPointF& point) const
{
    qreal scale = viewport_scale * arena_viewport.height () / POINTS_PER_VIEWPORT_VERTICALLY;
    return viewport_center + (point - arena_viewport_center) / scale;
}
QRectF CoordMap::toMapCoords (const QRectF& rect) const
{
    return QRectF (toMapCoords (rect.topLeft ()), toMapCoords (rect.bottomRight ()));
}
QPointF CoordMap::toScreenCoords (const QPointF& point) const
{
    qreal scale = viewport_scale * arena_viewport.height () / POINTS_PER_VIEWPORT_VERTICALLY;
    return arena_viewport_center + (point - viewport_center)*scale;
}
