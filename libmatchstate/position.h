#pragma once

#include "offset.h"


class Position {
public:
    Position (double x, double y)
        : x_ (x), y_ (y)
    {
    }
    Position ()
        : x_ (0.0), y_ (0.0)
    {
    }

    void setX (double x)
    {
        x_ = x;
    }
    void setY (double y)
    {
        y_ = y;
    }
    double x () const
    {
        return x_;
    }
    double y () const
    {
        return y_;
    }
    Offset operator- (const Position& b) const
    {
        return {x_ - b.x (), y_ - b.y ()};
    }
    Position operator+ (const Offset& offset) const
    {
        return {x_ + offset.dX (), y_ + offset.dY ()};
    }
    Position operator- (const Offset& offset) const
    {
        return {x_ - offset.dX (), y_ - offset.dY ()};
    }
    void operator+= (const Offset& offset)
    {
        x_ += offset.dX ();
        y_ += offset.dY ();
    }
    void operator-= (const Offset& offset)
    {
        x_ -= offset.dX ();
        y_ -= offset.dY ();
    }
    bool operator< (const Position& b) const
    {
        return y_ < b.y_ || (y_ == b.y_ && x_ < b.x_);
    }

private:
    double x_;
    double y_;
};
