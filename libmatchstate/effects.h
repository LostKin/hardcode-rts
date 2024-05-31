#pragma once


struct Missile {
    enum class Type {
        Rocket,
        Pestilence,
    };

    Missile (Missile::Type type, Unit::Team sender_team, const Position& position, uint32_t target_unit, const Position& target_position)
        : type (type)
        , sender_team (sender_team)
        , position (position)
        , target_unit (target_unit)
        , target_position (target_position)
    {
        Offset direction = target_position - position;
        orientation = qAtan2 (direction.dY (), direction.dX ()); // TODO: Add method Offset::orientation ()
    }
    Missile (Missile::Type type, Unit::Team sender_team, const Position& position, const Position& target_position)
        : type (type)
        , sender_team (sender_team)
        , position (position)
        , target_position (target_position)
    {
        Offset direction = target_position - position;
        orientation = qAtan2 (direction.dY (), direction.dX ());
    }

    Missile::Type type;
    Unit::Team sender_team;
    Position position;
    std::optional<uint32_t> target_unit = {};
    Position target_position;
    double orientation;
};

struct Explosion {
    enum class Type {
        Fire,
        Pestilence,
    };

    Explosion (Type type, const Position& position, int64_t duration)
        : type (type)
        , position (position)
        , remaining_ticks (duration)
    {
    }

    Type type;
    Position position;
    int64_t remaining_ticks = 0;
};
