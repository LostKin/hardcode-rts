#pragma once

#include <QtMath>

struct Missile {
    enum class Type {
        Rocket,
        Pestilence,
    };

    Missile (Missile::Type type, Unit::Team sender_team, const QPointF& position, uint32_t target_unit, const QPointF& target_position)
        : type (type)
        , sender_team (sender_team)
        , position (position)
        , target_unit (target_unit)
        , target_position (target_position)
    {
        QPointF direction = target_position - position;
        orientation = qAtan2 (direction.y (), direction.x ());
    }
    Missile (Missile::Type type, Unit::Team sender_team, const QPointF& position, const QPointF& target_position)
        : type (type)
        , sender_team (sender_team)
        , position (position)
        , target_position (target_position)
    {
        QPointF direction = target_position - position;
        orientation = qAtan2 (direction.y (), direction.x ());
    }

    Missile::Type type;
    Unit::Team sender_team;
    QPointF position;
    std::optional<uint32_t> target_unit = {};
    QPointF target_position;
    double orientation;
};

struct Explosion {
    enum class Type {
        Fire,
        Pestilence,
    };

    Explosion (Type type, const QPointF& position, int64_t duration)
        : type (type)
        , position (position)
        , remaining_ticks (duration)
    {
    }

    Type type;
    QPointF position;
    int64_t remaining_ticks = 0;
};
