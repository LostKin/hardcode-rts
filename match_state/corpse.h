#pragma once

#include "unit.h"

class Corpse
{
public:
    Corpse (const Unit& unit, int64_t decay_remaining_ticks = DECAY_DURATION_TICKS)
        : unit (unit)
        , decay_remaining_ticks (decay_remaining_ticks)
    {
    }

public:
    static const int64_t DECAY_DURATION_TICKS = 150;

    Unit unit;
    int64_t decay_remaining_ticks = 0;
};
