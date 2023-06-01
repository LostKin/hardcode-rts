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
static inline qreal vectorRadius (const QPointF& v)
{
    return qAtan2 (v.y (), v.x ());
}
static inline qreal vectorSize (const QPointF& v)
{
    return std::hypot (v.x (), v.y ());
}
static inline void vectorResize (QPointF& v, qreal new_size)
{
    qreal old_size = std::hypot (v.x (), v.y ());
    if (old_size > 0.000000001)
        v *= new_size/old_size;
}
static inline qreal unitDistance (const Unit& a, const Unit& b)
{
    return vectorSize (a.position - b.position);
}
static bool intersectRectangleCircle (const QRectF& rect, const QPointF& center, qreal radius)
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
static bool pointInsideCircle (const QPointF& point, const QPointF& center, qreal radius)
{
    return vectorSize (point - center) <= radius;
}
static bool orientationsFuzzyMatch (qreal a, qreal b)
{
    return qAbs (remainder (a - b, M_PI*2.0)) <= (M_PI/180.0); // Within 1 degree
}

MatchState::MatchState (bool is_client) : is_client(is_client)
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
const QHash<quint32, Missile>& MatchState::missilesRef () const
{
    return missiles;
}
const QHash<quint32, Explosion>& MatchState::explosionsRef () const
{
    return explosions;
}

quint32 MatchState::getRandomNumber() {
    quint32 id = quint32 (random_generator ());
    id = id % (1 << 30);
    id = id + (int(is_client) << 30);
    return id;
}

QHash<quint32, Unit>::iterator MatchState::createUnit (Unit::Type type, Unit::Team team, const QPointF& position, qreal direction)
{

    quint32 id = getRandomNumber();
    
    //qDebug() << "matchstate.cpp" << position.x() << position.y();

    QHash<quint32, Unit>::iterator unit = units.insert (next_id++, {type, id, team, position, direction});
    unit->hp = unitMaxHP (unit->type);
    return unit;
}

Unit& MatchState::addUnit(quint32 id, Unit::Type type, Unit::Team team, const QPointF& position, qreal direction) {
    //qDebug() << "created a new unit " << id;
    Unit& unit = *units.insert (id, {type, quint32(random_generator()), team, position, direction});
    unit.hp = unitMaxHP (unit.type);
    return unit;
}

Missile& MatchState::addMissile(quint32 id, Missile::Type type, Unit::Team team, const QPointF& position, qreal direction) {
    Missile& missile = *missiles.insert (id, {type, team, position, 0, QPointF(0, 0)});
    return missile;
}

void MatchState::trySelect (Unit::Team team, const QPointF& point, bool add)
{
    if (add) {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            if (unit.team == team && checkUnitInsideSelection (unit, point))
                unit.selected = !unit.selected;
        }
    } else {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            if (unit.team == team && checkUnitInsideSelection (unit, point)) {
                clearSelection ();
                unit.selected = true;
                break;
            }
        }
    }
}

void MatchState::setAction(quint32 unit_id, const std::variant<StopAction, AttackAction, MoveAction, CastAction>& action) {
    QHash<quint32, Unit>::iterator iter = units.find(unit_id);
    if (iter == units.end()) {
        return;
    }
    iter->action = action;
}

void MatchState::select (quint32 unit_id, bool add) {
    if (!add) {
        clearSelection();
    }
    QHash<quint32, Unit>::iterator it = units.find(unit_id);
    if (it != units.end()) {
        Unit& unit = it.value ();
        unit.selected = true;
    }
}

void MatchState::trimSelection (Unit::Type type, bool remove)
{
    if (remove) {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            if (unit.type == type)
                unit.selected = false;
        }
    } else {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            if (unit.type != type)
                unit.selected = false;
        }
    }
}

void MatchState::deselect (quint32 unit_id)
{
    QHash<quint32, Unit>::iterator it = units.find(unit_id);
    if (it != units.end()) {
        Unit& unit = it.value ();
        unit.selected = false;
    }
}

