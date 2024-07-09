#pragma once

#include "matchstate.h"

class SingleModeLoader
{
public:
    static void load (std::vector<std::pair<quint32, Unit>>& units, std::vector<std::pair<quint32, Corpse>>& corpses, std::vector<std::pair<quint32, Missile>>& missiles);
};
