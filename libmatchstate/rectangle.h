#pragma once

#include "position.h"


class Rectangle
{
public:
    Rectangle ()
        : left_ (0.0), right_ (0.0), top_ (0.0), bottom_ (0.0)
    {
    }
    Rectangle (double left, double right, double top, double bottom)
        : left_ (left), right_ (right), top_ (top), bottom_ (bottom)
    {
    }
    Rectangle (const Position& top_left, const Position& bottom_right)
        : left_ (top_left.x ()), right_ (bottom_right.x ()), top_ (top_left.y ()), bottom_ (bottom_right.y ())
    {
    }

    double left () const
    {
        return left_;
    }
    double right () const
    {
        return right_;
    }
    double top () const
    {
        return top_;
    }
    double bottom () const
    {
        return bottom_;
    }
    Position topLeft () const
    {
        return {left_, top_};
    }
    Position bottomLeft () const
    {
        return {left_, bottom_};
    }
    Position topRight () const
    {
        return {right_, top_};
    }
    Position bottomRight () const
    {
        return {right_, bottom_};
    }
    double width () const
    {
        return right_ - left_;
    }
    double height () const
    {
        return bottom_ - top_;
    }

private:
    double left_;
    double right_;
    double top_;
    double bottom_;
};
