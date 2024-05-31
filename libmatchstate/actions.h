#pragma once

#include "position.h"

#include <variant>


struct StopAction {
    StopAction ()
    {
    }

    std::optional<uint32_t> current_target = {};
};

struct MoveAction {
    MoveAction (const Position& target_position)
        : target (target_position)
    {
    }
    MoveAction (uint32_t target_unit_id)
        : target (target_unit_id)
    {
    }

    std::variant<Position, uint32_t> target;
};

struct AttackAction {
    AttackAction (const Position& target_position)
        : target (target_position)
    {
    }
    AttackAction (uint32_t target_unit_id)
        : target (target_unit_id)
    {
    }

    std::variant<Position, uint32_t> target;
    std::optional<uint32_t> current_target = {};
};

struct CastAction {
    enum class Type {
        Unknown,
        Pestilence,
        SpawnBeetle,
    };

    CastAction (Type type, const Position& target_position)
        : type (type), target (target_position)
    {
    }

    Type type;
    Position target;
};

typedef std::variant<StopAction, MoveAction, AttackAction, CastAction> IntentiveActionVariant;

struct PerformingAttackAction {
    PerformingAttackAction (const StopAction& next_action, int64_t duration_ticks)
        : next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingAttackAction (const MoveAction& next_action, int64_t duration_ticks)
        : next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingAttackAction (const AttackAction& next_action, int64_t duration_ticks)
        : next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingAttackAction (const CastAction& next_action, int64_t duration_ticks)
        : next_action (next_action), remaining_ticks (duration_ticks)
    {
    }

    IntentiveActionVariant next_action;
    int64_t remaining_ticks = 0;
};

struct PerformingCastAction {
    PerformingCastAction (CastAction::Type cast_type, const StopAction& next_action, uint64_t duration_ticks)
        : cast_type (cast_type), next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingCastAction (CastAction::Type cast_type, const MoveAction& next_action, uint64_t duration_ticks)
        : cast_type (cast_type), next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingCastAction (CastAction::Type cast_type, const AttackAction& next_action, uint64_t duration_ticks)
        : cast_type (cast_type), next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingCastAction (CastAction::Type cast_type, const CastAction& next_action, uint64_t duration_ticks)
        : cast_type (cast_type), next_action (next_action), remaining_ticks (duration_ticks)
    {
    }

    CastAction::Type cast_type;
    IntentiveActionVariant next_action;
    int64_t remaining_ticks;
};

typedef std::variant<StopAction, MoveAction, AttackAction, CastAction, PerformingAttackAction, PerformingCastAction> UnitActionVariant;
