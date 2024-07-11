#include "matchstate.h"


std::map<uint32_t, Unit>::iterator MatchState::createUnit (Unit::Type type, Unit::Team team, const Position& position, double direction)
{
    uint32_t id = getRandomNumber (); // TODO: Fix it

    std::pair<std::map<uint32_t, Unit>::iterator, bool> it_status = units.insert ({next_id++, {type, id, team, position, direction}});
    Unit& unit = it_status.first->second;
    unit.hp = unitMaxHP (unit.type);
    if (type == Unit::Type::Beetle)
        unit.ttl_ticks = MatchState::beetleTTLTicks ();
    return it_status.first;
}
void MatchState::setUnitAction (uint32_t unit_id, const UnitActionVariant& action)
{
    std::map<uint32_t, Unit>::iterator it = units.find (unit_id);
    if (it == units.end ())
        return;
    Unit& unit = it->second;
    IntentiveActionVariant* next_action;
    if (std::holds_alternative<PerformingAttackAction> (unit.action))
        next_action = &std::get<PerformingAttackAction> (unit.action).next_action;
    else if (std::holds_alternative<PerformingCastAction> (unit.action))
        next_action = &std::get<PerformingCastAction> (unit.action).next_action;
    else
        next_action = nullptr;
    if (next_action) {
        if (std::holds_alternative<StopAction> (action))
            *next_action = std::get<StopAction> (action);
        else if (std::holds_alternative<MoveAction> (action))
            *next_action = std::get<MoveAction> (action);
        else if (std::holds_alternative<AttackAction> (action))
            *next_action = std::get<AttackAction> (action);
        else if (std::holds_alternative<CastAction> (action))
            *next_action = std::get<CastAction> (action);
    } else {
        if (std::holds_alternative<StopAction> (action))
            unit.action = std::get<StopAction> (action);
        else if (std::holds_alternative<MoveAction> (action))
            unit.action = std::get<MoveAction> (action);
        else if (std::holds_alternative<AttackAction> (action))
            unit.action = std::get<AttackAction> (action);
        else if (std::holds_alternative<CastAction> (action))
            unit.action = std::get<CastAction> (action);
        else if (std::holds_alternative<PerformingAttackAction> (action))
            unit.action = std::get<PerformingAttackAction> (action);
        else if (std::holds_alternative<PerformingCastAction> (action))
            unit.action = std::get<PerformingCastAction> (action);
    }
}

uint32_t MatchState::getRandomNumber ()
{
    uint32_t id = uint32_t (random_generator ());
    id = id % (1 << 30);
    return id;
}
