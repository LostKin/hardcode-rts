#include "serialize.h"


static void fillStopAction (const StopAction& stop_action, RTS::StopAction* m_stop_action)
{
    if (stop_action.current_target.has_value ())
        m_stop_action->mutable_target ()->set_id (stop_action.current_target.value ());
}
static void fillMoveAction (const MoveAction& move_action, RTS::MoveAction* m_move_action)
{
    if (std::holds_alternative<Position> (move_action.target)) {
        const Position& position = std::get<Position> (move_action.target);
        m_move_action->mutable_position ()->mutable_position ()->set_x (position.x ());
        m_move_action->mutable_position ()->mutable_position ()->set_y (position.y ());
    } else {
        uint32_t id = std::get<uint32_t> (move_action.target);
        m_move_action->mutable_unit ()->set_id (id);
    }
}
static void fillAttackAction (const AttackAction& attack_action, RTS::AttackAction* m_attack_action)
{
    if (std::holds_alternative<Position> (attack_action.target)) {
        const Position& position = std::get<Position> (attack_action.target);
        m_attack_action->mutable_position ()->mutable_position ()->set_x (position.x ());
        m_attack_action->mutable_position ()->mutable_position ()->set_y (position.y ());
    } else {
        uint32_t id = std::get<uint32_t> (attack_action.target);
        m_attack_action->mutable_unit ()->set_id (id);
    }
}
static bool fillCastAction (const CastAction& cast_action, RTS::CastAction* m_cast_action)
{
    RTS::CastType type;
    switch (cast_action.type) {
    case CastAction::Type::Pestilence:
        type = RTS::CastType::CAST_TYPE_PESTILENCE;
        break;
    case CastAction::Type::SpawnBeetle:
        type = RTS::CastType::CAST_TYPE_SPAWN_BEETLE;
        break;
    default:
        return false;
    }
    const Position& position = cast_action.target;
    m_cast_action->mutable_position ()->mutable_position ()->set_x (position.x ());
    m_cast_action->mutable_position ()->mutable_position ()->set_y (position.y ());
    m_cast_action->set_type (type);
    return true;
}
static bool fillPerformingAttackAction (const PerformingAttackAction& performing_attack_action, RTS::PerformingAttackAction* m_performing_attack_action)
{
    if (std::holds_alternative<StopAction> (performing_attack_action.next_action)) {
        fillStopAction (std::get<StopAction> (performing_attack_action.next_action), m_performing_attack_action->mutable_stop ());
    } else if (std::holds_alternative<MoveAction> (performing_attack_action.next_action)) {
        fillMoveAction (std::get<MoveAction> (performing_attack_action.next_action), m_performing_attack_action->mutable_move ());
    } else if (std::holds_alternative<AttackAction> (performing_attack_action.next_action)) {
        fillAttackAction (std::get<AttackAction> (performing_attack_action.next_action), m_performing_attack_action->mutable_attack ());
    } else if (std::holds_alternative<CastAction> (performing_attack_action.next_action)) {
        if (!fillCastAction (std::get<CastAction> (performing_attack_action.next_action), m_performing_attack_action->mutable_cast ()))
            return false;
    } else {
        return false;
    }
    m_performing_attack_action->set_remaining_ticks (performing_attack_action.remaining_ticks);
    return true;
}
static bool fillPerformingCastAction (const PerformingCastAction& performing_cast_action, RTS::PerformingCastAction* m_performing_cast_action)
{
    if (std::holds_alternative<StopAction> (performing_cast_action.next_action)) {
        fillStopAction (std::get<StopAction> (performing_cast_action.next_action), m_performing_cast_action->mutable_stop ());
    } else if (std::holds_alternative<MoveAction> (performing_cast_action.next_action)) {
        fillMoveAction (std::get<MoveAction> (performing_cast_action.next_action), m_performing_cast_action->mutable_move ());
    } else if (std::holds_alternative<AttackAction> (performing_cast_action.next_action)) {
        fillAttackAction (std::get<AttackAction> (performing_cast_action.next_action), m_performing_cast_action->mutable_attack ());
    } else if (std::holds_alternative<CastAction> (performing_cast_action.next_action)) {
        if (!fillCastAction (std::get<CastAction> (performing_cast_action.next_action), m_performing_cast_action->mutable_cast ()))
            return false;
    } else {
        return false;
    }
    RTS::CastType cast_type;
    switch (performing_cast_action.cast_type) {
    case CastAction::Type::Pestilence:
        cast_type = RTS::CastType::CAST_TYPE_PESTILENCE;
        break;
    case CastAction::Type::SpawnBeetle:
        cast_type = RTS::CastType::CAST_TYPE_SPAWN_BEETLE;
        break;
    default:
        return false;
    }
    m_performing_cast_action->set_cast_type (cast_type);
    m_performing_cast_action->set_remaining_ticks (performing_cast_action.remaining_ticks);
    return true;
}
static bool fillUnit (uint32_t id, const Unit& unit, RTS::Unit& m_unit,
                      const std::map<uint32_t, uint32_t>& red_unit_id_client_to_server_map,
                      const std::map<uint32_t, uint32_t>& blue_unit_id_client_to_server_map)
{
    RTS::Team m_team;
    const std::map<uint32_t, uint32_t>* unit_id_client_to_server_map;
    switch (unit.team) {
    case Unit::Team::Red: {
        m_team = RTS::Team::TEAM_RED;
        unit_id_client_to_server_map = &red_unit_id_client_to_server_map;
    } break;
    case Unit::Team::Blue: {
        m_team = RTS::Team::TEAM_BLUE;
        unit_id_client_to_server_map = &blue_unit_id_client_to_server_map;
    } break;
    default: {
    } return false;
    }
    m_unit.set_team (m_team);
    {
        std::map<uint32_t, uint32_t>::const_iterator it = unit_id_client_to_server_map->find (id);
        if (it != unit_id_client_to_server_map->cend ())
            m_unit.mutable_client_id ()->set_id (it->first);
    }
    switch (unit.type) {
    case Unit::Type::Seal:
        m_unit.set_type (RTS::UnitType::UNIT_TYPE_SEAL);
        break;
    case Unit::Type::Crusader:
        m_unit.set_type (RTS::UnitType::UNIT_TYPE_CRUSADER);
        break;
    case Unit::Type::Goon:
        m_unit.set_type (RTS::UnitType::UNIT_TYPE_GOON);
        break;
    case Unit::Type::Beetle:
        m_unit.set_type (RTS::UnitType::UNIT_TYPE_BEETLE);
        break;
    case Unit::Type::Contaminator:
        m_unit.set_type (RTS::UnitType::UNIT_TYPE_CONTAMINATOR);
        break;
    default:
        return false;
    }

    if (std::holds_alternative<StopAction> (unit.action)) {
        fillStopAction (std::get<StopAction> (unit.action), m_unit.mutable_current_action ()->mutable_stop ());
    } else if (std::holds_alternative<MoveAction> (unit.action)) {
        fillMoveAction (std::get<MoveAction> (unit.action), m_unit.mutable_current_action ()->mutable_move ());
    } else if (std::holds_alternative<AttackAction> (unit.action)) {
        fillAttackAction (std::get<AttackAction> (unit.action), m_unit.mutable_current_action ()->mutable_attack ());
    } else if (std::holds_alternative<CastAction> (unit.action)) {
        if (!fillCastAction (std::get<CastAction> (unit.action), m_unit.mutable_current_action ()->mutable_cast ()))
            return false;
    } else if (std::holds_alternative<PerformingAttackAction> (unit.action)) {
        if (!fillPerformingAttackAction (std::get<PerformingAttackAction> (unit.action), m_unit.mutable_current_action ()->mutable_performing_attack ()))
            return false;
    } else if (std::holds_alternative<PerformingCastAction> (unit.action)) {
        if (!fillPerformingCastAction (std::get<PerformingCastAction> (unit.action), m_unit.mutable_current_action ()->mutable_performing_cast ()))
            return false;
    } else {
        return false;
    }
    m_unit.set_attack_cooldown_left_ticks (unit.attack_cooldown_left_ticks);
    m_unit.set_cast_cooldown_left_ticks (unit.cast_cooldown_left_ticks);

    m_unit.mutable_position ()->set_x (unit.position.x ());
    m_unit.mutable_position ()->set_y (unit.position.y ());
    m_unit.set_health (unit.hp);
    m_unit.set_orientation (unit.orientation);
    m_unit.set_id (id);
    if (unit.ttl_ticks.has_value ())
        m_unit.mutable_ttl ()->set_ttl_ticks (unit.ttl_ticks.value ());

    return true;
}
static bool fillCorpse (uint32_t id, const Corpse& corpse, RTS::Corpse& m_corpse,
                        const std::map<uint32_t, uint32_t>& red_unit_id_client_to_server_map,
                        const std::map<uint32_t, uint32_t>& blue_unit_id_client_to_server_map)
{
    m_corpse.set_decay_remaining_ticks (corpse.decay_remaining_ticks);
    return fillUnit (id, corpse.unit, *m_corpse.mutable_unit (), red_unit_id_client_to_server_map, blue_unit_id_client_to_server_map);
}
static bool fillMissile (uint32_t id, const Missile& missile, RTS::Missile& m_missile)
{
    switch (missile.type) {
    case Missile::Type::Pestilence:
        m_missile.set_type (RTS::MissileType::MISSILE_PESTILENCE);
        break;
    case Missile::Type::Rocket:
        m_missile.set_type (RTS::MissileType::MISSILE_ROCKET);
        break;
    default:
        return false;
    }

    if (missile.target_unit.has_value ())
        m_missile.mutable_target_unit ()->set_id (*(missile.target_unit));

    m_missile.mutable_target_position ()->set_x (missile.target_position.x ());
    m_missile.mutable_target_position ()->set_y (missile.target_position.y ());

    m_missile.mutable_position ()->set_x (missile.position.x ());
    m_missile.mutable_position ()->set_y (missile.position.y ());

    m_missile.set_id (id);

    switch (missile.sender_team) {
    case Unit::Team::Red:
        m_missile.set_team (RTS::Team::TEAM_RED);
        break;
    case Unit::Team::Blue:
        m_missile.set_team (RTS::Team::TEAM_BLUE);
        break;
    default:
        return false;
    }

    return true;
}

