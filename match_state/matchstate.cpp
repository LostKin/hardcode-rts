#include "matchstate.h"

#include <QtMath>
#include <QDebug>
#include <QSet>


static constexpr qreal SQRT_1_2 = 0.70710678118655;


static inline qreal unitSquareDistance (const Unit& a, const Unit& b)
{
    QPointF delta = a.position - b.position;
    return delta.x ()*delta.x () + delta.y ()*delta.y ();
}
static inline qreal unitDistance (const Unit& a, const Unit& b)
{
    QPointF delta = a.position - b.position;
    return std::hypot (delta.x (), delta.y ());
}


MatchState::MatchState ()
{
}
MatchState::~MatchState ()
{
}

quint64 MatchState::clockNS () const
{
    return clock_ns;
}
const QRectF& MatchState::areaRef () const
{
    return area;
}
const QHash<quint32, Unit>& MatchState::unitsRef () const
{
    return units;
}
QHash<quint32, Unit>::iterator  MatchState::createUnit (Unit::Type type, Unit::Team team, const QPointF& position, qreal direction)
{
    QHash<quint32, Unit>::iterator unit = units.insert (next_id++, {type, quint32 (random_generator ()), team, position, direction});
    unit->hp = unitMaxHP (unit->type);
    return unit;
}

Unit& MatchState::addUnit(quint32 id, Unit::Type type, Unit::Team team, const QPointF& position, qreal direction) {
    Unit& unit = *units.insert (id, {type, quint32 (random_generator ()), team, position, direction});
    unit.hp = unitMaxHP (unit.type);
    return unit;
}

