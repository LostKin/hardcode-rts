#include "matchstate.h"

#include <QtMath>
#include <QDebug>
#include <QSet>

static constexpr qreal SQRT_1_2 = 0.70710678118655;

static inline qreal unitSquareDistance (const Unit& a, const Unit& b)
{
    QPointF delta = a.position - b.position;
    return delta.x () * delta.x () + delta.y () * delta.y ();
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
        v *= new_size / old_size;
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
            return QPointF::dotProduct (delta, delta) <= radius * radius;
        } else if (center.y () > rect.bottom ()) {
            QPointF delta = center - rect.bottomLeft ();
            return QPointF::dotProduct (delta, delta) <= radius * radius;
        } else {
            return center.x () >= (rect.left () - radius);
        }
    } else if (center.x () > rect.right ()) {
        if (center.y () < rect.top ()) {
            QPointF delta = center - rect.topRight ();
            return QPointF::dotProduct (delta, delta) <= radius * radius;
        } else if (center.y () > rect.bottom ()) {
            QPointF delta = center - rect.bottomRight ();
            return QPointF::dotProduct (delta, delta) <= radius * radius;
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
    return qAbs (remainder (a - b, M_PI * 2.0)) <= (M_PI / 180.0); // Within 1 degree
}

MatchState::MatchState (bool is_client)
    : is_client (is_client)
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
const QHash<quint32, Corpse>& MatchState::corpsesRef () const
{
    return corpses;
}
const QHash<quint32, Missile>& MatchState::missilesRef () const
{
    return missiles;
}
const QHash<quint32, Explosion>& MatchState::explosionsRef () const
{
    return explosions;
}

quint32 MatchState::getRandomNumber ()
{
    quint32 id = quint32 (random_generator ());
    id = id % (1 << 30);
    id = id + (int (is_client) << 30);
    return id;
}
QHash<quint32, Unit>::iterator MatchState::createUnit (Unit::Type type, Unit::Team team, const QPointF& position, qreal direction)
{
    quint32 id = getRandomNumber (); // TODO: Fix it

    QHash<quint32, Unit>::iterator unit = units.insert (next_id++, {type, id, team, position, direction});
    unit->hp = unitMaxHP (unit->type);
    return unit;
}
Unit& MatchState::addUnit (quint32 id, Unit::Type type, Unit::Team team, const QPointF& position, qreal direction)
{
    Unit& unit = *units.insert (id, {type, quint32 (random_generator ()), team, position, direction});
    unit.hp = unitMaxHP (unit.type);
    return unit;
}
Corpse& MatchState::addCorpse (quint32 id, Unit::Type type, Unit::Team team, const QPointF& position, qreal direction, quint64 decay_remaining_ticks)
{
    Corpse& corpse = *corpses.insert (id, Corpse ({type, quint32 (random_generator ()), team, position, direction}, decay_remaining_ticks));
    corpse.unit.hp = unitMaxHP (corpse.unit.type);
    return corpse;
}
Missile& MatchState::addMissile (quint32 id, Missile::Type type, Unit::Team team, const QPointF& position, qreal direction)
{
    Missile& missile = *missiles.insert (id, {type, team, position, 0, QPointF (0, 0)});
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
void MatchState::setUnitAction (quint32 unit_id, const UnitActionVariant& action)
{
    QHash<quint32, Unit>::iterator it = units.find (unit_id);
    if (it == units.end ())
        return;
    Unit& unit = *it;
    IntentiveActionVariant* next_action;
    if (std::holds_alternative<PerformingAttackAction> (unit.action))
        next_action = &std::get<PerformingAttackAction> (unit.action).next_action;
    else if (std::holds_alternative<PerformingCastAction> (unit.action))
        next_action = &std::get<PerformingCastAction> (unit.action).next_action;
    else
        next_action = nullptr;
    if (next_action) {
        if (std::holds_alternative<StopAction> (action))
            *next_action = std::get<StopAction> (action);
        else if (std::holds_alternative<MoveAction> (action))
            *next_action = std::get<MoveAction> (action);
        else if (std::holds_alternative<AttackAction> (action))
            *next_action = std::get<AttackAction> (action);
        else if (std::holds_alternative<CastAction> (action))
            *next_action = std::get<CastAction> (action);
    } else {
        if (std::holds_alternative<StopAction> (action))
            unit.action = std::get<StopAction> (action);
        else if (std::holds_alternative<MoveAction> (action))
            unit.action = std::get<MoveAction> (action);
        else if (std::holds_alternative<AttackAction> (action))
            unit.action = std::get<AttackAction> (action);
        else if (std::holds_alternative<CastAction> (action))
            unit.action = std::get<CastAction> (action);
        else if (std::holds_alternative<PerformingAttackAction> (action))
            unit.action = std::get<PerformingAttackAction> (action);
        else if (std::holds_alternative<PerformingCastAction> (action))
            unit.action = std::get<PerformingCastAction> (action);
    }
}
void MatchState::select (quint32 unit_id, bool add)
{
    if (!add)
        clearSelection ();
    QHash<quint32, Unit>::iterator it = units.find (unit_id);
    if (it != units.end ()) {
        Unit& unit = *it;
        unit.selected = true;
    }
}
void MatchState::trimSelection (Unit::Type type, bool remove)
{
    if (remove) {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = *it;
            if (unit.type == type)
                unit.selected = false;
        }
    } else {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = *it;
            if (unit.type != type)
                unit.selected = false;
        }
    }
}
void MatchState::deselect (quint32 unit_id)
{
    QHash<quint32, Unit>::iterator it = units.find (unit_id);
    if (it != units.end ()) {
        Unit& unit = *it;
        unit.selected = false;
    }
}
void MatchState::trySelect (Unit::Team team, const QRectF& rect, bool add)
{
    if (add) {
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = *it;
            if (unit.team == team && checkUnitInsideSelection (unit, rect))
                unit.selected = true;
        }
    } else {
        bool selection_found = false;
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = *it;
            if (unit.team == team && checkUnitInsideSelection (unit, rect)) {
                selection_found = true;
                break;
            }
        }
        if (selection_found) {
            clearSelection ();
            for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
                Unit& unit = *it;
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
        Unit& unit = *it;
        if (unit.team == team && unit.type == type && checkUnitInsideViewport (unit, viewport))
            unit.selected = true;
    }
}
void MatchState::selectAll (Unit::Team team)
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
        if (unit.team == team)
            unit.selected = true;
    }
}
std::optional<QPointF> MatchState::selectionCenter () const
{
    size_t selection_count = 0;
    QPointF sum = {};
    for (QHash<quint32, Unit>::const_iterator it = units.cbegin (); it != units.cend (); ++it) {
        const Unit& unit = *it;
        if (unit.selected) {
            ++selection_count;
            sum += unit.position;
        }
    }
    return selection_count ? std::optional<QPointF> (QPointF (sum / selection_count)) : std::optional<QPointF> ();
}
void MatchState::attackEnemy (Unit::Team attacker_team, const QPointF& point)
{
    std::optional<quint32> target;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
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
        Unit& unit = *it;
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
        Unit& unit = *it;
        if (unit.selected) {
            unit.action = StopAction ();
            emit unitActionRequested (it.key (), StopAction ());
        }
    }
}
void MatchState::autoAction (Unit::Team attacker_team, const QPointF& point)
{
    std::optional<quint32> target;
    bool target_is_enemy = false;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
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
            // emit unitActionRequested ();
        }
    } else {
        startAction (MoveAction (point));
        // emit unitActionRequested ();
    }
}
void MatchState::redTeamUserTick (RedTeamUserData& /* user_data */)
{
}
void MatchState::blueTeamUserTick (BlueTeamUserData& user_data)
{
    // TODO: Define proper API
    QVector<const Missile*> new_missiles;
    for (QHash<quint32, Missile>::iterator it = missiles.begin (); it != missiles.end (); ++it) {
        if (!user_data.old_missiles.contains (it.key ())) {
            if (it->sender_team == Unit::Team::Red)
                new_missiles.append (&*it);
            user_data.old_missiles.insert (it.key ());
        }
    }

    for (QSet<quint32>::iterator it = user_data.old_missiles.begin (); it != user_data.old_missiles.end ();) {
        if (!missiles.contains (*it))
            it = user_data.old_missiles.erase (it);
        else
            ++it;
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
                        vectorResize (displacement, radius * 1.05);
                        unit.action = MoveAction (missile->target_position + displacement);
                        break;
                    }
                } else if (missile->type == Missile::Type::Rocket) {
                    qreal radius = rocket_explosion_attack.range + unitRadius (unit.type);
                    if (pointInsideCircle (unit.position, missile->target_position, radius) &&
                        (!missile->target_unit.has_value () || missile->target_unit.value () != it.key ())) {
                        QPointF displacement = unit.position - missile->target_position;
                        vectorResize (displacement, radius * 1.05);
                        unit.action = MoveAction (missile->target_position + displacement);
                        break;
                    }
                }
            }
        }
    }
}
quint32 MatchState::get_tick_no ()
{
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
    applyDeath ();
    applyDecay ();
}
void MatchState::loadUnits (const QVector<QPair<quint32, Unit>>& new_units)
{
    QSet<quint32> to_keep;
    for (const QPair<quint32, Unit>& new_unit_entry: new_units)
        to_keep.insert (new_unit_entry.first);
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end ();) {
        if (to_keep.find (it.key ()) == to_keep.end ())
            it = units.erase (it);
        else
            ++it;
    }
    for (const QPair<quint32, Unit>& new_unit_entry: new_units) {
        quint32 new_unit_id = new_unit_entry.first;
        const Unit& new_unit = new_unit_entry.second;
        QHash<quint32, Unit>::iterator to_change = units.find (new_unit_id);
        if (to_change != units.end ()) {
            to_change->position = new_unit.position;
            to_change->orientation = new_unit.orientation;
            to_change->hp = new_unit.hp;
            to_change->action = new_unit.action;
            to_change->attack_cooldown_left_ticks = new_unit.attack_cooldown_left_ticks;
            to_change->cast_cooldown_left_ticks = new_unit.cast_cooldown_left_ticks;
        } else {
            Unit& unit = addUnit (new_unit_id, new_unit.type, new_unit.team, new_unit.position, new_unit.orientation);
            unit.action = new_unit.action;
        }
    }
}
void MatchState::loadCorpses (const QVector<QPair<quint32, Corpse>>& new_corpses)
{
    QSet<quint32> to_keep;
    for (const QPair<quint32, Corpse>& new_corpse_entry: new_corpses)
        to_keep.insert (new_corpse_entry.first);
    for (QHash<quint32, Corpse>::iterator it = corpses.begin (); it != corpses.end ();) {
        if (to_keep.find (it.key ()) == to_keep.end ())
            it = corpses.erase (it);
        else
            ++it;
    }
    for (const QPair<quint32, Corpse>& new_corpse_entry: new_corpses) {
        quint32 new_corpse_id = new_corpse_entry.first;
        const Corpse& new_corpse = new_corpse_entry.second;
        QHash<quint32, Corpse>::iterator to_change = corpses.find (new_corpse_id);
        if (to_change != corpses.end ()) {
            to_change->unit.position = new_corpse.unit.position;
            to_change->unit.orientation = new_corpse.unit.orientation;
            to_change->unit.hp = new_corpse.unit.hp;
            to_change->unit.action = new_corpse.unit.action;
            to_change->unit.attack_cooldown_left_ticks = new_corpse.unit.attack_cooldown_left_ticks;
            to_change->unit.cast_cooldown_left_ticks = new_corpse.unit.cast_cooldown_left_ticks;
        } else {
            Corpse& corpse = addCorpse (new_corpse_id, new_corpse.unit.type, new_corpse.unit.team, new_corpse.unit.position, new_corpse.unit.orientation, new_corpse.decay_remaining_ticks);
            corpse.unit.action = new_corpse.unit.action;
        }
    }
}
void MatchState::loadMissiles (const QVector<QPair<quint32, Missile>>& new_missiles)
{
    QSet<quint32> m_to_keep;
    for (quint32 i = 0; i < new_missiles.size (); i++) {
        m_to_keep.insert (new_missiles.at (i).first);
    }
    auto m_it = missiles.begin ();
    while (m_it != missiles.end ()) {
        if (m_to_keep.find (m_it.key ()) == m_to_keep.end ())
            m_it = missiles.erase (m_it);
        else
            ++m_it;
    }

    for (quint32 i = 0; i < new_missiles.size (); i++) {
        if (missilesRef ().find (new_missiles.at (i).first) == missilesRef ().end ()) {
            Missile& missile = addMissile (new_missiles.at (i).first, new_missiles.at (i).second.type, new_missiles.at (i).second.sender_team, new_missiles.at (i).second.position, new_missiles.at (i).second.orientation);
            missile.target_position = new_missiles.at (i).second.target_position;
            if (new_missiles.at (i).second.target_unit.has_value ()) {
                missile.target_unit = new_missiles.at (i).second.target_unit;
            } else {
                missile.target_unit.reset ();
            }

        } else {
            QHash<quint32, Missile>::iterator to_change = missiles.find (new_missiles.at (i).first);
            to_change.value ().position = new_missiles.at (i).second.position;
            to_change.value ().orientation = new_missiles.at (i).second.orientation;
            to_change.value ().target_position = new_missiles.at (i).second.target_position;
            if (new_missiles.at (i).second.target_unit.has_value ()) {
                to_change.value ().target_unit = new_missiles.at (i).second.target_unit;
            } else {
                to_change.value ().target_unit.reset ();
            }
        }
    }
}
void MatchState::clearSelection ()
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it)
        it->selected = false;
}
void MatchState::rotateUnit (Unit& unit, qreal dt, qreal dest_orientation)
{
    qreal delta = remainder (dest_orientation - unit.orientation, M_PI * 2.0);
    qreal max_delta = dt * unitMaxAngularVelocity (unit.type);
    if (qAbs (delta) < max_delta) {
        unit.orientation = dest_orientation;
    } else {
        if (delta >= 0.0)
            unit.orientation += max_delta;
        else
            unit.orientation -= max_delta;
    }
}
qreal MatchState::unitDiameter (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return 2.0 / 3.0;
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
qreal MatchState::missileDiameter (Missile::Type type)
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
qreal MatchState::explosionDiameter (Explosion::Type type)
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
qreal MatchState::unitRadius (Unit::Type type)
{
    return unitDiameter (type) * 0.5;
}
qreal MatchState::unitMaxVelocity (Unit::Type type)
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
qreal MatchState::unitMaxAngularVelocity (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return M_PI * 4.0;
    case Unit::Type::Crusader:
        return M_PI * 7.0;
    case Unit::Type::Goon:
        return M_PI * 4.0;
    case Unit::Type::Beetle:
        return M_PI * 7.0;
    case Unit::Type::Contaminator:
        return M_PI * 4.0;
    default:
        return 0.0;
    }
}
int MatchState::unitHitBarCount (Unit::Type type)
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
int MatchState::unitMaxHP (Unit::Type type)
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
const AttackDescription& MatchState::unitPrimaryAttackDescription (Unit::Type type)
{
    static const AttackDescription seal = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::SealShot;
        ret.range = 5.0;
        ret.trigger_range = 7.0;
        ret.damage = 10;
        ret.duration_ticks = 20;
        ret.cooldown_ticks = 30;
        ret;
    });
    static const AttackDescription crusader = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::CrusaderChop;
        ret.range = 0.1;
        ret.trigger_range = 4.0;
        ret.damage = 16;
        ret.duration_ticks = 40;
        ret.cooldown_ticks = 30;
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
        ret.cooldown_ticks = 30;
        ret;
    });
    static const AttackDescription beetle = ({
        AttackDescription ret;
        ret.type = AttackDescription::Type::BeetleSlice;
        ret.range = 0.1;
        ret.trigger_range = 3.0;
        ret.damage = 8;
        ret.duration_ticks = 40;
        ret.cooldown_ticks = 30;
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
const AttackDescription& MatchState::effectAttackDescription (AttackDescription::Type type)
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
        ret.duration_ticks = 10;
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
        Unit& unit = *it;
        unit.selected = (unit.groups & group_flag) ? true : false;
    }
}
void MatchState::bindSelectionToGroup (quint64 group)
{
    quint64 group_flag = 1 << group;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
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
        Unit& unit = *it;
        if (unit.selected)
            unit.groups |= group_flag;
    }
}
void MatchState::moveSelectionToGroup (quint64 group, bool add)
{
    quint64 group_flag = 1 << group;
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
        if (unit.selected)
            unit.groups = group_flag;
        else if (!add)
            unit.groups &= ~group_flag;
    }
}
bool MatchState::fuzzyMatchPoints (const QPointF& p1, const QPointF& p2) const
{
    const qreal ratio = 0.5;
    return vectorSize (p2 - p1) < unitDiameter (Unit::Type::Beetle) * ratio;
}
bool MatchState::checkUnitInsideSelection (const Unit& unit, const QPointF& point) const
{
    qreal radius = unitRadius (unit.type);
    if (radius == 0.0)
        return false;
    QPointF delta = point - unit.position;
    return QPointF::dotProduct (delta, delta) <= radius * radius;
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
void MatchState::LoadState (const QVector<QPair<quint32, Unit>>& units, const QVector<QPair<quint32, Corpse>>& corpses, const QVector<QPair<quint32, Missile>>& missiles)
{
    loadUnits (units);
    loadCorpses (corpses);
    loadMissiles (missiles);
}
void MatchState::startAction (const MoveAction& action)
{
    // TODO: Check for team
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
        if (unit.selected) {
            if (std::holds_alternative<PerformingAttackAction> (unit.action))
                std::get<PerformingAttackAction> (unit.action).next_action = action;
            else if (std::holds_alternative<PerformingCastAction> (unit.action))
                std::get<PerformingCastAction> (unit.action).next_action = action;
            else
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
                target = std::get<QPointF> (action.target);
            }
            emit unitActionRequested (it.key (), action);
        }
    }
}
void MatchState::startAction (const AttackAction& action)
{
    // TODO: Check for team
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
        if (unit.selected) {
            if (std::holds_alternative<PerformingAttackAction> (unit.action))
                std::get<PerformingAttackAction> (unit.action).next_action = action;
            else if (std::holds_alternative<PerformingCastAction> (unit.action))
                std::get<PerformingCastAction> (unit.action).next_action = action;
            else
                unit.action = action;
            emit unitActionRequested (it.key (), action);
        }
    }
}
void MatchState::startAction (const CastAction& action)
{
    // TODO: Check for team
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
        if (unit.selected) {
            if (unit.type == Unit::Type::Contaminator && unit.cast_cooldown_left_ticks <= 0) {
                switch (action.type) {
                case CastAction::Type::Pestilence:
                    if (std::holds_alternative<PerformingAttackAction> (unit.action))
                        std::get<PerformingAttackAction> (unit.action).next_action = action;
                    else if (std::holds_alternative<PerformingCastAction> (unit.action))
                        std::get<PerformingCastAction> (unit.action).next_action = action;
                    else
                        unit.action = action;
                    emit unitActionRequested (it.key (), action);
                    break;
                case CastAction::Type::SpawnBeetle:
                    if (std::holds_alternative<PerformingAttackAction> (unit.action))
                        std::get<PerformingAttackAction> (unit.action).next_action = action;
                    else if (std::holds_alternative<PerformingCastAction> (unit.action))
                        std::get<PerformingCastAction> (unit.action).next_action = action;
                    else
                        unit.action = action;
                    emit unitActionRequested (it.key (), action);
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
        Unit& unit = *it;
        applyAreaBoundaryCollision (unit, dt);
    }
}
void MatchState::applyActions (qreal dt)
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
        if (unit.attack_cooldown_left_ticks > 0)
            --unit.attack_cooldown_left_ticks;
        if (unit.cast_cooldown_left_ticks > 0)
            --unit.cast_cooldown_left_ticks;
        if (std::holds_alternative<StopAction> (unit.action)) {
            StopAction& stop_action = std::get<StopAction> (unit.action);
            std::optional<quint32> closest_target = stop_action.current_target;
            if (!closest_target.has_value ())
                closest_target = findClosestTarget (unit);
            if (closest_target.has_value ()) {
                stop_action.current_target = closest_target;
                if (!unit.attack_cooldown_left_ticks) {
                    quint32 target_unit_id = stop_action.current_target.value ();
                    QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
                    if (target_unit_it != units.end ()) {
                        Unit& target_unit = *target_unit_it;
                        if (applyAttack (unit, target_unit_id, target_unit, dt))
                            unit.action = PerformingAttackAction (StopAction (std::get<StopAction> (unit.action)), unitPrimaryAttackDescription (unit.type).duration_ticks);
                    } else {
                        stop_action.current_target.reset ();
                    }
                }
            }
        } else if (std::holds_alternative<MoveAction> (unit.action)) {
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
            AttackAction& attack_action = std::get<AttackAction> (unit.action);
            const std::variant<QPointF, quint32>& target = attack_action.target;
            if (std::holds_alternative<QPointF> (target)) {
                std::optional<quint32> closest_target = attack_action.current_target;
                if (!closest_target.has_value ())
                    closest_target = findClosestTarget (unit);
                if (closest_target.has_value ()) {
                    attack_action.current_target = closest_target;
                    if (!unit.attack_cooldown_left_ticks) {
                        quint32 target_unit_id = attack_action.current_target.value ();
                        QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
                        if (target_unit_it != units.end ()) {
                            Unit& target_unit = *target_unit_it;
                            if (applyAttack (unit, target_unit_id, target_unit, dt))
                                unit.action = PerformingAttackAction (AttackAction (std::get<AttackAction> (unit.action)), unitPrimaryAttackDescription (unit.type).duration_ticks);
                        } else {
                            attack_action.current_target.reset ();
                        }
                    }
                } else {
                    applyMovement (unit, std::get<QPointF> (target), dt, true);
                }
            } else if (std::holds_alternative<quint32> (target)) {
                if (!unit.attack_cooldown_left_ticks) {
                    quint32 target_unit_id = std::get<quint32> (target);
                    QHash<quint32, Unit>::iterator target_unit_it = units.find (target_unit_id);
                    if (target_unit_it != units.end ()) {
                        Unit& target_unit = *target_unit_it;
                        if (applyAttack (unit, target_unit_id, target_unit, dt))
                            unit.action = PerformingAttackAction (AttackAction (std::get<AttackAction> (unit.action)), unitPrimaryAttackDescription (unit.type).duration_ticks);
                    } else {
                        unit.action = StopAction ();
                    }
                }
            }
        } else if (std::holds_alternative<CastAction> (unit.action)) {
            const CastAction& cast_action = std::get<CastAction> (unit.action);
            if (applyCast (unit, cast_action.type, cast_action.target, dt)) {
                qint64 duration_ticks;
                switch (cast_action.type) {
                case CastAction::Type::Pestilence:
                    duration_ticks = MatchState::effectAttackDescription (AttackDescription::Type::PestilenceMissile).duration_ticks;
                    break;
                case CastAction::Type::SpawnBeetle:
                    duration_ticks = MatchState::effectAttackDescription (AttackDescription::Type::SpawnBeetle).duration_ticks;
                    break;
                default:
                    duration_ticks = 0;
                }
                unit.action = PerformingCastAction (cast_action.type, StopAction (), duration_ticks);
            }
        } else if (std::holds_alternative<PerformingAttackAction> (unit.action)) {
            PerformingAttackAction& performing_attack_action = std::get<PerformingAttackAction> (unit.action);
            --performing_attack_action.remaining_ticks;
            if (performing_attack_action.remaining_ticks <= 0) {
                const AttackDescription& attack_description = unitPrimaryAttackDescription (Unit::Type::Goon);
                unit.attack_cooldown_left_ticks = attack_description.cooldown_ticks;
                if (std::holds_alternative<StopAction> (performing_attack_action.next_action))
                    unit.action = StopAction (std::get<StopAction> (performing_attack_action.next_action));
                else if (std::holds_alternative<AttackAction> (performing_attack_action.next_action))
                    unit.action = AttackAction (std::get<AttackAction> (performing_attack_action.next_action));
                else if (std::holds_alternative<MoveAction> (performing_attack_action.next_action))
                    unit.action = MoveAction (std::get<MoveAction> (performing_attack_action.next_action));
                else if (std::holds_alternative<CastAction> (performing_attack_action.next_action))
                    unit.action = CastAction (std::get<CastAction> (performing_attack_action.next_action));
                else
                    unit.action = StopAction ();
            }
        } else if (std::holds_alternative<PerformingCastAction> (unit.action)) {
            PerformingCastAction& performing_cast_action = std::get<PerformingCastAction> (unit.action);
            --performing_cast_action.remaining_ticks;
            if (performing_cast_action.remaining_ticks <= 0) {
                switch (performing_cast_action.cast_type) {
                case CastAction::Type::Pestilence:
                    unit.cast_cooldown_left_ticks = MatchState::effectAttackDescription (AttackDescription::Type::PestilenceMissile).cooldown_ticks;
                    break;
                case CastAction::Type::SpawnBeetle:
                    unit.cast_cooldown_left_ticks = MatchState::effectAttackDescription (AttackDescription::Type::SpawnBeetle).cooldown_ticks;
                    break;
                default:
                    break;
                }
                if (std::holds_alternative<StopAction> (performing_cast_action.next_action))
                    unit.action = StopAction (std::get<StopAction> (performing_cast_action.next_action));
                else if (std::holds_alternative<AttackAction> (performing_cast_action.next_action))
                    unit.action = AttackAction (std::get<AttackAction> (performing_cast_action.next_action));
                else if (std::holds_alternative<MoveAction> (performing_cast_action.next_action))
                    unit.action = MoveAction (std::get<MoveAction> (performing_cast_action.next_action));
                else if (std::holds_alternative<CastAction> (performing_cast_action.next_action))
                    unit.action = CastAction (std::get<CastAction> (performing_cast_action.next_action));
                else
                    unit.action = StopAction ();
            }
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
        qreal path_length = max_velocity * dt;
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
            missile.position += displacement * (path_length / displacement_length);
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
    qreal path_length = max_velocity * dt;
    qreal displacement_length = vectorSize (displacement);
    if (displacement_length <= path_length) {
        unit.position = target_position;
        if (clear_action_on_completion)
            unit.action = StopAction ();
    } else {
        unit.position += displacement * (path_length / displacement_length);
    }
}
bool MatchState::applyAttack (Unit& unit, quint32 target_unit_id, Unit& target_unit, qreal dt)
{
    const AttackDescription& attack_description = unitPrimaryAttackDescription (unit.type);
    QPointF displacement = target_unit.position - unit.position;
    qreal target_orientation = vectorRadius (displacement);
    rotateUnit (unit, dt, target_orientation);
    qreal displacement_length = vectorSize (displacement);
    qreal full_attack_range = attack_description.range + unitRadius (unit.type) + unitRadius (target_unit.type);
    bool in_range = false;
    if (displacement_length > full_attack_range) {
        qreal path_length = unitMaxVelocity (unit.type) * dt;
        if (path_length >= displacement_length - full_attack_range) {
            path_length = displacement_length - full_attack_range;
            in_range = true;
        }
        unit.position += displacement * (path_length / displacement_length);
    } else {
        in_range = true;
    }
    if (in_range && orientationsFuzzyMatch (unit.orientation, target_orientation)) {
        switch (attack_description.type) {
        case AttackDescription::Type::SealShot: {
            dealDamage (target_unit, attack_description.damage);
            emit soundEventEmitted (SoundEvent::SealAttack);
        } return true;
        case AttackDescription::Type::CrusaderChop: {
            dealDamage (target_unit, attack_description.damage);
            emit soundEventEmitted (SoundEvent::CrusaderAttack);
        } return true;
        case AttackDescription::Type::GoonRocket: {
            emitMissile (Missile::Type::Rocket, unit, target_unit_id, target_unit);
            emit soundEventEmitted (SoundEvent::RocketStart);
        } return true;
        case AttackDescription::Type::BeetleSlice: {
            dealDamage (target_unit, attack_description.damage);
            emit soundEventEmitted (SoundEvent::BeetleAttack);
        } return true;
        default: {
        }
        }
    }
    return false;
}
bool MatchState::applyCast (Unit& unit, CastAction::Type cast_type, const QPointF& target, qreal dt)
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
            return false;
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
        qreal path_length = unitMaxVelocity (unit.type) * dt;
        if (path_length >= displacement_length - full_attack_range) {
            path_length = displacement_length - full_attack_range;
            in_range = true;
        }
        unit.position += displacement * (path_length / displacement_length);
    } else {
        in_range = true;
    }
    if (!unit.cast_cooldown_left_ticks &&
        !std::holds_alternative<PerformingAttackAction> (unit.action) &&
        !std::holds_alternative<PerformingCastAction> (unit.action) &&
        in_range &&
        orientationsFuzzyMatch (unit.orientation, target_orientation)) {
        switch (cast_type) {
        case CastAction::Type::Pestilence:
            emitMissile (Missile::Type::Pestilence, unit, target);
            emit soundEventEmitted (SoundEvent::PestilenceMissileStart);
            return true;
        case CastAction::Type::SpawnBeetle:
            // createUnit (Unit::Type::Beetle, unit.team, target, unit.orientation);
            emit unitCreateRequested (unit.team, Unit::Type::Beetle, target);
            emit soundEventEmitted (SoundEvent::SpawnBeetle);
            return true;
        default:
            return false;
        }
    }
    return false;
}
void MatchState::applyAreaBoundaryCollision (Unit& unit, qreal dt)
{
    QPointF& position = unit.position;
    qreal unit_radius = unitRadius (unit.type);
    qreal max_velocity = unitMaxVelocity (unit.type) * 2.0;
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
    qreal path_length = max_velocity * dt;
    qreal square_length = QPointF::dotProduct (off, off);
    if (square_length <= path_length * path_length) {
        position += off;
    } else {
        position += off * (path_length / qSqrt (square_length));
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
                    qreal angle = qreal (M_PI * 2.0 * random_generator ()) / (qreal (std::numeric_limits<quint32>::max ()) + 1.0);
                    sincos (angle, &dy, &dx);
                    delta = {dx * min_distance, dy * min_distance};
                } else {
                    qreal distance_to_comfort = min_distance - delta_length;
                    delta *= distance_to_comfort / delta_length;
                }
                off += delta;
            }
        }
        offsets.append (off);
    }
    {
        size_t i = 0;
        for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
            Unit& unit = *it;
            QPointF& position = unit.position;
            QPointF off = offsets[i++];
            qreal max_velocity = unitMaxVelocity (unit.type) * 0.9; // TODO: Make force depend on distance
            qreal path_length = max_velocity * dt;
            qreal length = vectorSize (off);
            position += (length <= path_length) ? off : off * (path_length / length);
        }
    }
}
void MatchState::dealDamage (Unit& unit, qint64 damage)
{
    unit.hp = std::max<qint64> (unit.hp - damage, 0);
}