namespace RTSN::Serialize {

void matchState (const MatchState* match_state, RTS::Response& response_oneof,
                 const std::map<uint32_t, uint32_t>& red_unit_id_client_to_server_map,
                 const std::map<uint32_t, uint32_t>& blue_unit_id_client_to_server_map)
{
    RTS::MatchStateResponse* response = response_oneof.mutable_match_state ();

    response->set_tick (match_state->getTickNo ());

    google::protobuf::RepeatedPtrField<RTS::Unit>* m_units = response->mutable_units ();
    const std::map<uint32_t, Unit>& units = match_state->unitsRef ();
    for (std::map<uint32_t, Unit>::const_iterator it = units.cbegin (); it != units.cend (); it++) {
        uint32_t id = it->first;
        const Unit& unit = it->second;

        if (!fillUnit (id, unit, *m_units->Add (), red_unit_id_client_to_server_map, blue_unit_id_client_to_server_map))
            m_units->RemoveLast ();
    }

    google::protobuf::RepeatedPtrField<RTS::Corpse>* m_corpses = response->mutable_corpses ();
    const std::map<uint32_t, Corpse>& corpses = match_state->corpsesRef ();
    for (std::map<uint32_t, Corpse>::const_iterator it = corpses.cbegin (); it != corpses.cend (); it++) {
        uint32_t id = it->first;
        const Corpse& corpse = it->second;

        if (!fillCorpse (id, corpse, *m_corpses->Add (), red_unit_id_client_to_server_map, blue_unit_id_client_to_server_map))
            m_corpses->RemoveLast ();
    }

    google::protobuf::RepeatedPtrField<RTS::Missile>* m_missiles = response->mutable_missiles ();
    const std::map<uint32_t, Missile>& missiles = match_state->missilesRef ();
    for (std::map<uint32_t, Missile>::const_iterator it = missiles.cbegin (); it != missiles.cend (); it++) {
        uint32_t id = it->first;
        const Missile& missile = it->second;

        if (!fillMissile (id, missile, *m_missiles->Add ()))
            m_missiles->RemoveLast ();
    }
}

}
