#include "coordmap.h"


Position CoordMap::toMapCoords (const Position& point) const
{
    double scale = POINTS_PER_VIEWPORT_VERTICALLY / (viewport_scale * arena_viewport.height ());
    return viewport_center + (point - arena_viewport_center)*scale;
}
Position CoordMap::toMapCoords (const QPoint& point) const
{
    double scale = POINTS_PER_VIEWPORT_VERTICALLY / (viewport_scale * arena_viewport.height ());
    return viewport_center + (Position (point.x (), point.y ()) - arena_viewport_center)*scale;
}
Rectangle CoordMap::toMapCoords (const Rectangle& rect) const
{
    return {toMapCoords (rect.topLeft ()), toMapCoords (rect.bottomRight ())};
}
Rectangle CoordMap::toMapCoords (const QRect& rect) const
{
    return {toMapCoords (rect.topLeft ()), toMapCoords (rect.bottomRight ())};
}
Position CoordMap::toScreenCoords (const Position& point) const
{
    double scale = viewport_scale * arena_viewport.height () / POINTS_PER_VIEWPORT_VERTICALLY;
    return arena_viewport_center + (point - viewport_center)*scale;
}
Position CoordMap::toScreenCoords (const QPoint& point) const
{
    double scale = viewport_scale * arena_viewport.height () / POINTS_PER_VIEWPORT_VERTICALLY;
    return arena_viewport_center + (Position (point.x (), point.y ()) - viewport_center)*scale;
}
