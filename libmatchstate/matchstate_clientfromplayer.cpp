#include "matchstate.h"


static bool intersectRectangleCircle (const Rectangle& rect, const Position& center, double radius)
{
    if (center.x () < rect.left ()) {
        if (center.y () < rect.top ()) {
            Offset delta = center - rect.topLeft ();
            return Offset::dotProduct (delta, delta) <= radius * radius;
        } else if (center.y () > rect.bottom ()) {
            Offset delta = center - rect.bottomLeft ();
            return Offset::dotProduct (delta, delta) <= radius * radius;
        } else {
            return center.x () >= (rect.left () - radius);
        }
    } else if (center.x () > rect.right ()) {
        if (center.y () < rect.top ()) {
            Offset delta = center - rect.topRight ();
            return Offset::dotProduct (delta, delta) <= radius * radius;
        } else if (center.y () > rect.bottom ()) {
            Offset delta = center - rect.bottomRight ();
            return Offset::dotProduct (delta, delta) <= radius * radius;
        } else {
            return center.x () <= (rect.right () + radius);
        }
    } else {
        if (center.y () < rect.top ()) {
            return center.y () >= (rect.top () - radius);
        } else if (center.y () > rect.bottom ()) {
            return center.y () <= (rect.bottom () + radius);
        } else {
            return true;
        }
    }
}

std::optional<std::pair<uint32_t, const Unit&>> MatchState::unitUnderCursor (const Position& point) const
{
    for (std::map<uint32_t, Unit>::const_iterator it = units.begin (); it != units.end (); ++it) {
        const Unit& unit = it->second;
        if (checkUnitInsideSelection (unit, point))
            return std::pair<uint32_t, const Unit&> (it->first, unit);
    }
    return std::nullopt;
}
void MatchState::trySelect (Unit::Team team, const Position& point, bool add)
{
    if (add) {
        for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it->second;
            if (unit.team == team && checkUnitInsideSelection (unit, point))
                unit.selected = !unit.selected;
        }
    } else {
        for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it->second;
            if (unit.team == team && checkUnitInsideSelection (unit, point)) {
                clearSelection ();
                unit.selected = true;
                break;
            }
        }
    }
}
void MatchState::trySelect (Unit::Team team, const Rectangle& rect, bool add)
{
    if (add) {
        for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it->second;
            if (unit.team == team && checkUnitInsideSelection (unit, rect))
                unit.selected = true;
        }
    } else {
        bool selection_found = false;
        for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it->second;
            if (unit.team == team && checkUnitInsideSelection (unit, rect)) {
                selection_found = true;
                break;
            }
        }
        if (selection_found) {
            clearSelection ();
            for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
                Unit& unit = it->second;
                if (unit.team == team && checkUnitInsideSelection (unit, rect))
                    unit.selected = true;
            }
        }
    }
}
void MatchState::trySelectByType (Unit::Team team, const Position& point, const Rectangle& viewport, bool add)
{
    Unit* unit = findUnitAt (team, point);
    if (!unit)
        return;
    Unit::Type type = unit->type;
    if (!add)
        clearSelection ();
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.team == team && unit.type == type && checkUnitInsideViewport (unit, viewport))
            unit.selected = true;
    }
}
void MatchState::selectAll (Unit::Team team)
{
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.team == team)
            unit.selected = true;
    }
}
void MatchState::select (uint32_t unit_id, bool add)
{
    if (!add)
        clearSelection ();
    std::map<uint32_t, Unit>::iterator it = units.find (unit_id);
    if (it != units.end ()) {
        Unit& unit = it->second;
        unit.selected = true;
    }
}
void MatchState::deselect (uint32_t unit_id)
{
    std::map<uint32_t, Unit>::iterator it = units.find (unit_id);
    if (it != units.end ()) {
        Unit& unit = it->second;
        unit.selected = false;
    }
}
void MatchState::trimSelection (Unit::Type type, bool remove)
{
    if (remove) {
        for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it->second;
            if (unit.type == type)
                unit.selected = false;
        }
    } else {
        for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it->second;
            if (unit.type != type)
                unit.selected = false;
        }
    }
}
void MatchState::attackEnemy (Unit::Team attacker_team, const Position& point)
{
    std::optional<uint32_t> target;
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.team != attacker_team && checkUnitInsideSelection (unit, point)) {
            target = it->first;
            break;
        }
    }
    startAction (target.has_value () ? AttackAction (*target) : AttackAction (point));
}
void MatchState::cast (CastAction::Type cast_type, Unit::Team /* attacker_team */, const Position& point)
{
    startAction (CastAction (cast_type, point));
}
void MatchState::move (const Position& point)
{
    std::optional<uint32_t> target;
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (checkUnitInsideSelection (unit, point)) {
            target = it->first;
            break;
        }
    }
    startAction (target.has_value () ? MoveAction (*target) : MoveAction (point));
}
void MatchState::stop ()
{
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.selected) {
            unit.action = StopAction ();
            emit unitActionRequested (it->first, StopAction ());
        }
    }
}
void MatchState::autoAction (Unit::Team attacker_team, const Position& point)
{
    std::optional<uint32_t> target;
    bool target_is_enemy = false;
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (checkUnitInsideSelection (unit, point)) {
            target = it->first;
            target_is_enemy = it->second.team != attacker_team;
            break;
        }
    }
    if (target.has_value ()) {
        if (target_is_enemy) {
            startAction (AttackAction (*target));
        } else {
            startAction (MoveAction (*target));
            // emit unitActionRequested ();
        }
    } else {
        startAction (MoveAction (point));
        // emit unitActionRequested ();
    }
}
void MatchState::selectGroup (uint64_t group)
{
    uint64_t group_flag = 1 << group;
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        unit.selected = (unit.groups & group_flag) ? true : false;
    }
}
void MatchState::bindSelectionToGroup (uint64_t group)
{
    uint64_t group_flag = 1 << group;
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.selected)
            unit.groups |= group_flag;
        else
            unit.groups &= ~group_flag;
    }
}
void MatchState::addSelectionToGroup (uint64_t group)
{
    uint64_t group_flag = 1 << group;
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.selected)
            unit.groups |= group_flag;
    }
}
void MatchState::moveSelectionToGroup (uint64_t group, bool add)
{
    uint64_t group_flag = 1 << group;
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.selected)
            unit.groups = group_flag;
        else if (!add)
            unit.groups &= ~group_flag;
    }
}

