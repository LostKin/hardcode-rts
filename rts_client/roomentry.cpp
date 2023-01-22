#include "roomentry.h"


RoomEntry::RoomEntry ()
    : id (0)
{
}
RoomEntry::RoomEntry (uint32_t id, const QString& name)
    : id (id), name (name)
{
}
