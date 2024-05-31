#pragma once

#include "position.h"


class PositionAverage {
public:
    PositionAverage ()
        : x (0.0), y (0.0), count (0)
    {
    }

    void add (const Position& position)
    {
        x += position.x ();
        y += position.y ();
        ++count;
    }
    std::optional<Position> get ()
    {
        if (count)
            return Position (x/count, y/count);
        else
            return std::nullopt;
    }

private:
    double x;
    double y;
    size_t count;
};
