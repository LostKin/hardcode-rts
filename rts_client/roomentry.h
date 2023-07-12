#pragma once

#include <QString>
#include <stdint.h>

struct RoomEntry {
    RoomEntry ();
    RoomEntry (uint32_t id, const QString& name, uint32_t client_count, uint32_t player_count, uint32_t ready_player_count, uint32_t spectator_count);

    uint32_t id;
    QString name;
    uint32_t client_count;
    uint32_t player_count;
    uint32_t ready_player_count;
    uint32_t spectator_count;
};
