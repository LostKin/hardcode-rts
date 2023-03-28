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
    };

    enum class Team {
        Spectator,
        Red,
        Blue,
    };

    enum class Action {
        Stop = 0,
        Move,
        Attack,
        Auto,
    };

public:
    Unit (Type type, quint64 phase_offset, Team team, const QPointF& position, qreal direction);
    Unit (); // TODO: Remove after switch to emplace ()
    ~Unit ();

public:
    Type type;
    quint64 phase_offset;
    Team team;
    QPointF position;
    qreal orientation = 0.0;
    bool selected = false;
    std::variant<std::monostate, AttackAction, MoveAction> action;
    int hp = 0;
    int attack_remaining_ticks = 0;
};