void MatchState::trySelect (Unit::Team team, const QPointF& point, bool add)
{
    if (add) {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            if (unit.team == team && checkUnitSelection (unit, point))
                unit.selected = true;
        }
    } else {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            if (unit.team == team && checkUnitSelection (unit, point)) {
                clearSelection ();
                unit.selected = true;
                break;
            }
        }
    }
}
void MatchState::trySelect (Unit::Team team, const QRectF& rect, bool add)
{
    if (add) {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            if (unit.team == team && checkUnitSelection (unit, rect))
                unit.selected = true;
        }
    } else {
        bool selection_found = false;
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            if (unit.team == team && checkUnitSelection (unit, rect)) {
                selection_found = true;
                break;
            }
        }
        if (selection_found) {
            clearSelection ();
            for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
                Unit& unit = it.value ();
                if (unit.team == team && checkUnitSelection (unit, rect))
                    unit.selected = true;
            }
        }
    }
}
std::optional<QPointF> MatchState::selectionCenter () const
{
    size_t selection_count = 0;
    QPointF sum = {};
    for (QHash<quint32, Unit>::const_iterator it = units.cbegin (); it != units.cend (); ++it) {
        const Unit& unit = it.value ();
        if (unit.selected) {
            ++selection_count;
            sum += unit.position;
        }
    }
    return selection_count ? std::optional<QPointF> (QPointF (sum/selection_count)) : std::optional<QPointF> ();
}
void MatchState::attackEnemy (Unit::Team attacker_team, const QPointF& point)
{
    std::optional<quint32> target;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.team != attacker_team && checkUnitSelection (unit, point)) {
            target = it.key ();
            break;
        }
    }
    applyAction (target.has_value () ? AttackAction (*target) : AttackAction (point));
}
void MatchState::move (const QPointF& point)
{
    std::optional<quint32> target;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (checkUnitSelection (unit, point)) {
            target = it.key ();
            break;
        }
    }
    applyAction (target.has_value () ? MoveAction (*target) : MoveAction (point));
}
void MatchState::stop ()
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected)
            unit.action = std::monostate ();
    }
}
void MatchState::autoAction (Unit::Team attacker_team, const QPointF& point)
{
    std::optional<quint32> target;
    bool target_is_enemy = false;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (checkUnitSelection (unit, point)) {
            target = it.key ();
            target_is_enemy = it->team != attacker_team;
            break;
        }
    }
    if (target.has_value ()) {
        if (target_is_enemy) {
            applyAction (AttackAction (*target));
        } else {
            applyAction (MoveAction (*target));
        }
    } else {
        applyAction (MoveAction (point));
    }
}
void MatchState::tick ()
{
    constexpr uint64_t dt_nsec = 20000000;
    constexpr qreal dt = 0.020;

    clock_ns += dt_nsec;

    // TODO: Implement current and new positions

    // Apply action
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        QPointF& position = unit.position;
        if (unit.attack_remaining_ticks > 0) {
            --unit.attack_remaining_ticks;
        } else if (std::holds_alternative<MoveAction> (unit.action)) {
            const std::variant<QPointF, quint32>& target = std::get<MoveAction> (unit.action).target;
            if (std::holds_alternative<QPointF> (target)) {
                const QPointF& target_position = std::get<QPointF> (target);
                qreal max_velocity = unitMaxVelocity (unit.type);
                QPointF off = target_position - position;
                rotateUnit (unit, dt, qAtan2 (off.y (), off.x ()));
                qreal path_length = max_velocity*dt;
                qreal square_length = QPointF::dotProduct (off, off);
                if (square_length <= path_length*path_length) {
                    position = target_position;
                } else {
                    position += off*(path_length/qSqrt (square_length));
                }
            }
        } else if (std::holds_alternative<AttackAction> (unit.action)) {
            const std::variant<QPointF, quint32>& target = std::get<AttackAction> (unit.action).target;
            const AttackDescription& attack_description = unitPrimaryAttackDescription (unit.type);
            if (std::holds_alternative<quint32> (target)) {
                quint32 target_unit_id = std::get<quint32> (target);
                QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
                if (target_unit_it != units.end ()) {
                    Unit& target_unit = target_unit_it.value ();
                    QPointF direction = target_unit.position - position;
                    qreal target_orientation = qAtan2 (direction.y (), direction.x ());
                    bool in_range = false;
                    qreal max_velocity = unitMaxVelocity (unit.type);
                    QPointF off = target_unit.position - position;
                    rotateUnit (unit, dt, qAtan2 (off.y (), off.x ()));
                    qreal length = qSqrt (QPointF::dotProduct (off, off));
                    qreal path_length = max_velocity*dt;
                    qreal full_attack_range = attack_description.range + unitRadius (unit.type) + unitRadius (target_unit.type);
                    if (length > full_attack_range) {
                        if (path_length > length - full_attack_range)
                            path_length = length - full_attack_range;
                        position += off*(path_length/length);
                        in_range = unitDistance (unit, target_unit) <= full_attack_range;
                    } else {
                        in_range = true;
                    }
                    if (in_range && qAbs (unit.orientation - target_orientation) <= (M_PI/180.0)) {
                        target_unit.hp = std::max (target_unit.hp - attack_description.damage, 0);
                        unit.attack_remaining_ticks = attack_description.duration_ticks;
                        emit soundEventEmitted (SoundEvent::Shot);
                    }
                } else {
                    unit.action = std::monostate ();
                }
            }
        }
    }

    // Apply area boundary collisions
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        QPointF& position = unit.position;
        qreal unit_radius = unitRadius (unit.type);
        qreal max_velocity = unitMaxVelocity (unit.type)*2.0;
        QPointF off;
        if (position.x () < (area.left () + unit_radius)) {
            off.setX ((area.left () + unit_radius) - position.x ());
        } else if (position.x () > (area.right () - unit_radius)) {
            off.setX ((area.right () - unit_radius) - position.x ());
        }
        if (position.y () < (area.top () + unit_radius)) {
            off.setY ((area.top () + unit_radius) - position.y ());
        } else if (position.y () > (area.bottom () - unit_radius)) {
            off.setY ((area.bottom () - unit_radius) - position.y ());
        }
        qreal path_length = max_velocity*dt;
        qreal square_length = QPointF::dotProduct (off, off);
        if (square_length <= path_length*path_length) {
            position += off;
        } else {
            position += off*(path_length/qSqrt (square_length));
        }
    }

    // Apply unit collisions: O (N^2)
    QVector<QPointF> offsets;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        QPointF& position = unit.position;
        qreal unit_radius = unitRadius (unit.type);
        QPointF off;
        for (QHash<quint32, Unit>::iterator related_it = units.begin (); related_it != units.end (); ++related_it) {
            if (related_it == it)
                continue;
            Unit& related_unit = related_it.value ();
            QPointF& related_position = related_unit.position;
            qreal related_unit_radius = unitRadius (related_unit.type);
            qreal min_distance = unit_radius + related_unit_radius;
            QPointF delta = position - related_position;
            if (QPointF::dotProduct (delta, delta) < min_distance*min_distance) {
                qreal delta_length = qSqrt (QPointF::dotProduct (delta, delta));
                if (delta_length < 0.00001) {
                    qreal dx, dy;
                    qreal angle = qreal (M_PI*2.0*random_generator ())/(qreal (std::numeric_limits<quint32>::max ()) + 1.0);
                    sincos (angle, &dy, &dx);
                    delta.setX (dx*min_distance);
                    delta.setY (dy*min_distance);
                } else {
                    qreal distance_to_comfort = min_distance - delta_length;
                    delta *= distance_to_comfort/delta_length;
                }
                off += delta;
            }
        }
        offsets.append (off);
    }
    {
        size_t i = 0;
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            QPointF& position = unit.position;
            QPointF off = offsets[i++];
            qreal max_velocity = unitMaxVelocity (unit.type)*0.9; // TODO: Make force depend on distance
            qreal path_length = max_velocity*dt;
            qreal square_length = QPointF::dotProduct (off, off);
            if (square_length <= path_length*path_length) {
                position += off;
            } else {
                position += off*(path_length/qSqrt (square_length));
            }
        }
    }
}
bool MatchState::intersectRectangleCircle (const QRectF& rect, const QPointF& center, qreal radius)
{
    if (center.x () < rect.left ()) {
        if (center.y () < rect.top ()) {
            QPointF delta = center - rect.topLeft ();
            return QPointF::dotProduct (delta, delta) <= radius*radius;
        } else if (center.y () > rect.bottom ()) {
            QPointF delta = center - rect.bottomLeft ();
            return QPointF::dotProduct (delta, delta) <= radius*radius;
        } else {
            return center.x () >= (rect.left () - radius);
        }
    } else if (center.x () > rect.right ()) {
        if (center.y () < rect.top ()) {
            QPointF delta = center - rect.topRight ();
            return QPointF::dotProduct (delta, delta) <= radius*radius;
        } else if (center.y () > rect.bottom ()) {
            QPointF delta = center - rect.bottomRight ();
            return QPointF::dotProduct (delta, delta) <= radius*radius;
        } else {
            return center.x () <= (rect.right () + radius);
        }
    } else {
        if (center.y () < rect.top ()) {
            return center.y () >= (rect.top () - radius);
        } else if (center.y () > rect.bottom ()) {
            return center.y () <= (rect.bottom () + radius);
        } else {
            return true;
        }
    }
}
qreal MatchState::normalizeOrientation (qreal orientation)
{
    return remainder (orientation, M_PI*2.0);
}
void MatchState::clearSelection ()
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it)
        it->selected = false;
}
void MatchState::rotateUnit (Unit& unit, qreal dt, qreal dest_orientation)
{
    qreal delta = remainder (dest_orientation - unit.orientation, M_PI*2.0);
    qreal max_delta = dt*unitMaxAngularVelocity (unit.type);
    if (qAbs (delta) < max_delta) {
        unit.orientation = dest_orientation;
    } else {
        if (delta >= 0.0)
            unit.orientation += max_delta;
        else
            unit.orientation -= max_delta;
    }
}
qreal MatchState::unitDiameter (Unit::Type type) const
{
    switch (type) {
    case Unit::Type::Seal:
        return 2.0/3.0;
    case Unit::Type::Crusader:
        return 1.0;
    default:
        return 0.0;
    }
}
qreal MatchState::unitRadius (Unit::Type type) const
{
    return unitDiameter (type)*0.5;
}
qreal MatchState::unitMaxVelocity (Unit::Type type) const
{
    switch (type) {
    case Unit::Type::Seal:
        return 4.0;
    case Unit::Type::Crusader:
        return 7.0;
    default:
        return 0.0;
    }
}
qreal MatchState::unitMaxAngularVelocity (Unit::Type type) const
{
    switch (type) {
    case Unit::Type::Seal:
        return M_PI*4.0;
    case Unit::Type::Crusader:
        return M_PI*7.0;
    default:
        return 0.0;
    }
}
int MatchState::unitHitBarCount (Unit::Type type) const
{
    switch (type) {
    case Unit::Type::Seal:
        return 4;
    case Unit::Type::Crusader:
        return 5;
    default:
        return 0;
    }
}
int MatchState::unitMaxHP (Unit::Type type) const
{
    switch (type) {
    case Unit::Type::Seal:
        return 40;
    case Unit::Type::Crusader:
        return 100;
    default:
        return 0;
    }
}
const AttackDescription& MatchState::unitPrimaryAttackDescription (Unit::Type type) const
{
    static const AttackDescription seal = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::Immediate;
        ret.range = 5.0;
        ret.duration_ticks = 20;
        ret;
    });
    static const AttackDescription crusader = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::Immediate;
        ret.duration_ticks = 20;
        ret;
    });
    static const AttackDescription unkown = {};

    switch (type) {
    case Unit::Type::Seal:
        return seal;
    case Unit::Type::Crusader:
        return crusader;
    default:
        return unkown;
    }
}
quint64 MatchState::animationPeriodNS (Unit::Type type) const
{
    switch (type) {
    case Unit::Type::Seal:
        return 500000000;
    case Unit::Type::Crusader:
        return 300000000;
    default:
        return 0;
    }
}
bool MatchState::checkUnitSelection (const Unit& unit, const QPointF& point) const
{
    qreal radius = unitRadius (unit.type);
    if (radius == 0.0)
        return false;
    QPointF delta = point - unit.position;
    return QPointF::dotProduct (delta, delta) <= radius*radius;
}
bool MatchState::checkUnitSelection (const Unit& unit, const QRectF& rect) const
{
    qreal radius = unitRadius (unit.type);
    if (radius == 0.0)
        return false;
    return intersectRectangleCircle (rect, unit.position, radius);
}
void MatchState::applyAction (const MoveAction& action)
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected)
            unit.action = action;
    }
}
void MatchState::applyAction (const AttackAction& action)
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected)
            unit.action = action;
    }
}