void MatchState::clearSelection ()
{
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it)
        it->second.selected = false;
}
bool MatchState::checkUnitInsideSelection (const Unit& unit, const Position& point) const
{
    double radius = unitRadius (unit.type);
    if (radius == 0.0)
        return false;
    Offset delta = point - unit.position;
    return Offset::dotProduct (delta, delta) <= radius * radius;
}
bool MatchState::checkUnitInsideSelection (const Unit& unit, const Rectangle& rect) const
{
    double radius = unitRadius (unit.type);
    if (radius == 0.0)
        return false;
    return intersectRectangleCircle (rect, unit.position, radius);
}
bool MatchState::checkUnitInsideViewport (const Unit& unit, const Rectangle& viewport) const
{
    double radius = unitRadius (unit.type);
    if (radius == 0.0)
        return false;
    return intersectRectangleCircle (viewport, unit.position, radius);
}
Unit* MatchState::findUnitAt (Unit::Team team, const Position& point)
{
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.team == team && checkUnitInsideSelection (unit, point))
            return &unit;
    }
    return nullptr;
}
void MatchState::startAction (const MoveAction& action)
{
    // TODO: Check for team
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        uint32_t unit_id = it->first;
        Unit& unit = it->second;
        if (unit.selected &&
            (!std::holds_alternative<uint32_t> (action.target) || std::get<uint32_t> (action.target) != unit_id)) {
            if (std::holds_alternative<PerformingAttackAction> (unit.action))
                std::get<PerformingAttackAction> (unit.action).next_action = action;
            else if (std::holds_alternative<PerformingCastAction> (unit.action))
                std::get<PerformingCastAction> (unit.action).next_action = action;
            else
                unit.action = action;
            emit unitActionRequested (it->first, action);
        }
    }
}
void MatchState::startAction (const AttackAction& action)
{
    // TODO: Check for team
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.selected) {
            if (std::holds_alternative<PerformingAttackAction> (unit.action))
                std::get<PerformingAttackAction> (unit.action).next_action = action;
            else if (std::holds_alternative<PerformingCastAction> (unit.action))
                std::get<PerformingCastAction> (unit.action).next_action = action;
            else
                unit.action = action;
            emit unitActionRequested (it->first, action);
        }
    }
}
void MatchState::startAction (const CastAction& action)
{
    // TODO: Check for team
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.selected) {
            if (unit.type == Unit::Type::Contaminator) {
                switch (action.type) {
                case CastAction::Type::Pestilence:
                    if (std::holds_alternative<PerformingAttackAction> (unit.action))
                        std::get<PerformingAttackAction> (unit.action).next_action = action;
                    else if (std::holds_alternative<PerformingCastAction> (unit.action))
                        std::get<PerformingCastAction> (unit.action).next_action = action;
                    else
                        unit.action = action;
                    emit unitActionRequested (it->first, action);
                    break;
                case CastAction::Type::SpawnBeetle:
                    if (std::holds_alternative<PerformingAttackAction> (unit.action))
                        std::get<PerformingAttackAction> (unit.action).next_action = action;
                    else if (std::holds_alternative<PerformingCastAction> (unit.action))
                        std::get<PerformingCastAction> (unit.action).next_action = action;
                    else
                        unit.action = action;
                    emit unitActionRequested (it->first, action);
                    break;
                default:
                    break;
                }
                break;
            }
        }
    }
}
