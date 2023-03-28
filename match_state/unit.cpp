#include "unit.h"


Unit::Unit (Type type, quint64 phase_offset, Team team, const QPointF& position, qreal orientation)
    : type (type), phase_offset (phase_offset), team (team), position (position), orientation (orientation)
{
}
Unit::Unit() // TODO: Remove after switch to emplace ()
{
    type = Type::Seal;
    phase_offset = 0;
    team = Team::Red;
    position = QPointF(0, 0);
    orientation = 0;
}
Unit::~Unit ()
{
}
