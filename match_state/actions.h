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
        : type (type)
        , target (target_position)
    {
    }

    Type type;
    QPointF target;
};
