#include "matchstate.h"


void MatchState::loadState (const std::vector<std::pair<uint32_t, Unit>>& units, const std::vector<std::pair<uint32_t, Corpse>>& corpses, const std::vector<std::pair<uint32_t, Missile>>& missiles)
{
    loadUnits (units);
    loadCorpses (corpses);
    loadMissiles (missiles);
}

void MatchState::loadUnits (const std::vector<std::pair<uint32_t, Unit>>& new_units)
{
    std::set<uint32_t> to_keep;
    for (const std::pair<uint32_t, Unit>& new_unit_entry: new_units)
        to_keep.insert (new_unit_entry.first);
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end ();) {
        if (to_keep.find (it->first) == to_keep.end ())
            it = units.erase (it);
        else
            ++it;
    }
    for (const std::pair<uint32_t, Unit>& new_unit_entry: new_units) {
        uint32_t new_unit_id = new_unit_entry.first;
        const Unit& new_unit = new_unit_entry.second;
        std::map<uint32_t, Unit>::iterator to_change = units.find (new_unit_id);
        if (to_change != units.end ()) {
            Unit& unit_to_change = to_change->second;
            unit_to_change.position = new_unit.position;
            unit_to_change.orientation = new_unit.orientation;
            unit_to_change.hp = new_unit.hp;
            unit_to_change.action = new_unit.action;
            unit_to_change.attack_cooldown_left_ticks = new_unit.attack_cooldown_left_ticks;
            unit_to_change.cast_cooldown_left_ticks = new_unit.cast_cooldown_left_ticks;
        } else {
            Unit& unit = addUnit (new_unit_id, new_unit.type, new_unit.team, new_unit.position, new_unit.orientation);
            unit.action = new_unit.action;
        }
    }
}
void MatchState::loadCorpses (const std::vector<std::pair<uint32_t, Corpse>>& new_corpses)
{
    std::set<uint32_t> to_keep;
    for (const std::pair<uint32_t, Corpse>& new_corpse_entry: new_corpses)
        to_keep.insert (new_corpse_entry.first);
    for (std::map<uint32_t, Corpse>::iterator it = corpses.begin (); it != corpses.end ();) {
        if (to_keep.find (it->first) == to_keep.end ())
            it = corpses.erase (it);
        else
            ++it;
    }
    for (const std::pair<uint32_t, Corpse>& new_corpse_entry: new_corpses) {
        uint32_t new_corpse_id = new_corpse_entry.first;
        const Corpse& new_corpse = new_corpse_entry.second;
        std::map<uint32_t, Corpse>::iterator to_change = corpses.find (new_corpse_id);
        if (to_change != corpses.end ()) {
            Corpse& corpse_to_change = to_change->second;
            corpse_to_change.unit.position = new_corpse.unit.position;
            corpse_to_change.unit.orientation = new_corpse.unit.orientation;
            corpse_to_change.unit.hp = new_corpse.unit.hp;
            corpse_to_change.unit.action = new_corpse.unit.action;
            corpse_to_change.unit.attack_cooldown_left_ticks = new_corpse.unit.attack_cooldown_left_ticks;
            corpse_to_change.unit.cast_cooldown_left_ticks = new_corpse.unit.cast_cooldown_left_ticks;
        } else {
            Corpse& corpse = addCorpse (new_corpse_id, new_corpse.unit.type, new_corpse.unit.team, new_corpse.unit.position, new_corpse.unit.orientation, new_corpse.decay_remaining_ticks);
            corpse.unit.action = new_corpse.unit.action;
        }
    }
}
void MatchState::loadMissiles (const std::vector<std::pair<uint32_t, Missile>>& new_missiles)
{
    std::set<uint32_t> m_to_keep;
    for (uint32_t i = 0; i < new_missiles.size (); i++) {
        m_to_keep.insert (new_missiles.at (i).first);
    }
    auto m_it = missiles.begin ();
    while (m_it != missiles.end ()) {
        if (m_to_keep.find (m_it->first) == m_to_keep.end ())
            m_it = missiles.erase (m_it);
        else
            ++m_it;
    }

    for (uint32_t i = 0; i < new_missiles.size (); i++) {
        if (missiles.find (new_missiles.at (i).first) == missiles.end ()) {
            Missile& missile = addMissile (new_missiles.at (i).first, new_missiles.at (i).second.type, new_missiles.at (i).second.sender_team, new_missiles.at (i).second.position, new_missiles.at (i).second.orientation);
            missile.target_position = new_missiles.at (i).second.target_position;
            if (new_missiles.at (i).second.target_unit.has_value ()) {
                missile.target_unit = new_missiles.at (i).second.target_unit;
            } else {
                missile.target_unit.reset ();
            }

        } else {
            std::map<uint32_t, Missile>::iterator to_change = missiles.find (new_missiles.at (i).first);
            Missile& missile_to_change = to_change->second;
            missile_to_change.position = new_missiles.at (i).second.position;
            missile_to_change.orientation = new_missiles.at (i).second.orientation;
            missile_to_change.target_position = new_missiles.at (i).second.target_position;
            if (new_missiles.at (i).second.target_unit.has_value ()) {
                missile_to_change.target_unit = new_missiles.at (i).second.target_unit;
            } else {
                missile_to_change.target_unit.reset ();
            }
        }
    }
}
Unit& MatchState::addUnit (uint32_t id, Unit::Type type, Unit::Team team, const Position& position, double direction)
{
    std::pair<std::map<uint32_t, Unit>::iterator, bool> it_status = units.insert ({id, {type, uint32_t (random_generator ()), team, position, direction}});
    Unit& unit = it_status.first->second;
    unit.hp = unitMaxHP (unit.type);
    return unit;
}
Corpse& MatchState::addCorpse (uint32_t id, Unit::Type type, Unit::Team team, const Position& position, double direction, int64_t decay_remaining_ticks)
{
    std::pair<std::map<uint32_t, Corpse>::iterator, bool> it_status = corpses.insert ({id, {{type, uint32_t (random_generator ()), team, position, direction}, decay_remaining_ticks}});
    Corpse& corpse = it_status.first->second;
    corpse.unit.hp = unitMaxHP (corpse.unit.type);
    return corpse;
}
Missile& MatchState::addMissile (uint32_t id, Missile::Type type, Unit::Team team, const Position& position, double /* direction */)
{
    std::pair<std::map<uint32_t, Missile>::iterator, bool> it_status = missiles.insert ({id, {type, team, position, 0, Position (0, 0)}});
    Missile& missile = it_status.first->second;
    return missile;
}
