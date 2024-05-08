#pragma once

#include <variant>
#include <QPointF>


struct StopAction {
    StopAction ()
    {
    }

    std::optional<quint32> current_target = {};
};

struct MoveAction {
    MoveAction (const QPointF& target_position)
        : target (target_position)
    {
    }
    MoveAction (quint32 target_unit_id)
        : target (target_unit_id)
    {
    }

    std::variant<QPointF, quint32> target;
};

struct AttackAction {
    AttackAction (const QPointF& target_position)
        : target (target_position)
    {
    }
    AttackAction (quint32 target_unit_id)
        : target (target_unit_id)
    {
    }

    std::variant<QPointF, quint32> target;
    std::optional<quint32> current_target = {};
};

struct CastAction {
    enum class Type {
        Unknown,
        Pestilence,
        SpawnBeetle,
    };

    CastAction (Type type, const QPointF& target_position)
        : type (type), target (target_position)
    {
    }

    Type type;
    QPointF target;
};

typedef std::variant<StopAction, MoveAction, AttackAction, CastAction> IntentiveActionVariant;

struct PerformingAttackAction {
    PerformingAttackAction (const StopAction& next_action, qint64 duration_ticks)
        : next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingAttackAction (const MoveAction& next_action, qint64 duration_ticks)
        : next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingAttackAction (const AttackAction& next_action, qint64 duration_ticks)
        : next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingAttackAction (const CastAction& next_action, qint64 duration_ticks)
        : next_action (next_action), remaining_ticks (duration_ticks)
    {
    }

    IntentiveActionVariant next_action;
    qint64 remaining_ticks = 0;
};

struct PerformingCastAction {
    PerformingCastAction (CastAction::Type cast_type, const StopAction& next_action, quint64 duration_ticks)
        : cast_type (cast_type), next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingCastAction (CastAction::Type cast_type, const MoveAction& next_action, quint64 duration_ticks)
        : cast_type (cast_type), next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingCastAction (CastAction::Type cast_type, const AttackAction& next_action, quint64 duration_ticks)
        : cast_type (cast_type), next_action (next_action), remaining_ticks (duration_ticks)
    {
    }
    PerformingCastAction (CastAction::Type cast_type, const CastAction& next_action, quint64 duration_ticks)
        : cast_type (cast_type), next_action (next_action), remaining_ticks (duration_ticks)
    {
    }

    CastAction::Type cast_type;
    IntentiveActionVariant next_action;
    qint64 remaining_ticks;
};

typedef std::variant<StopAction, MoveAction, AttackAction, CastAction, PerformingAttackAction, PerformingCastAction> UnitActionVariant;
