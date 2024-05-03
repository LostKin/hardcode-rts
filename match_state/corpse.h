#pragma once

#include "unit.h"

class Corpse
{
public:
    Corpse (const Unit& unit, qint64 decay_remaining_ticks = DECAY_DURATION_TICKS)
        : unit (unit)
        , decay_remaining_ticks (decay_remaining_ticks)
    {
    }
    Corpse () // TODO: Remove after switch to emplace ()
        : decay_remaining_ticks (DECAY_DURATION_TICKS)
    {
    }

public:
    static const qint64 DECAY_DURATION_TICKS = 150;

    Unit unit;
    qint64 decay_remaining_ticks = 0;
};
