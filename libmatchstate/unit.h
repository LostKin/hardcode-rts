#pragma once

#include "actions.h"
#include "position.h"

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
    Unit (Type type, uint64_t phase_offset, Team team, const Position& position, double orientation)
        : type (type)
        , phase_offset (phase_offset)
        , team (team)
        , position (position)
        , orientation (orientation)
    {
    }

public:
    Type type;
    uint64_t phase_offset;
    Team team;
    Position position;
    double orientation = 0.0;
    bool selected = false;
    UnitActionVariant action = StopAction ();
    int64_t hp = 0;
    int64_t attack_cooldown_left_ticks = 0;
    int64_t cast_cooldown_left_ticks = 0;
    int64_t pestilence_disease_left_ticks = 0;
    uint64_t groups = 0;
    std::optional<int64_t> ttl_ticks = std::nullopt;
};
