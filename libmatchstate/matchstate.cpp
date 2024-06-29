#include "matchstate.h"

#include "positionaverage.h"


MatchState::MatchState ()
{
    initNodeTrees ();
}
MatchState::~MatchState ()
{
}

uint64_t MatchState::clockNS () const
{
    return clock_ns;
}
const Rectangle& MatchState::areaRef () const
{
    return area;
}
const std::map<uint32_t, Unit>& MatchState::unitsRef () const
{
    return units;
}
const std::map<uint32_t, Corpse>& MatchState::corpsesRef () const
{
    return corpses;
}
const std::map<uint32_t, Missile>& MatchState::missilesRef () const
{
    return missiles;
}
const std::map<uint32_t, Explosion>& MatchState::explosionsRef () const
{
    return explosions;
}

std::optional<Position> MatchState::selectionCenter () const
{
    PositionAverage average;
    for (std::map<uint32_t, Unit>::const_iterator it = units.cbegin (); it != units.cend (); ++it) {
        const Unit& unit = it->second;
        if (unit.selected)
            average.add (unit.position);
    }
    return average.get ();
}
uint32_t MatchState::getTickNo () const
{
    return tick_no;
}
double MatchState::unitDiameter (Unit::Type type)
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
double MatchState::missileDiameter (Missile::Type type)
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
double MatchState::explosionDiameter (Explosion::Type type)
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
double MatchState::unitRadius (Unit::Type type)
{
    return unitDiameter (type) * 0.5;
}
double MatchState::unitMaxVelocity (Unit::Type type)
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
double MatchState::unitMaxAngularVelocity (Unit::Type type)
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
int64_t MatchState::beetleTTLTicks ()
{
    return 700;
}
int64_t MatchState::pestilenceDiseaseDurationTicks ()
{
    return 250;
}
int64_t MatchState::pestilenceDamagePeriodTicks ()
{
    return 20;
}
int64_t MatchState::pestilenceDamagePerPeriod ()
{
    return 1;
}
double MatchState::pestilenceDiseaseSlowdownFactor ()
{
    return 0.3;
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
bool MatchState::fuzzyMatchPoints (const Position& p1, const Position& p2) const
{
    const double ratio = 0.5;
    return (p2 - p1).length () < unitDiameter (Unit::Type::Beetle) * ratio;
}


std::vector<std::pair<uint32_t, const Unit*>> MatchState::buildOrderedSelection () const
{
    static const Unit::Type unit_order[] = {
        Unit::Type::Contaminator,
        Unit::Type::Crusader,
        Unit::Type::Goon,
        Unit::Type::Seal,
        Unit::Type::Beetle,
    };

    std::vector<std::pair<uint32_t, const Unit*>> selection;

    // TODO: Use radix algorithm
    for (const Unit::Type unit_type: unit_order)
        for (std::map<uint32_t, Unit>::const_iterator it = units.cbegin (); it != units.cend (); ++it) {
            const Unit& unit = it->second;
            if (unit.selected && unit.type == unit_type)
                selection.push_back ({it->first, &unit});
        }

    return selection;
}
