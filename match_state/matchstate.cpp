#include "matchstate.h"

#include "positionaverage.h"


static constexpr double SQRT_1_2 = 0.70710678118655;


static inline double unitDistance (const Unit& a, const Unit& b)
{
    return (a.position - b.position).length ();
}
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
static bool pointInsideCircle (const Position& point, const Position& center, double radius)
{
    return (point - center).length () <= radius;
}
static bool orientationsFuzzyMatch (double a, double b)
{
    return qAbs (remainder (a - b, M_PI * 2.0)) <= (M_PI / 180.0); // Within 1 degree
}

MatchState::MatchState (bool is_client)
    : is_client (is_client)
{
}
MatchState::~MatchState ()
{
}

uint64_t MatchState::clockNS () const
{
    return clock_ns;
}
const Rectangle& MatchState::areaRef () const
{
    return area;
}
const std::map<uint32_t, Unit>& MatchState::unitsRef () const
{
    return units;
}
const std::map<uint32_t, Corpse>& MatchState::corpsesRef () const
{
    return corpses;
}
const std::map<uint32_t, Missile>& MatchState::missilesRef () const
{
    return missiles;
}
const std::map<uint32_t, Explosion>& MatchState::explosionsRef () const
{
    return explosions;
}

uint32_t MatchState::getRandomNumber ()
{
    uint32_t id = uint32_t (random_generator ());
    id = id % (1 << 30);
    id = id + (int (is_client) << 30);
    return id;
}
std::map<uint32_t, Unit>::iterator MatchState::createUnit (Unit::Type type, Unit::Team team, const Position& position, double direction)
{
    uint32_t id = getRandomNumber (); // TODO: Fix it

    std::pair<std::map<uint32_t, Unit>::iterator, bool> it_status = units.insert ({next_id++, {type, id, team, position, direction}});
    Unit& unit = it_status.first->second;
    unit.hp = unitMaxHP (unit.type);
    return it_status.first;
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
Missile& MatchState::addMissile (uint32_t id, Missile::Type type, Unit::Team team, const Position& position, double direction)
{
    std::pair<std::map<uint32_t, Missile>::iterator, bool> it_status = missiles.insert ({id, {type, team, position, 0, Position (0, 0)}});
    Missile& missile = it_status.first->second;
    return missile;
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
void MatchState::deselect (uint32_t unit_id)
{
    std::map<uint32_t, Unit>::iterator it = units.find (unit_id);
    if (it != units.end ()) {
        Unit& unit = it->second;
        unit.selected = false;
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
std::optional<Position> MatchState::selectionCenter () const
{
    PositionAverage average;
    for (std::map<uint32_t, Unit>::const_iterator it = units.cbegin (); it != units.cend (); ++it) {
        const Unit& unit = it->second;
        if (unit.selected)
            average.add (unit.position);
    }
    return average.get ();
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
void MatchState::redTeamUserTick (RedTeamUserData& /* user_data */)
{
}
void MatchState::blueTeamUserTick (BlueTeamUserData& user_data)
{
    // TODO: Define proper API
    std::vector<const Missile*> new_missiles;
    for (std::map<uint32_t, Missile>::iterator it = missiles.begin (); it != missiles.end (); ++it) {
        if (!user_data.old_missiles.count (it->first)) {
            if (it->second.sender_team == Unit::Team::Red)
                new_missiles.push_back (&it->second);
            user_data.old_missiles.insert (it->first);
        }
    }

    for (std::set<uint32_t>::iterator it = user_data.old_missiles.begin (); it != user_data.old_missiles.end ();) {
        if (!missiles.count (*it))
            it = user_data.old_missiles.erase (it);
        else
            ++it;
    }

    const AttackDescription& pestilence_splash_attack = MatchState::effectAttackDescription (AttackDescription::Type::PestilenceSplash);
    const AttackDescription& rocket_explosion_attack = MatchState::effectAttackDescription (AttackDescription::Type::GoonRocketExplosion);
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.team == Unit::Team::Blue) {
            for (const Missile* missile: new_missiles) {
                if (missile->type == Missile::Type::Pestilence) {
                    double radius = pestilence_splash_attack.range + unitRadius (unit.type);
                    if (pointInsideCircle (unit.position, missile->target_position, radius)) {
                        Offset displacement = unit.position - missile->target_position;
                        displacement.setLength (radius * 1.05);
                        unit.action = MoveAction (missile->target_position + displacement);
                        break;
                    }
                } else if (missile->type == Missile::Type::Rocket) {
                    double radius = rocket_explosion_attack.range + unitRadius (unit.type);
                    if (pointInsideCircle (unit.position, missile->target_position, radius) &&
                        (!missile->target_unit.has_value () || missile->target_unit.value () != it->first)) {
                        Offset displacement = unit.position - missile->target_position;
                        displacement.setLength (radius * 1.05);
                        unit.action = MoveAction (missile->target_position + displacement);
                        break;
                    }
                }
            }
        }
    }
}
uint32_t MatchState::get_tick_no ()
{
    return tick_no;
}
void MatchState::tick ()
{
    tick_no += 1;
    redTeamUserTick (red_team_user_data);
    blueTeamUserTick (blue_team_user_data);

    constexpr uint64_t dt_nsec = 20'000'000;
    constexpr double dt = 0.020;

    clock_ns += dt_nsec;

    // TODO: Implement current and new positions for actions

    applyActions (dt);
    applyEffects (dt);
    applyAreaBoundaryCollisions (dt);
    applyUnitCollisions (dt); // TODO: Possibly optimize fron O (N^2) to O (N*log (N))
    applyDeath ();
    applyDecay ();
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
void MatchState::clearSelection ()
{
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it)
        it->second.selected = false;
}
void MatchState::rotateUnit (Unit& unit, double dt, double dest_orientation)
{
    double delta = remainder (dest_orientation - unit.orientation, M_PI * 2.0);
    double max_delta = dt * unitMaxAngularVelocity (unit.type);
    if (qAbs (delta) < max_delta) {
        unit.orientation = dest_orientation;
    } else {
        if (delta >= 0.0)
            unit.orientation += max_delta;
        else
            unit.orientation -= max_delta;
    }
}
double MatchState::unitDiameter (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return 2.0 / 3.0;
    case Unit::Type::Crusader:
        return 1.0;
    case Unit::Type::Goon:
        return 1.0;
    case Unit::Type::Beetle:
        return 0.5;
    case Unit::Type::Contaminator:
        return 1.5;
    default:
        return 0.0;
    }
}
double MatchState::missileDiameter (Missile::Type type)
{
    switch (type) {
    case Missile::Type::Rocket:
        return 0.5;
    case Missile::Type::Pestilence:
        return 0.5;
    default:
        return 0.0;
    }
}
double MatchState::explosionDiameter (Explosion::Type type)
{
    switch (type) {
    case Explosion::Type::Fire:
        return 3.0;
    case Explosion::Type::Pestilence:
        return 3.0;
    default:
        return 0.0;
    }
}
double MatchState::unitRadius (Unit::Type type)
{
    return unitDiameter (type) * 0.5;
}
double MatchState::unitMaxVelocity (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return 4.0;
    case Unit::Type::Crusader:
        return 7.0;
    case Unit::Type::Goon:
        return 4.0;
    case Unit::Type::Beetle:
        return 5.0;
    case Unit::Type::Contaminator:
        return 3.5;
    default:
        return 0.0;
    }
}
double MatchState::unitMaxAngularVelocity (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return M_PI * 4.0;
    case Unit::Type::Crusader:
        return M_PI * 7.0;
    case Unit::Type::Goon:
        return M_PI * 4.0;
    case Unit::Type::Beetle:
        return M_PI * 7.0;
    case Unit::Type::Contaminator:
        return M_PI * 4.0;
    default:
        return 0.0;
    }
}
int MatchState::unitHitBarCount (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return 4;
    case Unit::Type::Crusader:
        return 5;
    case Unit::Type::Goon:
        return 5;
    case Unit::Type::Beetle:
        return 4;
    case Unit::Type::Contaminator:
        return 7;
    default:
        return 0;
    }
}
int MatchState::unitMaxHP (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return 40;
    case Unit::Type::Crusader:
        return 100;
    case Unit::Type::Goon:
        return 80;
    case Unit::Type::Beetle:
        return 20;
    case Unit::Type::Contaminator:
        return 120;
    default:
        return 0;
    }
}
const AttackDescription& MatchState::unitPrimaryAttackDescription (Unit::Type type)
{
    static const AttackDescription seal = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::SealShot;
        ret.range = 5.0;
        ret.trigger_range = 7.0;
        ret.damage = 10;
        ret.duration_ticks = 20;
        ret.cooldown_ticks = 30;
        ret;
    });
    static const AttackDescription crusader = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::CrusaderChop;
        ret.range = 0.1;
        ret.trigger_range = 4.0;
        ret.damage = 16;
        ret.duration_ticks = 40;
        ret.cooldown_ticks = 30;
        ret;
    });
    static const AttackDescription goon = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::GoonRocket;
        ret.range = 8.0;
        ret.trigger_range = 9.0;
        ret.damage = 12;
        ret.missile_velocity = 16.0;
        ret.duration_ticks = 60;
        ret.cooldown_ticks = 30;
        ret;
    });
    static const AttackDescription beetle = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::BeetleSlice;
        ret.range = 0.1;
        ret.trigger_range = 3.0;
        ret.damage = 8;
        ret.duration_ticks = 40;
        ret.cooldown_ticks = 30;
        ret;
    });
    static const AttackDescription unkown = {};

    switch (type) {
    case Unit::Type::Seal:
        return seal;
    case Unit::Type::Crusader:
        return crusader;
    case Unit::Type::Goon:
        return goon;
    case Unit::Type::Beetle:
        return beetle;
    default:
        return unkown;
    }
}
const AttackDescription& MatchState::effectAttackDescription (AttackDescription::Type type)
{
    static const AttackDescription goon_rocket_explosion = ({
        AttackDescription ret;
        ret.type = type;
        ret.range = 1.4;
        ret.damage = 8;
        ret.duration_ticks = 20;
        ret;
    });
    static const AttackDescription pestilence_missile = ({
        AttackDescription ret;
        ret.type = type;
        ret.range = 7.0;
        ret.damage = 0;
        ret.missile_velocity = 16.0;
        ret.duration_ticks = 10;
        ret.cooldown_ticks = 40;
        ret;
    });
    static const AttackDescription pestilence_splash = ({
        AttackDescription ret;
        ret.type = type;
        ret.range = 1.8;
        ret.damage = 6;
        ret.duration_ticks = 20;
        ret.friendly_fire = false;
        ret;
    });
    static const AttackDescription spawn_beetle = ({
        AttackDescription ret;
        ret.type = type;
        ret.range = 4;
        ret.duration_ticks = 20;
        ret.cooldown_ticks = 20;
        ret;
    });
    static const AttackDescription unkown = {};

    switch (type) {
    case AttackDescription::Type::GoonRocketExplosion:
        return goon_rocket_explosion;
    case AttackDescription::Type::PestilenceMissile:
        return pestilence_missile;
    case AttackDescription::Type::PestilenceSplash:
        return pestilence_splash;
    case AttackDescription::Type::SpawnBeetle:
        return spawn_beetle;
    default:
        return unkown;
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
bool MatchState::fuzzyMatchPoints (const Position& p1, const Position& p2) const
{
    const double ratio = 0.5;
    return (p2 - p1).length () < unitDiameter (Unit::Type::Beetle) * ratio;
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
void MatchState::LoadState (const std::vector<std::pair<uint32_t, Unit>>& units, const std::vector<std::pair<uint32_t, Corpse>>& corpses, const std::vector<std::pair<uint32_t, Missile>>& missiles)
{
    loadUnits (units);
    loadCorpses (corpses);
    loadMissiles (missiles);
}
void MatchState::startAction (const MoveAction& action)
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
            Position target;
            if (std::holds_alternative<uint32_t> (action.target)) {
                uint32_t target_unit_id = std::get<uint32_t> (action.target);
                std::map<uint32_t, Unit>::iterator target_unit_it = units.find (target_unit_id);
                if (target_unit_it != units.end ()) {
                    Unit& target_unit = target_unit_it->second;
                    target = target_unit.position;
                }
            } else {
                target = std::get<Position> (action.target);
            }
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

void MatchState::emitMissile (Missile::Type missile_type, const Unit& unit, uint32_t target_unit_id, const Unit& target_unit)
{
    missiles.insert ({next_id++, {missile_type, unit.team, unit.position, target_unit_id, target_unit.position}});
}
void MatchState::emitMissile (Missile::Type missile_type, const Unit& unit, const Position& target)
{
    missiles.insert ({next_id++, {missile_type, unit.team, unit.position, target}});
}
void MatchState::emitExplosion (Explosion::Type explosion_type, Unit::Team sender_team, const Position& position)
{
    const AttackDescription& attack_description = ({
        const AttackDescription* ret;
        switch (explosion_type) {
        case Explosion::Type::Fire:
            ret = &effectAttackDescription (AttackDescription::Type::GoonRocketExplosion);
            break;
        case Explosion::Type::Pestilence:
            ret = &effectAttackDescription (AttackDescription::Type::PestilenceSplash);
            break;
        default:
            return;
        }
        *ret;
    });

    explosions.insert ({next_id++, {explosion_type, position, attack_description.duration_ticks}});

    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& target_unit = it->second;
        if (attack_description.friendly_fire || target_unit.team != sender_team) {
            if ((target_unit.position - position).length () <= attack_description.range + unitRadius (target_unit.type))
                dealDamage (target_unit, attack_description.damage);
        }
    }
    switch (explosion_type) {
    case Explosion::Type::Fire:
        emit soundEventEmitted (SoundEvent::RocketExplosion);
        break;
    case Explosion::Type::Pestilence:
        emit soundEventEmitted (SoundEvent::PestilenceSplash);
        break;
    default:
        break;
    }
}
void MatchState::applyAreaBoundaryCollisions (double dt)
{
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        applyAreaBoundaryCollision (unit, dt);
    }
}
void MatchState::applyActions (double dt)
{
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        if (unit.attack_cooldown_left_ticks > 0)
            --unit.attack_cooldown_left_ticks;
        if (unit.cast_cooldown_left_ticks > 0)
            --unit.cast_cooldown_left_ticks;
        if (std::holds_alternative<StopAction> (unit.action)) {
            StopAction& stop_action = std::get<StopAction> (unit.action);
            std::optional<uint32_t> closest_target = stop_action.current_target;
            if (!closest_target.has_value ())
                closest_target = findClosestTarget (unit);
            if (closest_target.has_value ()) {
                stop_action.current_target = closest_target;
                if (!unit.attack_cooldown_left_ticks) {
                    uint32_t target_unit_id = stop_action.current_target.value ();
                    std::map<uint32_t, Unit>::iterator target_unit_it = units.find (target_unit_id);
                    if (target_unit_it != units.end ()) {
                        Unit& target_unit = target_unit_it->second;
                        if (applyAttack (unit, target_unit_id, target_unit, dt))
                            unit.action = PerformingAttackAction (StopAction (std::get<StopAction> (unit.action)), unitPrimaryAttackDescription (unit.type).duration_ticks);
                    } else {
                        stop_action.current_target.reset ();
                    }
                }
            }
        } else if (std::holds_alternative<MoveAction> (unit.action)) {
            const std::variant<Position, uint32_t>& target = std::get<MoveAction> (unit.action).target;
            if (std::holds_alternative<Position> (target)) {
                const Position& target_position = std::get<Position> (target);
                applyMovement (unit, target_position, dt, true);
            } else if (std::holds_alternative<uint32_t> (target)) {
                uint32_t target_unit_id = std::get<uint32_t> (target);
                std::map<uint32_t, Unit>::iterator target_unit_it = units.find (target_unit_id);
                if (target_unit_it != units.end ()) {
                    Unit& target_unit = target_unit_it->second;
                    applyMovement (unit, target_unit.position, dt, false);
                } else {
                    unit.action = StopAction ();
                }
            }
        } else if (std::holds_alternative<AttackAction> (unit.action)) {
            AttackAction& attack_action = std::get<AttackAction> (unit.action);
            const std::variant<Position, uint32_t>& target = attack_action.target;
            if (std::holds_alternative<Position> (target)) {
                std::optional<uint32_t> closest_target = attack_action.current_target;
                if (!closest_target.has_value ())
                    closest_target = findClosestTarget (unit);
                if (closest_target.has_value ()) {
                    attack_action.current_target = closest_target;
                    if (!unit.attack_cooldown_left_ticks) {
                        uint32_t target_unit_id = attack_action.current_target.value ();
                        std::map<uint32_t, Unit>::iterator target_unit_it = units.find (target_unit_id);
                        if (target_unit_it != units.end ()) {
                            Unit& target_unit = target_unit_it->second;
                            if (applyAttack (unit, target_unit_id, target_unit, dt))
                                unit.action = PerformingAttackAction (AttackAction (std::get<AttackAction> (unit.action)), unitPrimaryAttackDescription (unit.type).duration_ticks);
                        } else {
                            attack_action.current_target.reset ();
                        }
                    }
                } else {
                    applyMovement (unit, std::get<Position> (target), dt, true);
                }
            } else if (std::holds_alternative<uint32_t> (target)) {
                if (!unit.attack_cooldown_left_ticks) {
                    uint32_t target_unit_id = std::get<uint32_t> (target);
                    std::map<uint32_t, Unit>::iterator target_unit_it = units.find (target_unit_id);
                    if (target_unit_it != units.end ()) {
                        Unit& target_unit = target_unit_it->second;
                        if (applyAttack (unit, target_unit_id, target_unit, dt))
                            unit.action = PerformingAttackAction (AttackAction (std::get<AttackAction> (unit.action)), unitPrimaryAttackDescription (unit.type).duration_ticks);
                    } else {
                        unit.action = StopAction ();
                    }
                }
            }
        } else if (std::holds_alternative<CastAction> (unit.action)) {
            const CastAction& cast_action = std::get<CastAction> (unit.action);
            if (applyCast (unit, cast_action.type, cast_action.target, dt)) {
                int64_t duration_ticks;
                switch (cast_action.type) {
                case CastAction::Type::Pestilence:
                    duration_ticks = MatchState::effectAttackDescription (AttackDescription::Type::PestilenceMissile).duration_ticks;
                    break;
                case CastAction::Type::SpawnBeetle:
                    duration_ticks = MatchState::effectAttackDescription (AttackDescription::Type::SpawnBeetle).duration_ticks;
                    break;
                default:
                    duration_ticks = 0;
                }
                unit.action = PerformingCastAction (cast_action.type, StopAction (), duration_ticks);
            }
        } else if (std::holds_alternative<PerformingAttackAction> (unit.action)) {
            PerformingAttackAction& performing_attack_action = std::get<PerformingAttackAction> (unit.action);
            --performing_attack_action.remaining_ticks;
            if (performing_attack_action.remaining_ticks <= 0) {
                const AttackDescription& attack_description = unitPrimaryAttackDescription (Unit::Type::Goon);
                unit.attack_cooldown_left_ticks = attack_description.cooldown_ticks;
                if (std::holds_alternative<StopAction> (performing_attack_action.next_action))
                    unit.action = StopAction (std::get<StopAction> (performing_attack_action.next_action));
                else if (std::holds_alternative<AttackAction> (performing_attack_action.next_action))
                    unit.action = AttackAction (std::get<AttackAction> (performing_attack_action.next_action));
                else if (std::holds_alternative<MoveAction> (performing_attack_action.next_action))
                    unit.action = MoveAction (std::get<MoveAction> (performing_attack_action.next_action));
                else if (std::holds_alternative<CastAction> (performing_attack_action.next_action))
                    unit.action = CastAction (std::get<CastAction> (performing_attack_action.next_action));
                else
                    unit.action = StopAction ();
            }
        } else if (std::holds_alternative<PerformingCastAction> (unit.action)) {
            PerformingCastAction& performing_cast_action = std::get<PerformingCastAction> (unit.action);
            --performing_cast_action.remaining_ticks;
            if (performing_cast_action.remaining_ticks <= 0) {
                switch (performing_cast_action.cast_type) {
                case CastAction::Type::Pestilence:
                    unit.cast_cooldown_left_ticks = MatchState::effectAttackDescription (AttackDescription::Type::PestilenceMissile).cooldown_ticks;
                    break;
                case CastAction::Type::SpawnBeetle:
                    unit.cast_cooldown_left_ticks = MatchState::effectAttackDescription (AttackDescription::Type::SpawnBeetle).cooldown_ticks;
                    break;
                default:
                    break;
                }
                if (std::holds_alternative<StopAction> (performing_cast_action.next_action))
                    unit.action = StopAction (std::get<StopAction> (performing_cast_action.next_action));
                else if (std::holds_alternative<AttackAction> (performing_cast_action.next_action))
                    unit.action = AttackAction (std::get<AttackAction> (performing_cast_action.next_action));
                else if (std::holds_alternative<MoveAction> (performing_cast_action.next_action))
                    unit.action = MoveAction (std::get<MoveAction> (performing_cast_action.next_action));
                else if (std::holds_alternative<CastAction> (performing_cast_action.next_action))
                    unit.action = CastAction (std::get<CastAction> (performing_cast_action.next_action));
                else
                    unit.action = StopAction ();
            }
        }
    }
}
void MatchState::applyEffects (double dt)
{
    applyMissilesMovement (dt);
    applyExplosionEffects (dt);
}
void MatchState::applyMissilesMovement (double dt)
{
    for (std::map<uint32_t, Missile>::iterator it = missiles.begin (); it != missiles.end ();) {
        Missile& missile = it->second;
        if (missile.target_unit.has_value ()) {
            uint32_t target_unit_id = *missile.target_unit;
            std::map<uint32_t, Unit>::iterator target_unit_it = units.find (target_unit_id);
            if (target_unit_it != units.end ()) {
                missile.target_position = target_unit_it->second.position;
                Offset direction = missile.target_position - missile.position;
                missile.orientation = direction.orientation ();
            } else {
                missile.target_unit.reset ();
            }
        }
        Offset displacement = missile.target_position - missile.position;
        double max_velocity = 0.0;
        switch (missile.type) {
        case Missile::Type::Rocket: {
            const AttackDescription& attack_description = unitPrimaryAttackDescription (Unit::Type::Goon);
            max_velocity = attack_description.missile_velocity;
        } break;
        case Missile::Type::Pestilence: {
            const AttackDescription& attack_description = effectAttackDescription (AttackDescription::Type::PestilenceMissile);
            max_velocity = attack_description.missile_velocity;
        } break;
        default: {
        }
        }
        double path_length = max_velocity * dt;
        double displacement_length = displacement.length ();
        if (displacement_length <= path_length) {
            if (missile.target_unit.has_value ()) {
                uint32_t target_unit_id = *missile.target_unit;
                std::map<uint32_t, Unit>::iterator target_unit_it = units.find (target_unit_id);
                if (target_unit_it != units.end ()) {
                    Unit& target_unit = target_unit_it->second;
                    switch (missile.type) {
                    case Missile::Type::Rocket: {
                        const AttackDescription& attack_description = unitPrimaryAttackDescription (Unit::Type::Goon);
                        dealDamage (target_unit, attack_description.damage);
                    } break;
                    default: {
                    }
                    }
                }
            }
            switch (missile.type) {
            case Missile::Type::Rocket:
                emitExplosion (Explosion::Type::Fire, missile.sender_team, missile.target_position);
                break;
            case Missile::Type::Pestilence:
                emitExplosion (Explosion::Type::Pestilence, missile.sender_team, missile.target_position);
                break;
            default:
                break;
            }
            it = missiles.erase (it);
        } else {
            missile.position += displacement * (path_length / displacement_length);
            ++it;
        }
    }
}
void MatchState::applyExplosionEffects (double /* dt */)
{
    for (std::map<uint32_t, Explosion>::iterator it = explosions.begin (); it != explosions.end ();) {
        Explosion& explosion = it->second;
        if (--explosion.remaining_ticks > 0)
            ++it;
        else
            it = explosions.erase (it);
    }
}
void MatchState::applyMovement (Unit& unit, const Position& target_position, double dt, bool clear_action_on_completion)
{
    double max_velocity = unitMaxVelocity (unit.type);
    Offset displacement = target_position - unit.position;
    rotateUnit (unit, dt, displacement.orientation ());
    double path_length = max_velocity * dt;
    double displacement_length = displacement.length ();
    if (displacement_length <= path_length) {
        unit.position = target_position;
        if (clear_action_on_completion)
            unit.action = StopAction ();
    } else {
        unit.position += displacement * (path_length / displacement_length);
    }
}
bool MatchState::applyAttack (Unit& unit, uint32_t target_unit_id, Unit& target_unit, double dt)
{
    const AttackDescription& attack_description = unitPrimaryAttackDescription (unit.type);
    Offset displacement = target_unit.position - unit.position;
    double target_orientation = displacement.orientation ();
    rotateUnit (unit, dt, target_orientation);
    double displacement_length = displacement.length ();
    double full_attack_range = attack_description.range + unitRadius (unit.type) + unitRadius (target_unit.type);
    bool in_range = false;
    if (displacement_length > full_attack_range) {
        double path_length = unitMaxVelocity (unit.type) * dt;
        if (path_length >= displacement_length - full_attack_range) {
            path_length = displacement_length - full_attack_range;
            in_range = true;
        }
        unit.position += displacement * (path_length / displacement_length);
    } else {
        in_range = true;
    }
    if (in_range && orientationsFuzzyMatch (unit.orientation, target_orientation)) {
        switch (attack_description.type) {
        case AttackDescription::Type::SealShot: {
            dealDamage (target_unit, attack_description.damage);
            emit soundEventEmitted (SoundEvent::SealAttack);
        } return true;
        case AttackDescription::Type::CrusaderChop: {
            dealDamage (target_unit, attack_description.damage);
            emit soundEventEmitted (SoundEvent::CrusaderAttack);
        } return true;
        case AttackDescription::Type::GoonRocket: {
            emitMissile (Missile::Type::Rocket, unit, target_unit_id, target_unit);
            emit soundEventEmitted (SoundEvent::RocketStart);
        } return true;
        case AttackDescription::Type::BeetleSlice: {
            dealDamage (target_unit, attack_description.damage);
            emit soundEventEmitted (SoundEvent::BeetleAttack);
        } return true;
        default: {
        }
        }
    }
    return false;
}
bool MatchState::applyCast (Unit& unit, CastAction::Type cast_type, const Position& target, double dt)
{
    const AttackDescription& attack_description = ({
        const AttackDescription* ret;
        switch (cast_type) {
        case CastAction::Type::Pestilence:
            ret = &effectAttackDescription (AttackDescription::Type::PestilenceMissile);
            break;
        case CastAction::Type::SpawnBeetle:
            ret = &effectAttackDescription (AttackDescription::Type::SpawnBeetle);
            break;
        default:
            return false;
        }
        *ret;
    });
    Offset displacement = target - unit.position;
    double target_orientation = displacement.orientation ();
    rotateUnit (unit, dt, target_orientation);
    double displacement_length = displacement.length ();
    double full_attack_range = attack_description.range + unitRadius (unit.type);
    bool in_range = false;
    if (displacement_length > full_attack_range) {
        double path_length = unitMaxVelocity (unit.type) * dt;
        if (path_length >= displacement_length - full_attack_range) {
            path_length = displacement_length - full_attack_range;
            in_range = true;
        }
        unit.position += displacement * (path_length / displacement_length);
    } else {
        in_range = true;
    }
    if (!unit.cast_cooldown_left_ticks &&
        !std::holds_alternative<PerformingAttackAction> (unit.action) &&
        !std::holds_alternative<PerformingCastAction> (unit.action) &&
        in_range &&
        orientationsFuzzyMatch (unit.orientation, target_orientation)) {
        switch (cast_type) {
        case CastAction::Type::Pestilence:
            emitMissile (Missile::Type::Pestilence, unit, target);
            emit soundEventEmitted (SoundEvent::PestilenceMissileStart);
            return true;
        case CastAction::Type::SpawnBeetle:
            // createUnit (Unit::Type::Beetle, unit.team, target, unit.orientation);
            emit unitCreateRequested (unit.team, Unit::Type::Beetle, target);
            emit soundEventEmitted (SoundEvent::SpawnBeetle);
            return true;
        default:
            return false;
        }
    }
    return false;
}
void MatchState::applyAreaBoundaryCollision (Unit& unit, double dt)
{
    Position& position = unit.position;
    double unit_radius = unitRadius (unit.type);
    double max_velocity = unitMaxVelocity (unit.type) * 2.0;
    Offset off;
    if (position.x () < (area.left () + unit_radius)) {
        off.setDX ((area.left () + unit_radius) - position.x ());
    } else if (position.x () > (area.right () - unit_radius)) {
        off.setDX ((area.right () - unit_radius) - position.x ());
    }
    if (position.y () < (area.top () + unit_radius)) {
        off.setDY ((area.top () + unit_radius) - position.y ());
    } else if (position.y () > (area.bottom () - unit_radius)) {
        off.setDY ((area.bottom () - unit_radius) - position.y ());
    }
    double path_length = max_velocity * dt;
    double square_length = Offset::dotProduct (off, off);
    if (square_length <= path_length * path_length) {
        position += off;
    } else {
        position += off * (path_length / qSqrt (square_length));
    }
}
void MatchState::applyUnitCollisions (double dt)
{
    // Apply unit collisions: O (N^2)
    std::vector<Offset> offsets;
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it->second;
        double unit_radius = unitRadius (unit.type);
        Offset off;
        for (std::map<uint32_t, Unit>::iterator related_it = units.begin (); related_it != units.end (); ++related_it) {
            if (related_it == it)
                continue;
            Unit& related_unit = related_it->second;
            double related_unit_radius = unitRadius (related_unit.type);
            double min_distance = unit_radius + related_unit_radius;
            Offset delta = unit.position - related_unit.position;
            if (delta.length () < min_distance) {
                double delta_length = qSqrt (Offset::dotProduct (delta, delta));
                if (delta_length < 0.00001) {
                    double dx, dy;
                    double angle = double (M_PI * 2.0 * random_generator ()) / (double (std::numeric_limits<uint32_t>::max ()) + 1.0);
                    sincos (angle, &dy, &dx);
                    delta = {dx * min_distance, dy * min_distance};
                } else {
                    double distance_to_comfort = min_distance - delta_length;
                    delta *= distance_to_comfort / delta_length;
                }
                off += delta;
            }
        }
        offsets.push_back (off);
    }
    {
        size_t i = 0;
        for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it->second;
            Position& position = unit.position;
            Offset off = offsets[i++];
            double max_velocity = unitMaxVelocity (unit.type) * 0.9; // TODO: Make force depend on distance
            double path_length = max_velocity * dt;
            double length = off.length ();
            position += (length <= path_length) ? off : off * (path_length / length);
        }
    }
}
void MatchState::dealDamage (Unit& unit, int64_t damage)
{
    unit.hp = std::max<int64_t> (unit.hp - damage, 0);
}

