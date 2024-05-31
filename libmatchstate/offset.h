#pragma once

#include "scale.h"

#include <cmath>


class Offset {
public:
    static double dotProduct (const Offset& a, const Offset& b)
    {
        return a.dX ()*b.dX () + a.dY ()*b.dY ();
    }

public:
    Offset (double dx, double dy)
        : dx (dx), dy (dy)
    {
    }
    Offset ()
        : dx (0.0), dy (0.0)
    {
    }

    void setDX (double dx)
    {
        this->dx = dx;
    }
    void setDY (double dy)
    {
        this->dy = dy;
    }
    double dX () const
    {
        return dx;
    }
    double dY () const
    {
        return dy;
    }
    double length () const
    {
        return std::hypot (dx, dy);
    }
    double orientation () const
    {
        return std::atan2 (dy, dx);
    }
    void setLength (double new_length)
    {
        double old_length = std::hypot (dx, dy);
        if (old_length > 0.000000001)
            *this *= new_length / old_length;
    }
    Offset operator* (double multiplier) const
    {
        return {dx * multiplier, dy * multiplier};
    }
    void operator*= (double multiplier)
    {
        dx *= multiplier;
        dy *= multiplier;
    }
    Offset operator* (const Scale& scale) const
    {
        return {dx * scale.x (), dy * scale.y ()};
    }
    void operator*= (const Scale& scale)
    {
        dx *= scale.x ();
        dy *= scale.y ();
    }
    Offset operator+ (const Offset& addition) const
    {
        return {dx + addition.dX (), dy + addition.dY ()};
    }
    void operator+= (const Offset& addition)
    {
        dx += addition.dX ();
        dy += addition.dY ();
    }
    Offset operator- (const Offset& addition) const
    {
        return {dx - addition.dX (), dy - addition.dY ()};
    }
    void operator-= (const Offset& addition)
    {
        dx -= addition.dX ();
        dy -= addition.dY ();
    }

private:
    double dx;
    double dy;
};
