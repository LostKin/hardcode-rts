#pragma once


class Scale
{
public:
    Scale (double x, double y)
        : x_ (x), y_ (y)
    {
    }
    Scale ()
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

private:
    double x_;
    double y_;
};