void MatchState::applyDeath ()
{
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end ();) {
        Unit& unit = it->second;
        if (unit.hp <= 0) {
            corpses.emplace (it->first, unit);
            it = units.erase (it);
        } else {
            ++it;
        }
    }
}
void MatchState::applyDecay ()
{
    for (std::map<uint32_t, Corpse>::iterator it = corpses.begin (); it != corpses.end ();) {
        Corpse& corpse = it->second;
        if (corpse.decay_remaining_ticks <= 1) {
            it = corpses.erase (it);
        } else {
            --(corpse.decay_remaining_ticks);
            ++it;
        }
    }
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
std::optional<uint32_t> MatchState::findClosestTarget (const Unit& unit)
{
    std::optional<uint32_t> closest_target = {};
    double radius = unitRadius (unit.type);
    double trigger_range = unitPrimaryAttackDescription (unit.type).trigger_range;
    double minimal_range = 1000000000.0;
    for (std::map<uint32_t, Unit>::iterator target_it = units.begin (); target_it != units.end (); ++target_it) {
        Unit& target_unit = target_it->second;
        if (target_unit.team != unit.team &&
            unitDistance (unit, target_unit) <= qMin (radius + unitRadius (target_unit.type) + trigger_range, minimal_range)) {
            minimal_range = unitDistance (unit, target_unit);
            closest_target = target_it->first;
        }
    }
    return closest_target;
}
std::vector<std::pair<uint32_t, const Unit*>> MatchState::buildOrderedSelection ()
{
    static const Unit::Type unit_order[] = {
        Unit::Type::Contaminator,
        Unit::Type::Crusader,
        Unit::Type::Goon,
        Unit::Type::Seal,
        Unit::Type::Beetle,
    };

    std::vector<std::pair<uint32_t, const Unit*>> selection;

    // TODO: Use radix algorithm
    for (const Unit::Type unit_type: unit_order)
        for (std::map<uint32_t, Unit>::const_iterator it = units.cbegin (); it != units.cend (); ++it) {
            const Unit& unit = it->second;
            if (unit.selected && unit.type == unit_type)
                selection.push_back ({it->first, &unit});
        }

    return selection;
}
