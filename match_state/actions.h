#pragma once

#include <variant>
#include <QPointF>


struct MoveAction
{
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

struct AttackAction
{
    AttackAction (const QPointF& target_position)
        : target (target_position)
    {
    }
    AttackAction (quint32 target_unit_id)
        : target (target_unit_id)
    {
    }

    std::variant<QPointF, quint32> target;
};
