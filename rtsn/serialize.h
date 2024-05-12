#pragma once

#include ".proto_stubs/responses.pb.h"
#include "matchstate.h"

#include <QMap>


namespace RTSN {
namespace Serialize {
void matchState (const MatchState* match_state, RTS::Response& response_oneof,
                 const QMap<uint32_t, uint32_t>& red_unit_id_client_to_server_map,
                 const QMap<uint32_t, uint32_t>& blue_unit_id_client_to_server_map);
}
}
