#include "matchstate.h"

#include <QtMath>
#include <QDebug>


Unit::Unit (Type type, Team team, const QPointF& position, qreal orientation)
    : type (type), team (team), position (position), orientation (orientation)
{
}
Unit::~Unit ()
{
}


MatchState::MatchState ()
{
}
MatchState::~MatchState ()
{
}

const QRectF& MatchState::areaRef () const
{
    return area;
}
const QHash<uint32_t, Unit>& MatchState::unitsRef () const
{
    return units;
}
void MatchState::addUnit (const Unit& unit)
{
    units.insert (next_id++, unit);
}
void MatchState::trySelect (const QPointF& point, bool add)
{
    bool already_found = false;
    for (QHash<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (!add && already_found) {
            unit.selected = false;
            continue;
        }

        bool select = false;
        switch (unit.type) {
        case Unit::Type::Seal: {
            QPointF delta = point - unit.position;
            qreal radius = 10.0;
            if ((delta.x ()*delta.x () + delta.y ()*delta.y ()) <= radius*radius)
                select = true;
        } break;
        case Unit::Type::Crusader: {
            QPointF delta = point - unit.position;
            qreal radius = 15.0;
            if ((delta.x ()*delta.x () + delta.y ()*delta.y ()) <= radius*radius)
                select = true;
        } break;
        }
        if (select) {
            if (add) {
                unit.selected = true;
            } else {
                unit.selected = true;
                already_found = true;
            }
        } else {
            if (!add)
                unit.selected = false;
        }
    }
}
void MatchState::trySelect (const QRectF& point, bool add)
{
}
void MatchState::move (const QPointF& point)
{
    for (QHash<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected) {
            unit.move_target = point;
        }
    }
}
void MatchState::stop ()
{
    for (QHash<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected) {
            unit.move_target.reset ();
        }
    }
}
void MatchState::autoAction (const QPointF& point)
{
    // TODO: Attack if enemy
    for (QHash<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected) {
            unit.move_target = point;
        }
    }
}
void MatchState::tick ()
{
    qreal dt = 0.020;
    for (QHash<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.move_target.has_value ()) {
            qreal velocity = 0.0;
            switch (unit.type) {
            case Unit::Type::Seal: {
                velocity = 150.0;
            } break;
            case Unit::Type::Crusader: {
                velocity = 250.0;
            } break;
            }
            QPointF off = unit.move_target.value () - unit.position;
            unit.orientation = qAtan2 (off.y (), off.x ());
            qreal path_length = velocity*dt;
            qreal square_length = off.x ()*off.x () + off.y ()*off.y ();
            if (square_length <= path_length*path_length) {
                unit.position = unit.move_target.value ();
                unit.move_target.reset ();
            } else {
                unit.position += off*(path_length/qSqrt (square_length));
            }
        }
    }
}
