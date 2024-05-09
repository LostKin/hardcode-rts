#pragma once

#include <map>
#include <set>
#include <vector>
#include <QObject>
#include <random>
#include <optional>
#include <variant>

#include "attack_description.h"
#include "actions.h"
#include "unit.h"
#include "corpse.h"
#include "effects.h"
#include "position.h"
#include "offset.h"
#include "rectangle.h"


enum class SoundEvent {
    SealAttack,
    CrusaderAttack,
    RocketStart,
    RocketExplosion,
    PestilenceSplash,
    BeetleAttack,
    PestilenceMissileStart,
    SpawnBeetle,
};

enum class ActionType {
    Movement,
    Attack,
    Cast,
};


class MatchState: public QObject
{
    Q_OBJECT

public:
    // TODO: Refine it into property set
    static double unitRadius (Unit::Type type);
    static double unitDiameter (Unit::Type type);
    static double missileDiameter (Missile::Type type);
    static double explosionDiameter (Explosion::Type type);
    static double unitMaxVelocity (Unit::Type type);
    static double unitMaxAngularVelocity (Unit::Type type);
    static int unitHitBarCount (Unit::Type type);
    static int unitMaxHP (Unit::Type type);
    static const AttackDescription& unitPrimaryAttackDescription (Unit::Type type);
    static uint64_t animationPeriodNS (Unit::Type type);
    static const AttackDescription& effectAttackDescription (AttackDescription::Type type);

public:
    MatchState (bool is_client);
    ~MatchState ();
    uint64_t clockNS () const;
    const Rectangle& areaRef () const;
    const std::map<uint32_t, Unit>& unitsRef () const;
    const std::map<uint32_t, Corpse>& corpsesRef () const;
    Unit& addUnit (uint32_t id, Unit::Type type, Unit::Team team, const Position& position, double direction);
    Corpse& addCorpse (uint32_t id, Unit::Type type, Unit::Team team, const Position& position, double direction, int64_t decay_remaining_ticks);
    Missile& addMissile (uint32_t id, Missile::Type type, Unit::Team team, const Position& position, double direction);
    const std::map<uint32_t, Missile>& missilesRef () const;
    const std::map<uint32_t, Explosion>& explosionsRef () const;
    std::map<uint32_t, Unit>::iterator createUnit (Unit::Type type, Unit::Team team, const Position& position, double direction);
    void trySelect (Unit::Team team, const Position& point, bool add);
    void trySelect (Unit::Team team, const Rectangle& rect, bool add);
    void trySelectByType (Unit::Team team, const Position& point, const Rectangle& viewport, bool add);
    void selectAll (Unit::Team team);
    void select (uint32_t unit_id, bool add);
    void trimSelection (Unit::Type type, bool remove);
    void deselect (uint32_t unit_id);
    void setUnitAction (uint32_t unit_id, const UnitActionVariant& action);
    std::optional<Position> selectionCenter () const;
    void attackEnemy (Unit::Team attacker_team, const Position& point);
    void cast (CastAction::Type cast_type, Unit::Team attacker_team, const Position& point); // TODO: Cast with the closest caster
    void move (const Position& point);
    void stop ();
    void autoAction (Unit::Team attacker_team, const Position& point);
    void tick ();
    void LoadState (const std::vector<std::pair<uint32_t, Unit>>& units, const std::vector<std::pair<uint32_t, Corpse>>& corpses, const std::vector<std::pair<uint32_t, Missile>>& missiles);
    void selectGroup (uint64_t group);
    void bindSelectionToGroup (uint64_t group);
    void addSelectionToGroup (uint64_t group);
    void moveSelectionToGroup (uint64_t group, bool add);
    bool fuzzyMatchPoints (const Position& p1, const Position& p2) const; // TODO: Points -> Positions
    uint32_t get_tick_no ();
    std::vector<std::pair<uint32_t, const Unit*>> buildOrderedSelection ();

signals:
    void soundEventEmitted (SoundEvent event);
    void unitActionRequested (uint32_t id, const UnitActionVariant& action);
    void unitCreateRequested (Unit::Team team, Unit::Type type, const Position& position);

private:
    struct RedTeamUserData {
    };
    struct BlueTeamUserData {
        std::set<uint32_t> old_missiles;
    };

private:
    void loadUnits (const std::vector<std::pair<uint32_t, Unit>>& units);
    void loadCorpses (const std::vector<std::pair<uint32_t, Corpse>>& corpses);
    void loadMissiles (const std::vector<std::pair<uint32_t, Missile>>& missiles);
    void clearSelection ();
    void rotateUnit (Unit& unit, double dt, double dest_orientation);
    bool checkUnitInsideSelection (const Unit& unit, const Position& point) const;
    bool checkUnitInsideSelection (const Unit& unit, const Rectangle& rect) const;
    bool checkUnitInsideViewport (const Unit& unit, const Rectangle& viewport) const;
    void startAction (const MoveAction& action);
    void startAction (const AttackAction& action);
    void startAction (const CastAction& action);
    void emitMissile (Missile::Type missile_type, const Unit& unit, uint32_t target_unit_id, const Unit& target_unit);
    void emitMissile (Missile::Type missile_type, const Unit& unit, const Position& target);
    void emitExplosion (Explosion::Type explosion_type, Unit::Team sender_team, const Position& position);
    void applyActions (double dt);
    void applyEffects (double dt);
    void applyMissilesMovement (double dt);
    void applyExplosionEffects (double dt);
    void applyMovement (Unit& unit, const Position& target_position, double dt, bool clear_action_on_completion);
    bool applyAttack (Unit& unit, uint32_t target_unit_id, Unit& target_unit, double dt);
    bool applyCast (Unit& unit, CastAction::Type cast_type, const Position& target, double dt);
    void applyAreaBoundaryCollisions (double dt);
    void applyAreaBoundaryCollision (Unit& unit, double dt);
    void applyUnitCollisions (double dt);
    void dealDamage (Unit& unit, int64_t damage);
    void applyDeath ();
    void applyDecay ();
    Unit* findUnitAt (Unit::Team team, const Position& point);
    std::optional<uint32_t> findClosestTarget (const Unit& unit);
    uint32_t getRandomNumber ();
    void redTeamUserTick (RedTeamUserData& user_data);
    void blueTeamUserTick (BlueTeamUserData& user_data);

private:
    const bool is_client;
    uint32_t tick_no = 0;
    uint64_t clock_ns = 0;
    Rectangle area = Rectangle (-64, 64, -48, 48);
    std::map<uint32_t, Unit> units;
    std::map<uint32_t, Corpse> corpses;
    std::map<uint32_t, Missile> missiles;
    std::map<uint32_t, Explosion> explosions;
    RedTeamUserData red_team_user_data;
    BlueTeamUserData blue_team_user_data;
    uint32_t next_id = 0;
    std::mt19937 random_generator;
};