void MatchState::trySelect (Unit::Team team, const QRectF& rect, bool add)
{
    if (add) {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            if (unit.team == team && checkUnitInsideSelection (unit, rect))
                unit.selected = true;
        }
    } else {
        bool selection_found = false;
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = it.value ();
            if (unit.team == team && checkUnitInsideSelection (unit, rect)) {
                selection_found = true;
                break;
            }
        }
        if (selection_found) {
            clearSelection ();
            for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
                Unit& unit = it.value ();
                if (unit.team == team && checkUnitInsideSelection (unit, rect))
                    unit.selected = true;
            }
        }
    }
}
void MatchState::trySelectByType (Unit::Team team, const QPointF& point, const QRectF& viewport, bool add)
{
    Unit* unit = findUnitAt (team, point);
    if (!unit)
        return;
    Unit::Type type = unit->type;
    if (!add)
        clearSelection ();
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.team == team && unit.type == type && checkUnitInsideViewport (unit, viewport)) {
            unit.selected = true;
        }
    }
}
void MatchState::selectAll (Unit::Team team)
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.team == team) {
            unit.selected = true;
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
        if (unit.team != attacker_team && checkUnitInsideSelection (unit, point)) {
            target = it.key ();
            break;
        }
    }
    startAction (target.has_value () ? AttackAction (*target) : AttackAction (point));
}
void MatchState::cast (CastAction::Type cast_type, Unit::Team /* attacker_team */, const QPointF& point)
{
    startAction (CastAction (cast_type, point));
}
void MatchState::move (const QPointF& point)
{
    std::optional<quint32> target;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (checkUnitInsideSelection (unit, point)) {
            target = it.key ();
            break;
        }
    }
    startAction (target.has_value () ? MoveAction (*target) : MoveAction (point));
}
void MatchState::stop ()
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected)
            unit.action = StopAction ();
            emit unitActionRequested (it.key(), StopAction());
            
    }
}
void MatchState::autoAction (Unit::Team attacker_team, const QPointF& point)
{
    std::optional<quint32> target;
    bool target_is_enemy = false;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (checkUnitInsideSelection (unit, point)) {
            target = it.key ();
            target_is_enemy = it->team != attacker_team;
            break;
        }
    }
    if (target.has_value ()) {
        if (target_is_enemy) {
            startAction (AttackAction (*target));
        } else {
            startAction (MoveAction (*target));
            //emit unitActionRequested ();
        }
    } else {
        startAction (MoveAction (point));
        //emit unitActionRequested ();
    }
}
void MatchState::redTeamUserTick (RedTeamUserData& /* user_data */)
{
}
void MatchState::blueTeamUserTick (BlueTeamUserData& user_data)
{
    QVector<const Missile*> new_missiles;
    for (QHash<quint32, Missile>::iterator it = missiles.begin (); it != missiles.end (); ++it) {
        if (!user_data.old_missiles.contains (it.key ())) {
            if (it->sender_team == Unit::Team::Red)
                new_missiles.append (&*it);
            user_data.old_missiles.insert (it.key ());
        }
    }

    {
        QSet<quint32>::iterator it = user_data.old_missiles.begin ();
        while (it != user_data.old_missiles.end()) {
            if (!missiles.contains (*it))
                it = user_data.old_missiles.erase (it);
            else
                ++it;
        }
    }

    const AttackDescription& pestilence_splash_attack = MatchState::effectAttackDescription (AttackDescription::Type::PestilenceSplash);
    const AttackDescription& rocket_explosion_attack = MatchState::effectAttackDescription (AttackDescription::Type::GoonRocketExplosion);
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
        if (unit.team == Unit::Team::Blue) {
            for (const Missile* missile: new_missiles) {
                if (missile->type == Missile::Type::Pestilence) {
                    qreal radius = pestilence_splash_attack.range + unitRadius (unit.type);
                    if (pointInsideCircle (unit.position, missile->target_position, radius)) {
                        QPointF displacement = unit.position - missile->target_position;
                        vectorResize (displacement, radius*1.05);
                        unit.action = MoveAction (missile->target_position + displacement);
                        break;
                    }
                } else if (missile->type == Missile::Type::Rocket) {
                    qreal radius = rocket_explosion_attack.range + unitRadius (unit.type);
                    if (pointInsideCircle (unit.position, missile->target_position, radius) &&
                        (!missile->target_unit.has_value () || missile->target_unit.value () != it.key ())) {
                        QPointF displacement = unit.position - missile->target_position;
                        vectorResize (displacement, radius*1.05);
                        unit.action = MoveAction (missile->target_position + displacement);
                        break;
                    }
                }
            }
        }
    }
}

quint32 MatchState::get_tick_no () {
    return tick_no;
}