void MatchState::applyDeath ()
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end ();) {
        if (it->hp <= 0) {
            corpses.emplace (it.key (), *it);
            it = units.erase (it);
        } else {
            ++it;
        }
    }
}
void MatchState::applyDecay ()
{
    for (QHash<quint32, Corpse>::iterator it = corpses.begin (); it != corpses.end ();) {
        if (it->decay_remaining_ticks <= 1) {
            it = corpses.erase (it);
        } else {
            --(it->decay_remaining_ticks);
            ++it;
        }
    }
}
Unit* MatchState::findUnitAt (Unit::Team team, const QPointF& point)
{
    for (QHash<quint32, Unit>::iterator it = units.begin (); it != units.end (); ++it) {
        Unit& unit = *it;
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
QVector<QPair<quint32, const Unit*>> MatchState::buildOrderedSelection ()
{
    static const Unit::Type unit_order[] = {
        Unit::Type::Contaminator,
        Unit::Type::Crusader,
        Unit::Type::Goon,
        Unit::Type::Seal,
        Unit::Type::Beetle,
    };

    QVector<QPair<quint32, const Unit*>> selection;

    // TODO: Use radix algorithm
    for (const Unit::Type unit_type: unit_order)
        for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it)
            if (it->selected && it->type == unit_type)
                selection.append ({it.key (), &*it});

    return selection;
}