void MatchState::LoadState(const QVector<QPair<quint32, Unit> >& other, const QVector<QPair<quint32, quint32> >& to_delete) {
    /*QSet<quint32> to_keep;
    for (quint32 i = 0; i < other.size(); i++) {
        to_keep.insert(other.at(i).first);
    }
    QVector<quint32> to_delete; // Не нашел способа лучше удалить умерших/исчезнувших юнитов
    for (QHash<quint32, Unit>::const_iterator iter = unitsRef().cbegin(); iter != unitsRef().cend(); iter++) {
        if (to_keep.find(iter.key()) == to_keep.end()) {
            to_delete.push_back(iter.key());
        }
    }
    for (quint32 id : to_delete) {
        units.remove(id);
    }*/
    for (const QPair<quint32, quint32> &i : to_delete) {
        units[i.second] = std::move(units[i.first]);
        units.remove(i.first);
    }

    //qDebug() << "Deleted" << to_delete.size() << "units";
    for (quint32 i = 0; i < other.size(); i++) {
        if (unitsRef().find(other.at(i).first) == unitsRef().end()) {
            addUnit(other.at(i).first, other.at(i).second.type, other.at(i).second.team, other.at(i).second.position, other.at(i).second.orientation);
        } else {
            QHash<quint32, Unit>::iterator to_change = units.find(other.at(i).first);
            to_change.value().position = other.at(i).second.position;
            to_change.value().orientation = other.at(i).second.orientation;
        }
    }
    //qDebug() << "Changed" << other.size() - to_add.size() << "Units";
    /*for (QHash<quint32, Unit>::const_iterator iter = units.cbegin(); iter != units.cend(); iter++) {
        qDebug() << iter.key();
    }*/
    /*for (QHash<quint32, Unit>::const_iterator iter = other->unitsRef().cbegin(); iter != other->unitsRef().cend(); iter++) {
        createUnit(iter->type, iter->team, iter->position, 0);
    }*/
}