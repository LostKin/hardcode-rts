#pragma once

#include <QHash>
#include <QPointF>
#include <QRectF>
#include <QObject>
#include <QSharedPointer>
#include <QPair>
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
    const QHash<quint32, Missile>& missilesRef () const;
    const QHash<quint32, Explosion>& explosionsRef () const;
    QHash<quint32, Unit>::iterator createUnit (Unit::Type type, Unit::Team team, const QPointF& position, qreal direction);
    void trySelect (Unit::Team team, const QPointF& point, bool add);
    void trySelect (Unit::Team team, const QRectF& rect, bool add);
    void trySelectByType (Unit::Team team, const QPointF& point, const QRectF& viewport, bool add);
    void selectAll (Unit::Team team);
    void select(quint32 unit_id, bool add); // DELETE
    void setAction(quint32 unit_id, MoveAction action);
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
    void LoadState(const QVector<QPair<quint32, Unit> >& other, const QVector<QPair<quint32, quint32> >& to_delete);
    const AttackDescription& effectAttackDescription (AttackDescription::Type type) const;
    void selectGroup (quint64 group);
    void bindSelectionToGroup (quint64 group);
    void addSelectionToGroup (quint64 group);
    void moveSelectionToGroup (quint64 group, bool add);
    bool fuzzyMatchPoints (const QPointF& p1, const QPointF& p2) const;

signals:
    void soundEventEmitted (SoundEvent event);
    void unitActionRequested (quint32 id, ActionType type, std::variant<QPointF, quint32> target);

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
    Unit* findUnitAt (Unit::Team team, const QPointF& point);
    quint32 getRandomNumber ();

private:
    const bool is_client;
    quint64 clock_ns = 0;
    QRectF area = {-64, -32, 128, 64};
    QHash<quint32, Unit> units;
    QHash<quint32, Missile> missiles;
    QHash<quint32, Explosion> explosions;
    quint32 next_id = 0;
    std::mt19937 random_generator;
};
