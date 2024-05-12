#pragma once

#include "requests.pb.h"
#include "responses.pb.h"
#include "matchstate.h"

#include <QPair>
#include <vector>
#include <optional>


namespace RTSN {
    namespace Parse {
        bool matchState (const RTS::MatchStateResponse& response,
                         std::vector<QPair<quint32, Unit>>& units, std::vector<QPair<quint32, Corpse>>& corpses, std::vector<QPair<quint32, Missile>>& missiles,
                         QString& error_message);
    }
}
