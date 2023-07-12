#include "roomentry.h"

RoomEntry::RoomEntry ()
    : id (0)
{
}
RoomEntry::RoomEntry (uint32_t id, const QString& name, uint32_t client_count, uint32_t player_count, uint32_t ready_player_count, uint32_t spectator_count)
    : id (id)
    , name (name)
    , client_count (client_count)
    , player_count (player_count)
    , ready_player_count (ready_player_count)
    , spectator_count (spectator_count)
{
}