void MatchState::tick ()
{
    tick_no += 1;
    redTeamUserTick (red_team_user_data);
    blueTeamUserTick (blue_team_user_data);

    constexpr uint64_t dt_nsec = 20'000'000;
    constexpr qreal dt = 0.020;

    clock_ns += dt_nsec;

    // TODO: Implement current and new positions for actions

    applyActions (dt);
    applyEffects (dt);
    applyAreaBoundaryCollisions (dt);
    applyUnitCollisions (dt); // TODO: Possibly optimize fron O (N^2) to O (N*log (N))
    killUnits();
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
    case Unit::Type::Goon:
        return 1.0;
    case Unit::Type::Beetle:
        return 0.5;
    case Unit::Type::Contaminator:
        return 1.5;
    default:
        return 0.0;
    }
}
qreal MatchState::missileDiameter (Missile::Type type) const
{
    switch (type) {
    case Missile::Type::Rocket:
        return 0.5;
    case Missile::Type::Pestilence:
        return 0.5;
    default:
        return 0.0;
    }
}
qreal MatchState::explosionDiameter (Explosion::Type type) const
{
    switch (type) {
    case Explosion::Type::Fire:
        return 3.0;
    case Explosion::Type::Pestilence:
        return 3.0;
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
    case Unit::Type::Goon:
        return 4.0;
    case Unit::Type::Beetle:
        return 5.0;
    case Unit::Type::Contaminator:
        return 3.5;
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
    case Unit::Type::Goon:
        return M_PI*4.0;
    case Unit::Type::Beetle:
        return M_PI*7.0;
    case Unit::Type::Contaminator:
        return M_PI*4.0;
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
    case Unit::Type::Goon:
        return 5;
    case Unit::Type::Beetle:
        return 4;
    case Unit::Type::Contaminator:
        return 7;
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
    case Unit::Type::Goon:
        return 80;
    case Unit::Type::Beetle:
        return 20;
    case Unit::Type::Contaminator:
        return 120;
    default:
        return 0;
    }
}
const AttackDescription& MatchState::unitPrimaryAttackDescription (Unit::Type type) const
{
    static const AttackDescription seal = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::SealShot;
        ret.range = 5.0;
        ret.trigger_range = 7.0;
        ret.damage = 10;
        ret.duration_ticks = 20;
        ret;
    });
    static const AttackDescription crusader = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::CrusaderChop;
        ret.range = 0.1;
        ret.trigger_range = 4.0;
        ret.damage = 16;
        ret.duration_ticks = 40;
        ret;
    });
    static const AttackDescription goon = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::GoonRocket;
        ret.range = 8.0;
        ret.trigger_range = 9.0;
        ret.damage = 12;
        ret.missile_velocity = 16.0;
        ret.duration_ticks = 60;
        ret;
    });
    static const AttackDescription beetle = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::BeetleSlice;
        ret.range = 0.1;
        ret.trigger_range = 3.0;
        ret.damage = 8;
        ret.duration_ticks = 40;
        ret;
    });
    static const AttackDescription unkown = {};

    switch (type) {
    case Unit::Type::Seal:
        return seal;
    case Unit::Type::Crusader:
        return crusader;
    case Unit::Type::Goon:
        return goon;
    case Unit::Type::Beetle:
        return beetle;
    default:
        return unkown;
    }
}
const AttackDescription& MatchState::effectAttackDescription (AttackDescription::Type type) const
{
    static const AttackDescription goon_rocket_explosion = ({
        AttackDescription ret;
        ret.type = type;
        ret.range = 1.4;
        ret.damage = 8;
        ret.duration_ticks = 20;
        ret;
    });
    static const AttackDescription pestilence_missile = ({
        AttackDescription ret;
        ret.type = type;
        ret.range = 7.0;
        ret.damage = 0;
        ret.missile_velocity = 16.0;
        ret.duration_ticks = 20;
        ret.cooldown_ticks = 40;
        ret;
    });
    static const AttackDescription pestilence_splash = ({
        AttackDescription ret;
        ret.type = type;
        ret.range = 1.8;
        ret.damage = 6;
        ret.duration_ticks = 20;
        ret.friendly_fire = false;
        ret;
    });
    static const AttackDescription spawn_beetle = ({
        AttackDescription ret;
        ret.type = type;
        ret.range = 4;
        ret.duration_ticks = 20;
        ret.cooldown_ticks = 20;
        ret;
    });
    static const AttackDescription unkown = {};

    switch (type) {
    case AttackDescription::Type::GoonRocketExplosion:
        return goon_rocket_explosion;
    case AttackDescription::Type::PestilenceMissile:
        return pestilence_missile;
    case AttackDescription::Type::PestilenceSplash:
        return pestilence_splash;
    case AttackDescription::Type::SpawnBeetle:
        return spawn_beetle;
    default:
        return unkown;
    }
}
void MatchState::selectGroup (quint64 group)
{
    quint64 group_flag = 1 << group;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        unit.selected = (unit.groups & group_flag) ? true : false;
    }
}
void MatchState::bindSelectionToGroup (quint64 group)
{
    quint64 group_flag = 1 << group;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected)
            unit.groups |= group_flag;
        else
            unit.groups &= ~group_flag;
    }
}
void MatchState::addSelectionToGroup (quint64 group)
{
    quint64 group_flag = 1 << group;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected)
            unit.groups |= group_flag;
    }
}
void MatchState::moveSelectionToGroup (quint64 group, bool add)
{
    quint64 group_flag = 1 << group;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected)
            unit.groups = group_flag;
        else if (!add)
            unit.groups &= ~group_flag;
    }
}
bool MatchState::fuzzyMatchPoints (const QPointF& p1, const QPointF& p2) const
{
    const qreal ratio = 0.5;
    return vectorSize (p2 - p1) < unitDiameter (Unit::Type::Beetle)*ratio;
}
bool MatchState::checkUnitInsideSelection (const Unit& unit, const QPointF& point) const
{
    qreal radius = unitRadius (unit.type);
    if (radius == 0.0)
        return false;
    QPointF delta = point - unit.position;
    return QPointF::dotProduct (delta, delta) <= radius*radius;
}
bool MatchState::checkUnitInsideSelection (const Unit& unit, const QRectF& rect) const
{
    qreal radius = unitRadius (unit.type);
    if (radius == 0.0)
        return false;
    return intersectRectangleCircle (rect, unit.position, radius);
}
bool MatchState::checkUnitInsideViewport (const Unit& unit, const QRectF& viewport) const
{
    qreal radius = unitRadius (unit.type);
    if (radius == 0.0)
        return false;
    return intersectRectangleCircle (viewport, unit.position, radius);
}
void MatchState::startAction (const MoveAction& action)
{
    // TODO: Check for team
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected) {
            unit.action = action;
            QPointF target;
            if (std::holds_alternative<quint32> (action.target)) {
                quint32 target_unit_id = std::get<quint32> (action.target);
                QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
                if (target_unit_it != units.end ()) {
                    Unit& target_unit = *target_unit_it;
                    target = target_unit.position;
                }
            } else {
                target = std::get<QPointF>(action.target);
            }
            emit unitActionRequested (it.key(), action);
        }
    }
}
void MatchState::startAction (const AttackAction& action)
{
    // TODO: Check for team
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected) {
            unit.action = action;
            emit unitActionRequested (it.key(), action);
        }
    }
}
void MatchState::LoadState(const QVector<QPair<quint32, Unit>>& other, QVector<QPair<quint32, Missile>>& other_missiles) {
    QSet<quint32> to_keep;
    for (quint32 i = 0; i < other.size(); i++) {
        to_keep.insert(other.at(i).first);
    }
    auto it = units.begin ();
    while (it != units.end ()) {
        if (to_keep.find (it.key ()) == to_keep.end ())
            it = units.erase (it);
        else
            ++it;
    }
    for (quint32 i = 0; i < other.size(); i++) {
        if (unitsRef().find(other.at(i).first) == unitsRef().end()) {
            Unit& unit = addUnit(other.at(i).first, other.at(i).second.type, other.at(i).second.team, other.at(i).second.position, other.at(i).second.orientation);
            unit.action = other.at(i).second.action;
        } else {
            QHash<quint32, Unit>::iterator to_change = units.find(other.at(i).first);
            to_change.value().position = other.at(i).second.position;
            to_change.value().orientation = other.at(i).second.orientation;
            to_change.value().hp = other.at(i).second.hp;
            to_change.value().action = other.at(i).second.action;
            to_change.value().attack_remaining_ticks = other.at(i).second.attack_remaining_ticks;
            to_change.value().cast_cooldown_left_ticks = other.at(i).second.cast_cooldown_left_ticks;
        }
    }

    //qDebug() << unitsRef().size();

    QSet<quint32> m_to_keep;
    for (quint32 i = 0; i < other_missiles.size(); i++) {
        m_to_keep.insert(other_missiles.at(i).first);
    }
    auto m_it = missiles.begin ();
    while (m_it != missiles.end ()) {
        if (m_to_keep.find (m_it.key ()) == m_to_keep.end ())
            m_it = missiles.erase (m_it);
        else
            ++m_it;
    }

    for (quint32 i = 0; i < other_missiles.size(); i++) {
        if (missilesRef().find(other_missiles.at(i).first) == missilesRef().end()) {
            Missile& missile = addMissile(other_missiles.at(i).first, other_missiles.at(i).second.type, other_missiles.at(i).second.sender_team, other_missiles.at(i).second.position, other_missiles.at(i).second.orientation);
            missile.target_position = other_missiles.at(i).second.target_position;
            if (other_missiles.at(i).second.target_unit.has_value()) {
                //qDebug() << "MatchState Missile has target unit";
                missile.target_unit = other_missiles.at(i).second.target_unit;
            } else {
                missile.target_unit.reset ();
            }
            //qDebug() << "created a new missile";
        } else {
            QHash<quint32, Missile>::iterator to_change = missiles.find(other_missiles.at(i).first);
            to_change.value().position = other_missiles.at(i).second.position;
            to_change.value().orientation = other_missiles.at(i).second.orientation;
            to_change.value().target_position = other_missiles.at(i).second.target_position;
            if (other_missiles.at(i).second.target_unit.has_value()) {
                //qDebug() << "MatchState Missile has target unit";
                to_change.value().target_unit = other_missiles.at(i).second.target_unit;
            } else {
                to_change.value().target_unit.reset ();
            }
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
void MatchState::startAction (const CastAction& action)
{
    // TODO: Check for team
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.selected) {
            if (unit.type == Unit::Type::Contaminator && unit.cast_cooldown_left_ticks <= 0) {
                switch (action.type) {
                case CastAction::Type::Pestilence:
                    unit.action = action;
                    emit unitActionRequested (it.key(), action);
                    break;
                case CastAction::Type::SpawnBeetle:
                    unit.action = action;
                    emit unitActionRequested (it.key(), action);
                    break;
                default:
                    break;
                }
                break;
            }
        }
    }
}

void MatchState::emitMissile (Missile::Type missile_type, const Unit& unit, quint32 target_unit_id, const Unit& target_unit)
{
    missiles.insert (next_id++, {missile_type, unit.team, unit.position, target_unit_id, target_unit.position});
}
void MatchState::emitMissile (Missile::Type missile_type, const Unit& unit, const QPointF& target)
{
    missiles.insert (next_id++, {missile_type, unit.team, unit.position, target});
}
void MatchState::emitExplosion (Explosion::Type explosion_type, Unit::Team sender_team, const QPointF& position)
{
    const AttackDescription& attack_description = ({
        const AttackDescription* ret;
        switch (explosion_type) {
        case Explosion::Type::Fire:
            ret = &effectAttackDescription (AttackDescription::Type::GoonRocketExplosion);
            break;
        case Explosion::Type::Pestilence:
            ret = &effectAttackDescription (AttackDescription::Type::PestilenceSplash);
            break;
        default:
            return;
        }
        *ret;
    });

    *explosions.insert (next_id++, {explosion_type, position, attack_description.duration_ticks});

    for (QMutableHashIterator<quint32, Unit> it (units); it.hasNext ();) {
        it.next ();
        Unit& target_unit = it.value ();
        if (attack_description.friendly_fire || target_unit.team != sender_team) {
            QPointF displacement = target_unit.position - position;
            qreal displacement_length = vectorSize (displacement);
            if (displacement_length <= attack_description.range + unitRadius (target_unit.type))
                dealDamage (target_unit, attack_description.damage);
        }
    }
    switch (explosion_type) {
    case Explosion::Type::Fire:
        emit soundEventEmitted (SoundEvent::RocketExplosion);
        break;
    case Explosion::Type::Pestilence:
        emit soundEventEmitted (SoundEvent::PestilenceSplash);
        break;
    default:
        break;
    }
}
void MatchState::applyAreaBoundaryCollisions (qreal dt)
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        applyAreaBoundaryCollision (unit, dt);
    }
}
void MatchState::applyActions (qreal dt)
{
    // Apply action
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
        if (unit.cast_cooldown_left_ticks > 0)
            --unit.cast_cooldown_left_ticks;
        if (unit.attack_remaining_ticks > 1) {
            --unit.attack_remaining_ticks;
        } else if (std::holds_alternative<MoveAction> (unit.action)) {
            if (unit.attack_remaining_ticks > 0)
                --unit.attack_remaining_ticks;
            const std::variant<QPointF, quint32>& target = std::get<MoveAction> (unit.action).target;
            if (std::holds_alternative<QPointF> (target)) {
                const QPointF& target_position = std::get<QPointF> (target);
                applyMovement (unit, target_position, dt, true);
            } else if (std::holds_alternative<quint32> (target)) {
                quint32 target_unit_id = std::get<quint32> (target);
                QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
                if (target_unit_it != units.end ()) {
                    Unit& target_unit = *target_unit_it;
                    applyMovement (unit, target_unit.position, dt, false);
                } else {
                    unit.action = StopAction ();
                }
            }
        } else if (std::holds_alternative<AttackAction> (unit.action)) {
            if (unit.attack_remaining_ticks > 0)
                --unit.attack_remaining_ticks;
            AttackAction& attack_action = std::get<AttackAction> (unit.action);
            const std::variant<QPointF, quint32>& target = attack_action.target;
            if (std::holds_alternative<QPointF> (target)) {
                std::optional<quint32> closest_target = attack_action.current_target;
                if (!closest_target.has_value ())
                    closest_target = findClosestTarget (unit);
                if (closest_target.has_value ()) {
                    attack_action.current_target = closest_target;
                    quint32 target_unit_id = attack_action.current_target.value ();
                    QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
                    if (target_unit_it != units.end ()) {
                        Unit& target_unit = *target_unit_it;
                        applyAttack (unit, target_unit_id, target_unit, dt);
                    } else {
                        attack_action.current_target.reset ();
                    }
                } else {
                    applyMovement (unit, std::get<QPointF> (target), dt, true);
                }
            } else if (std::holds_alternative<quint32> (target)) {
                quint32 target_unit_id = std::get<quint32> (target);
                QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
                if (target_unit_it != units.end ()) {
                    Unit& target_unit = *target_unit_it;
                    applyAttack (unit, target_unit_id, target_unit, dt);
                } else {
                    unit.action = StopAction ();
                }
            }
        } else if (std::holds_alternative<CastAction> (unit.action)) {
            const CastAction& cast_action = std::get<CastAction> (unit.action);
            applyCast (unit, cast_action.type, cast_action.target, dt);
        } else if (std::holds_alternative<StopAction> (unit.action)) {
            if (unit.attack_remaining_ticks > 0)
                --unit.attack_remaining_ticks;
            StopAction& stop_action = std::get<StopAction> (unit.action);
            std::optional<quint32> closest_target = stop_action.current_target;
            if (!closest_target.has_value ())
                closest_target = findClosestTarget (unit);
            if (closest_target.has_value ()) {
                stop_action.current_target = closest_target;
                quint32 target_unit_id = stop_action.current_target.value ();
                QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
                if (target_unit_it != units.end ()) {
                    Unit& target_unit = *target_unit_it;
                    applyAttack (unit, target_unit_id, target_unit, dt);
                } else {
                    stop_action.current_target.reset ();
                }
            }
            //emit unitActionRequested (it.key(), stop_action);
        } else if (unit.attack_remaining_ticks > 0) {
            --unit.attack_remaining_ticks;
        }
    }
}
void MatchState::applyEffects (qreal dt)
{
    applyMissilesMovement (dt);
    applyExplosionEffects (dt);
}
void MatchState::applyMissilesMovement (qreal dt)
{
    for (QMutableHashIterator<quint32, Missile> it (missiles); it.hasNext ();) {
        it.next ();
        Missile& missile = it.value ();
        if (missile.target_unit.has_value ()) {
            quint32 target_unit_id = *missile.target_unit;
            QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
            if (target_unit_it != units.end ()) {
                missile.target_position = target_unit_it->position;
                QPointF direction = missile.target_position - missile.position;
                missile.orientation = qAtan2 (direction.y (), direction.x ());
            } else {
                missile.target_unit.reset ();
            }
        }
        QPointF displacement = missile.target_position - missile.position;
        qreal max_velocity = 0.0;
        switch (missile.type) {
        case Missile::Type::Rocket: {
            const AttackDescription& attack_description = unitPrimaryAttackDescription (Unit::Type::Goon);
            max_velocity = attack_description.missile_velocity;
        } break;
        case Missile::Type::Pestilence: {
            const AttackDescription& attack_description = effectAttackDescription (AttackDescription::Type::PestilenceMissile);
            max_velocity = attack_description.missile_velocity;
        } break;
        default: {
        }
        }
        qreal path_length = max_velocity*dt;
        qreal displacement_length = vectorSize (displacement);
        if (displacement_length <= path_length) {
            if (missile.target_unit.has_value ()) {
                quint32 target_unit_id = *missile.target_unit;
                QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
                if (target_unit_it != units.end ()) {
                    Unit& target_unit = *target_unit_it;
                    switch (missile.type) {
                    case Missile::Type::Rocket: {
                        const AttackDescription& attack_description = unitPrimaryAttackDescription (Unit::Type::Goon);
                        dealDamage (target_unit, attack_description.damage);
                    } break;
                    default: {
                    }
                    }
                }
            }
            switch (missile.type) {
            case Missile::Type::Rocket:
                emitExplosion (Explosion::Type::Fire, missile.sender_team, missile.target_position);
                break;
            case Missile::Type::Pestilence:
                emitExplosion (Explosion::Type::Pestilence, missile.sender_team, missile.target_position);
                break;
            default:
                break;
            }
            it.remove ();
        } else {
            missile.position += displacement*(path_length/displacement_length);
        }
    }
}
void MatchState::applyExplosionEffects (qreal /* dt */)
{
    for (QMutableHashIterator<quint32, Explosion> it (explosions); it.hasNext ();) {
        it.next ();
        Explosion& explosion = it.value ();
        if (--explosion.remaining_ticks <= 0)
            it.remove ();
    }
}
void MatchState::applyMovement (Unit& unit, const QPointF& target_position, qreal dt, bool clear_action_on_completion)
{
    qreal max_velocity = unitMaxVelocity (unit.type);
    QPointF displacement = target_position - unit.position;
    rotateUnit (unit, dt, vectorRadius (displacement));
    qreal path_length = max_velocity*dt;
    qreal displacement_length = vectorSize (displacement);
    if (displacement_length <= path_length) {
        unit.position = target_position;
        if (clear_action_on_completion)
            unit.action = StopAction ();
    } else {
        unit.position += displacement*(path_length/displacement_length);
    }
}
void MatchState::applyAttack (Unit& unit, quint32 target_unit_id, Unit& target_unit, qreal dt)
{
    const AttackDescription& attack_description = unitPrimaryAttackDescription (unit.type);
    QPointF displacement = target_unit.position - unit.position;
    qreal target_orientation = vectorRadius (displacement);
    rotateUnit (unit, dt, target_orientation);
    qreal displacement_length = vectorSize (displacement);
    qreal full_attack_range = attack_description.range + unitRadius (unit.type) + unitRadius (target_unit.type);
    bool in_range = false;
    if (displacement_length > full_attack_range) {
        qreal path_length = unitMaxVelocity (unit.type)*dt;
        if (path_length >= displacement_length - full_attack_range) {
            path_length = displacement_length - full_attack_range;
            in_range = true;
        }
        unit.position += displacement*(path_length/displacement_length);
    } else {
        in_range = true;
    }
    if (in_range && orientationsFuzzyMatch (unit.orientation, target_orientation)) {
        switch (attack_description.type) {
        case AttackDescription::Type::SealShot: {
            dealDamage (target_unit, attack_description.damage);
            unit.attack_remaining_ticks = attack_description.duration_ticks;
            emit soundEventEmitted (SoundEvent::SealAttack);
        } break;
        case AttackDescription::Type::CrusaderChop: {
            dealDamage (target_unit, attack_description.damage);
            unit.attack_remaining_ticks = attack_description.duration_ticks;
            emit soundEventEmitted (SoundEvent::CrusaderAttack);
        } break;
        case AttackDescription::Type::GoonRocket: {
            unit.attack_remaining_ticks = attack_description.duration_ticks;
            emitMissile (Missile::Type::Rocket, unit, target_unit_id, target_unit);
            emit soundEventEmitted (SoundEvent::RocketStart);
        } break;
        case AttackDescription::Type::BeetleSlice: {
            dealDamage (target_unit, attack_description.damage);
            unit.attack_remaining_ticks = attack_description.duration_ticks;
            emit soundEventEmitted (SoundEvent::BeetleAttack);
        } break;
        default: {
        }
        }
    }
}
void MatchState::applyCast (Unit& unit, CastAction::Type cast_type, const QPointF& target, qreal dt)
{
    const AttackDescription& attack_description = ({
        const AttackDescription* ret;
        switch (cast_type) {
        case CastAction::Type::Pestilence:
            ret = &effectAttackDescription (AttackDescription::Type::PestilenceMissile);
            break;
        case CastAction::Type::SpawnBeetle:
            ret = &effectAttackDescription (AttackDescription::Type::SpawnBeetle);
            break;
        default:
            return;
        }
        *ret;
    });
    QPointF displacement = target - unit.position;
    qreal target_orientation = vectorRadius (displacement);
    rotateUnit (unit, dt, target_orientation);
    qreal displacement_length = vectorSize (displacement);
    qreal full_attack_range = attack_description.range + unitRadius (unit.type);
    bool in_range = false;
    if (displacement_length > full_attack_range) {
        qreal path_length = unitMaxVelocity (unit.type)*dt;
        if (path_length >= displacement_length - full_attack_range) {
            path_length = displacement_length - full_attack_range;
            in_range = true;
        }
        unit.position += displacement*(path_length/displacement_length);
    } else {
        in_range = true;
    }
    if (in_range && orientationsFuzzyMatch (unit.orientation, target_orientation)) {
        switch (cast_type) {
        case CastAction::Type::Pestilence:
            emitMissile (Missile::Type::Pestilence, unit, target);
            unit.action = StopAction ();
            unit.cast_cooldown_left_ticks = attack_description.cooldown_ticks;
            emit soundEventEmitted (SoundEvent::PestilenceMissileStart);
            break;
        case CastAction::Type::SpawnBeetle:
            //createUnit (Unit::Type::Beetle, unit.team, target, unit.orientation);
            emit unitCreateRequested(unit.team, Unit::Type::Beetle, target);
            unit.action = StopAction ();
            unit.cast_cooldown_left_ticks = attack_description.cooldown_ticks;
            emit soundEventEmitted (SoundEvent::SpawnBeetle);
            break;
        default:
            return;
        }
    }
}
void MatchState::applyAreaBoundaryCollision (Unit& unit, qreal dt)
{
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
void MatchState::applyUnitCollisions (qreal dt)
{
    // Apply unit collisions: O (N^2)
    QVector<QPointF> offsets;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
        qreal unit_radius = unitRadius (unit.type);
        QPointF off;
        for (QHash<quint32, Unit>::iterator related_it = units.begin (); related_it != units.end (); ++related_it) {
            if (related_it == it)
                continue;
            Unit& related_unit = *related_it;
            qreal related_unit_radius = unitRadius (related_unit.type);
            qreal min_distance = unit_radius + related_unit_radius;
            QPointF delta = unit.position - related_unit.position;
            if (vectorSize (delta) < min_distance) {
                qreal delta_length = qSqrt (QPointF::dotProduct (delta, delta));
                if (delta_length < 0.00001) {
                    qreal dx, dy;
                    qreal angle = qreal (M_PI*2.0*random_generator ())/(qreal (std::numeric_limits<quint32>::max ()) + 1.0);
                    sincos (angle, &dy, &dx);
                    delta = {dx*min_distance, dy*min_distance};
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
            qreal length = vectorSize (off);
            position += (length <= path_length) ? off : off*(path_length/length);
        }
    }
}
void MatchState::dealDamage (Unit& unit, qint64 damage)
{
    unit.hp = std::max<qint64> (unit.hp - damage, 0);
}

void MatchState::killUnits () {
    for (auto it = units.begin(); it != units.end();)
    {
        if(it->hp == 0) {
            it = units.erase(it); 
        } else {
            ++it;
        }
    }
}

Unit* MatchState::findUnitAt (Unit::Team team, const QPointF& point)
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = it.value ();
        if (unit.team == team && checkUnitInsideSelection (unit, point))
            return &unit;
    }
    return nullptr;
}
std::optional<quint32> MatchState::findClosestTarget (const Unit& unit)
{
    std::optional<quint32> closest_target = {};
    qreal radius = unitRadius (unit.type);
    qreal trigger_range = unitPrimaryAttackDescription (unit.type).trigger_range;
    qreal minimal_range = 1000000000.0;
    for (QHash<quint32, Unit>::iterator target_it = units.begin (); target_it != units.end (); ++target_it) {
        if (target_it->team != unit.team &&
            unitDistance (unit, *target_it) <= qMin (radius + unitRadius (target_it->type) + trigger_range, minimal_range)) {
            minimal_range = unitDistance (unit, *target_it);
            closest_target = target_it.key ();
        }
    }
    return closest_target;
}
