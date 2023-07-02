#pragma once

#include <QtMath>

struct Missile {
    enum class Type {
        Rocket,
        Pestilence,
    };

    Missile (Missile::Type type, Unit::Team sender_team, const QPointF& position, quint32 target_unit, const QPointF& target_position)
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
    Missile ()
    {
    }

    Missile::Type type;
    Unit::Team sender_team;
    QPointF position;
    std::optional<quint32> target_unit = {};
    QPointF target_position;
    qreal orientation;
};

struct Explosion {
    enum class Type {
        Fire,
        Pestilence,
    };

    Explosion (Type type, const QPointF& position, qint64 duration)
        : type (type)
        , position (position)
        , remaining_ticks (duration)
    {
    }

    Type type;
    QPointF position;
    qint64 remaining_ticks = 0;
};
