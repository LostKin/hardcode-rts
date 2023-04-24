#pragma once

#include <QPointF>
#include <variant>

#include "actions.h"


class Unit
{
public:
    enum class Type {
        Seal,
        Crusader,
        Goon,
        Beetle,
        Contaminator,
    };

    enum class Team {
        Spectator,
        Red,
        Blue,
    };

public:
    Unit (Type type, quint64 phase_offset, Team team, const QPointF& position, qreal direction);
    Unit (); // TODO: Remove after switch to emplace ()

public:
    Type type;
    quint64 phase_offset;
    Team team;
    QPointF position;
    qreal orientation = 0.0;
    bool selected = false;
    std::variant<std::monostate, AttackAction, MoveAction, CastAction> action;
    qint64 hp = 0;
    qint64 attack_remaining_ticks = 0;
    qint64 cast_cooldown_left_ticks = 0;
    quint64 groups = 0;
};
