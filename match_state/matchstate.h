#pragma once

#include <QHash>
#include <QPointF>
#include <QRectF>
#include <QObject>
#include <QSharedPointer>
#include <QPair>
#include <QSet>
#include <QVector>
#include <random>
#include <optional>
#include <variant>

#include "attack_description.h"
#include "actions.h"
#include "unit.h"
#include "effects.h"


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
    MatchState (bool is_client);
    ~MatchState ();
    quint64 clockNS () const;
    const QRectF& areaRef () const;
    const QHash<quint32, Unit>& unitsRef () const;
    Unit& addUnit(quint32 id, Unit::Type type, Unit::Team team, const QPointF& position, qreal direction);
    Missile& addMissile(quint32 id, Missile::Type type, Unit::Team team, const QPointF& position, qreal direction); 
    const QHash<quint32, Missile>& missilesRef () const;
    const QHash<quint32, Explosion>& explosionsRef () const;
    QHash<quint32, Unit>::iterator createUnit (Unit::Type type, Unit::Team team, const QPointF& position, qreal direction);
    void trySelect (Unit::Team team, const QPointF& point, bool add);
    void trySelect (Unit::Team team, const QRectF& rect, bool add);
    void trySelectByType (Unit::Team team, const QPointF& point, const QRectF& viewport, bool add);
    void selectAll (Unit::Team team);
    void select (quint32 unit_id, bool add);
    void trimSelection (Unit::Type type, bool remove);
    void deselect (quint32 unit_id);
    void setAction(quint32 unit_id, const std::variant<StopAction, AttackAction, MoveAction, CastAction>& action);
    std::optional<QPointF> selectionCenter () const;
    void attackEnemy (Unit::Team attacker_team, const QPointF& point);
    void cast (CastAction::Type cast_type, Unit::Team attacker_team, const QPointF& point);
    void move (const QPointF& point);
    void stop ();
    void autoAction (Unit::Team attacker_team, const QPointF& point);
    void tick ();
    qreal unitRadius (Unit::Type type) const;
    qreal unitDiameter (Unit::Type type) const;
    qreal missileDiameter (Missile::Type type) const;
    qreal explosionDiameter (Explosion::Type type) const;
    qreal unitMaxVelocity (Unit::Type type) const;
    qreal unitMaxAngularVelocity (Unit::Type type) const;
    int unitHitBarCount (Unit::Type type) const;
    int unitMaxHP (Unit::Type type) const;
    const AttackDescription& unitPrimaryAttackDescription (Unit::Type type) const;
    quint64 animationPeriodNS (Unit::Type type) const;
    void LoadState(const QVector<QPair<quint32, Unit>>& other, QVector<QPair<quint32, Missile>>& other_missiles);
    const AttackDescription& effectAttackDescription (AttackDescription::Type type) const;
    void selectGroup (quint64 group);
    void bindSelectionToGroup (quint64 group);
    void addSelectionToGroup (quint64 group);
    void moveSelectionToGroup (quint64 group, bool add);
    bool fuzzyMatchPoints (const QPointF& p1, const QPointF& p2) const;
    quint32 get_tick_no ();

signals:
    void soundEventEmitted (SoundEvent event);
    //void unitActionRequested (quint32 id, ActionType type, std::variant<QPointF, quint32> target);
    void unitActionRequested (quint32 id, const std::variant<StopAction, MoveAction, AttackAction, CastAction>& action);
    void unitCreateRequested (Unit::Team team, Unit::Type type, const QPointF& position);
private:
    struct RedTeamUserData
    {
    };
    struct BlueTeamUserData
    {
        QSet<quint32> old_missiles;
    };

private:
    void clearSelection ();
    void rotateUnit (Unit& unit, qreal dt, qreal dest_orientation);
    bool checkUnitInsideSelection (const Unit& unit, const QPointF& point) const;
    bool checkUnitInsideSelection (const Unit& unit, const QRectF& rect) const;
    bool checkUnitInsideViewport (const Unit& unit, const QRectF& viewport) const;
    void startAction (const MoveAction& action);
    void startAction (const AttackAction& action);
    void startAction (const CastAction& action);
    void emitMissile (Missile::Type missile_type, const Unit& unit, quint32 target_unit_id, const Unit& target_unit);
    void emitMissile (Missile::Type missile_type, const Unit& unit, const QPointF& target);
    void emitExplosion (Explosion::Type explosion_type, Unit::Team sender_team, const QPointF& position);
    void applyActions (qreal dt);
    void applyEffects (qreal dt);
    void applyMissilesMovement (qreal dt);
    void applyExplosionEffects (qreal dt);
    void applyMovement (Unit& unit, const QPointF& target_position, qreal dt, bool clear_action_on_completion);
    void applyAttack (Unit& unit, quint32 target_unit_id, Unit& target_unit, qreal dt);
    void applyCast (Unit& unit, CastAction::Type cast_type, const QPointF& target, qreal dt);
    void applyAreaBoundaryCollisions (qreal dt);
    void applyAreaBoundaryCollision (Unit& unit, qreal dt);
    void applyUnitCollisions (qreal dt);
    void dealDamage (Unit& unit, qint64 damage);
    void killUnits ();
    Unit* findUnitAt (Unit::Team team, const QPointF& point);
    std::optional<quint32> findClosestTarget (const Unit& unit);
    quint32 getRandomNumber ();
    void redTeamUserTick (RedTeamUserData& user_data);
    void blueTeamUserTick (BlueTeamUserData& user_data);

private:
    const bool is_client;
    quint64 clock_ns = 0;
    QRectF area = {-64, -48, 128, 96};
    QHash<quint32, Unit> units;
    QHash<quint32, Missile> missiles;
    QHash<quint32, Explosion> explosions;
    RedTeamUserData red_team_user_data;
    BlueTeamUserData blue_team_user_data;
    quint32 next_id = 0;
    std::mt19937 random_generator;
    quint32 tick_no = 0;
};
