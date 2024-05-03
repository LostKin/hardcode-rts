#pragma once

#include <QPointF>
#include <variant>

#include "actions.h"

class Unit
{
public:
    enum class Type {
        Beetle = 0,
        Seal = 1,
        Crusader = 2,
        Goon = 3,
        Contaminator = 4,
        Count = 5,
    };

    enum class Team {
        Spectator,
        Red,
        Blue,
    };

public:
    Unit (Type type, quint64 phase_offset, Team team, const QPointF& position, qreal orientation)
        : type (type)
        , phase_offset (phase_offset)
        , team (team)
        , position (position)
        , orientation (orientation)
    {
    }
    Unit ()
    {
        type = Type::Seal;
        phase_offset = 0;
        team = Team::Red;
        position = QPointF (0, 0);
        orientation = 0;
    } // TODO: Remove after switch to emplace ()

public:
    Type type;
    quint64 phase_offset;
    Team team;
    QPointF position;
    qreal orientation = 0.0;
    bool selected = false;
    std::variant<StopAction, AttackAction, MoveAction, CastAction> action;
    qint64 hp = 0;
    qint64 attack_remaining_ticks = 0;
    qint64 cast_cooldown_left_ticks = 0;
    quint64 groups = 0;
};
