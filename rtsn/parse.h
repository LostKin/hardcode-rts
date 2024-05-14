#pragma once

#include "requests.pb.h"
#include "responses.pb.h"
#include "matchstate.h"

#include <vector>
#include <optional>


namespace RTSN::Parse {

bool matchState (const RTS::MatchStateResponse& response,
                 std::vector<std::pair<uint32_t, Unit>>& units, std::vector<std::pair<uint32_t, Corpse>>& corpses, std::vector<std::pair<uint32_t, Missile>>& missiles,
                 std::string& error_message);

}
