#pragma once

#include <QString>
#include <stdint.h>


struct RoomEntry
{
    RoomEntry (uint32_t id, const QString& name);

    uint32_t id;
    QString name;
};
