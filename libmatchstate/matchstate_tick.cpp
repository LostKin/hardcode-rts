#include "matchstate.h"

#include "hc_parser.h"


static bool pointInsideCircle (const Position& point, const Position& center, double radius)
{
    return (point - center).length () <= radius;
}
static inline double unitDistance (const Unit& a, const Unit& b)
{
    return (a.position - b.position).length ();
}
static bool orientationsFuzzyMatch (double a, double b)
{
    return qAbs (remainder (a - b, M_PI * 2.0)) <= (M_PI / 180.0); // Within 1 degree
}
static std::vector<std::string> split (const std::string& s, char seperator)
{
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;

    while ((pos = s.find (seperator, pos)) != std::string::npos) {
        output.push_back (s.substr (prev_pos, pos-prev_pos));
        prev_pos = ++pos;
    }

    output.push_back (s.substr (prev_pos, pos-prev_pos));

    return output;
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

void MatchState::initNodeTrees ()
{
    node (blue_team_node_tree, "unit");
    node (blue_team_node_tree, "unit.teams");
    node (blue_team_node_tree, "unit.teams.red");
    node (blue_team_node_tree, "unit.teams.blue");
    node (blue_team_node_tree, "unit.is_friend");

    node (blue_team_node_tree, "unit.is_friend").function = DynamicallyTypedFunction ({
            .parameters = "team",
            .source = "team == unit.teams.blue ()",
        });
}
Node& MatchState::node (Node& node_tree, const std::string& name) const
{
    std::vector<std::string> subnames = split (name, '.');
    Node& node = node_tree;
    for (const std::string& subname: subnames)
        node = node.children[subname];
    return node;
}
void MatchState::call (Node& node_tree, const std::string& name, BlueTeamUserData& user_data)
{
    const std::optional<DynamicallyTypedFunction>& optional_function = node (node_tree, name).function;
    if (!optional_function.has_value ())
        return;
    const DynamicallyTypedFunction& function = optional_function.value ();
    const std::string& parameters = function.parameters;
    const std::string& source = function.source;
    // TODO
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
void MatchState::applyDeath ()
{
    for (std::map<uint32_t, Unit>::iterator it = units.begin (); it != units.end ();) {
        Unit& unit = it->second;
        if (unit.hp <= 0) {
            corpses.emplace (it->first, unit);
            it = units.erase (it);
        } else if (unit.ttl_ticks.has_value ()) {
            if (unit.ttl_ticks.value () <= 1) {
                corpses.emplace (it->first, unit);
                it = units.erase (it);
            } else {
                --unit.ttl_ticks.value ();
                ++it;
            }
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
            --corpse.decay_remaining_ticks;
            ++it;
        }
    }
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
void MatchState::dealDamage (Unit& unit, int64_t damage)
{
    unit.hp = std::max<int64_t> (unit.hp - damage, 0);
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
void MatchState::redTeamUserTick (RedTeamUserData& /* user_data */)
{
}
void MatchState::blueTeamUserTick (BlueTeamUserData& user_data)
{
    Node node_tree;

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
