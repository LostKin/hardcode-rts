#include "parse.h"


static StopAction parseUnitActionStop (const RTS::StopAction& m_stop_action)
{
    StopAction stop;
    if (m_stop_action.has_target ())
        stop.current_target = m_stop_action.target ().id ();
    return stop;
}
static MoveAction parseUnitActionMove (const RTS::MoveAction& m_move_action)
{
    MoveAction move (Position (0, 0));
    if (m_move_action.target_case () == RTS::MoveAction::kPosition)
        move.target = Position (m_move_action.position ().position ().x (), m_move_action.position ().position ().y ());
    else
        move.target = (uint32_t) m_move_action.unit ().id ();
    return move;
}
static AttackAction parseUnitActionAttack (const RTS::AttackAction& m_attack_action)
{
    AttackAction attack (Position (0, 0));
    if (m_attack_action.target_case () == RTS::AttackAction::kPosition)
        attack.target = Position (m_attack_action.position ().position ().x (), m_attack_action.position ().position ().y ());
    else
        attack.target = (uint32_t) m_attack_action.unit ().id ();
    return attack;
}
static std::optional<CastAction> parseUnitActionCast (const RTS::CastAction& m_cast_action, std::string& error_message)
{
    CastAction cast (CastAction::Type::Unknown, Position (0, 0));
    cast.target = Position (m_cast_action.position ().position ().x (), m_cast_action.position ().position ().y ());
    switch (m_cast_action.type ()) {
    case RTS::CastType::CAST_TYPE_PESTILENCE:
        cast.type = CastAction::Type::Pestilence;
        break;
    case RTS::CastType::CAST_TYPE_SPAWN_BEETLE:
        cast.type = CastAction::Type::SpawnBeetle;
        break;
    default:
        error_message = "Invalid unit action cast type from server";
        return std::nullopt;
    }
    return cast;
}
static std::optional<PerformingAttackAction> parseUnitActionPerformingAttack (const RTS::PerformingAttackAction& m_peforming_attack_action, std::string& error_message)
{
    int64_t remaining_ticks = m_peforming_attack_action.remaining_ticks ();
    switch (m_peforming_attack_action.next_action_case ()) {
    case RTS::UnitAction::kStop: {
        return PerformingAttackAction (parseUnitActionStop (m_peforming_attack_action.stop ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kMove: {
        return PerformingAttackAction (parseUnitActionMove (m_peforming_attack_action.move ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kAttack: {
        return PerformingAttackAction (parseUnitActionAttack (m_peforming_attack_action.attack ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kCast: {
        std::optional<CastAction> cast = parseUnitActionCast (m_peforming_attack_action.cast (), error_message);
        if (cast == std::nullopt)
            return std::nullopt;
        return PerformingAttackAction (*cast, remaining_ticks);
    } break;
    default: {
        error_message = "Invalid unit next action type from server";
        return std::nullopt;
    }
    }
}
static std::optional<PerformingCastAction> parseUnitActionPerformingCast (const RTS::PerformingCastAction& m_peforming_cast_action, std::string& error_message)
{
    int64_t remaining_ticks = m_peforming_cast_action.remaining_ticks ();
    CastAction::Type cast_type;
    switch (m_peforming_cast_action.cast_type ()) {
    case RTS::CastType::CAST_TYPE_PESTILENCE:
        cast_type = CastAction::Type::Pestilence;
        break;
    case RTS::CastType::CAST_TYPE_SPAWN_BEETLE:
        cast_type = CastAction::Type::SpawnBeetle;
        break;
    default:
        error_message = "Invalid unit next action cast type from server";
        return std::nullopt;
    }
    switch (m_peforming_cast_action.next_action_case ()) {
    case RTS::UnitAction::kStop: {
        return PerformingCastAction (cast_type, parseUnitActionStop (m_peforming_cast_action.stop ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kMove: {
        return PerformingCastAction (cast_type, parseUnitActionMove (m_peforming_cast_action.move ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kAttack: {
        return PerformingCastAction (cast_type, parseUnitActionAttack (m_peforming_cast_action.attack ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kCast: {
        std::optional<CastAction> cast = parseUnitActionCast (m_peforming_cast_action.cast (), error_message);
        if (cast == std::nullopt)
            return std::nullopt;
        return PerformingCastAction (cast_type, *cast, remaining_ticks);
    } break;
    default: {
        error_message = "Invalid unit next action type from server 2";
        return std::nullopt;
    }
    }
}
static bool parseUnitAction (const RTS::UnitAction& m_current_action, UnitActionVariant& unit_action, std::string& error_message)
{
    switch (m_current_action.action_case ()) {
    case RTS::UnitAction::kStop: {
        unit_action = parseUnitActionStop (m_current_action.stop ());
    } break;
    case RTS::UnitAction::kMove: {
        unit_action = parseUnitActionMove (m_current_action.move ());
    } break;
    case RTS::UnitAction::kAttack: {
        unit_action = parseUnitActionAttack (m_current_action.attack ());
    } break;
    case RTS::UnitAction::kCast: {
        std::optional<CastAction> cast = parseUnitActionCast (m_current_action.cast (), error_message);
        if (cast == std::nullopt)
            return false;
        unit_action = *cast;
    } break;
    case RTS::UnitAction::kPerformingAttack: {
        std::optional<PerformingAttackAction> performing_attack = parseUnitActionPerformingAttack (m_current_action.performing_attack (), error_message);
        if (performing_attack == std::nullopt)
            return false;
        unit_action = *performing_attack;
    } break;
    case RTS::UnitAction::kPerformingCast: {
        std::optional<PerformingCastAction> performing_cast = parseUnitActionPerformingCast (m_current_action.performing_cast (), error_message);
        if (performing_cast == std::nullopt)
            return false;
        unit_action = *performing_cast;
    } break;
    default: {
        error_message = "Invalid unit action type from server";
    } return false;
    }
    return true;
}
static std::optional<std::pair<uint32_t, Unit>> parseUnit (const RTS::Unit& m_unit, std::string& error_message)
{
    Unit::Team team;
    switch (m_unit.team ()) {
    case RTS::Team::TEAM_RED:
        team = Unit::Team::Red;
        break;
    case RTS::Team::TEAM_BLUE:
        team = Unit::Team::Blue;
        break;
    default:
        error_message = "Invalid 'MatchState' from server: invalid unit team";
        return std::nullopt;
    }

    Unit::Type type;
    switch (m_unit.type ()) {
    case RTS::UnitType::UNIT_TYPE_CRUSADER:
        type = Unit::Type::Crusader;
        break;
    case RTS::UnitType::UNIT_TYPE_SEAL:
        type = Unit::Type::Seal;
        break;
    case RTS::UnitType::UNIT_TYPE_GOON:
        type = Unit::Type::Goon;
        break;
    case RTS::UnitType::UNIT_TYPE_BEETLE:
        type = Unit::Type::Beetle;
        break;
    case RTS::UnitType::UNIT_TYPE_CONTAMINATOR:
        type = Unit::Type::Contaminator;
        break;
    default:
        error_message = "Invalid 'MatchState' from server: invalid unit type";
        return std::nullopt;
    }

    if (!m_unit.has_position ()) {
        error_message = "Invalid 'MatchState' from server: missing position";
        return std::nullopt;
    }

    uint32_t id = m_unit.has_client_id () ? m_unit.client_id ().id () : m_unit.id ();
    Unit unit = Unit (type, 0, team, Position (m_unit.position ().x (), m_unit.position ().y ()), m_unit.orientation ());
    unit.hp = m_unit.health ();
    if (!parseUnitAction (m_unit.current_action (), unit.action, error_message))
        return std::nullopt;
    unit.attack_cooldown_left_ticks = m_unit.attack_cooldown_left_ticks ();
    unit.cast_cooldown_left_ticks = m_unit.cast_cooldown_left_ticks ();
    if (m_unit.has_ttl ())
        unit.ttl_ticks = m_unit.ttl ().ttl_ticks ();

    return std::pair<uint32_t, Unit> (id, unit);
}
static std::optional<std::pair<uint32_t, Corpse>> parseCorpse (const RTS::Corpse& m_corpse, std::string& error_message)
{
    if (!m_corpse.has_unit ())
        return std::nullopt;
    std::optional<std::pair<uint32_t, Unit>> parsed_unit = parseUnit (m_corpse.unit (), error_message);
    if (!parsed_unit.has_value ())
        return std::nullopt;
    Corpse corpse (parsed_unit->second, m_corpse.decay_remaining_ticks ());
    return std::pair<uint32_t, Corpse> (parsed_unit->first, corpse);
}
static std::optional<std::pair<uint32_t, Missile>> parseMissile (const RTS::Missile& m_missile, std::string& error_message)
{
    Unit::Team team;
    switch (m_missile.team ()) {
    case RTS::Team::TEAM_RED:
        team = Unit::Team::Red;
        break;
    case RTS::Team::TEAM_BLUE:
        team = Unit::Team::Blue;
        break;
    default:
        error_message = "Invalid 'MatchState' from server: invalid missile sender team";
        return std::nullopt;
    }

    Missile::Type type;
    switch (m_missile.type ()) {
    case RTS::MissileType::MISSILE_ROCKET:
        type = Missile::Type::Rocket;
        break;
    case RTS::MissileType::MISSILE_PESTILENCE:
        type = Missile::Type::Pestilence;
        break;
    default:
        error_message = "Invalid 'MatchState' from server: invalid missile type";
        return std::nullopt;
    }

    if (!m_missile.has_position ()) {
        error_message = "Invalid 'MatchState' from server: missing missile position";
        return std::nullopt;
    }

    uint32_t id = m_missile.id ();
    Missile missile (type, team, Position (m_missile.position ().x (), m_missile.position ().y ()), 0, Position (m_missile.target_position ().x (), m_missile.target_position ().y ()));

    if (m_missile.has_target_unit ())
        missile.target_unit = m_missile.target_unit ().id ();
    else
        missile.target_unit.reset ();

    return std::pair<uint32_t, Missile> (id, missile);
}

namespace RTSN::Parse {

bool matchState (const RTS::MatchStateResponse& response,
                 std::vector<std::pair<uint32_t, Unit>>& units, std::vector<std::pair<uint32_t, Corpse>>& corpses, std::vector<std::pair<uint32_t, Missile>>& missiles,
                 std::string& error_message)
{
    for (int i = 0; i < response.units_size (); i++) {
        std::optional<std::pair<uint32_t, Unit>> id_unit = parseUnit (response.units (i), error_message);
        if (id_unit == std::nullopt)
            return false;
        units.emplace_back (id_unit->first, id_unit->second);
    }
    for (int i = 0; i < response.corpses_size (); i++) {
        std::optional<std::pair<uint32_t, Corpse>> id_corpse = parseCorpse (response.corpses (i), error_message);
        if (id_corpse == std::nullopt)
            return false;
        corpses.emplace_back (id_corpse->first, id_corpse->second);
    }
    for (int i = 0; i < response.missiles_size (); i++) {
        std::optional<std::pair<uint32_t, Missile>> id_missile = parseMissile (response.missiles (i), error_message);
        if (id_missile == std::nullopt)
            return false;
        missiles.emplace_back (id_missile->first, id_missile->second);
    }
    return true;
}

}
