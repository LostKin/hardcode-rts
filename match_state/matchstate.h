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


enum class SoundEvent {
    Shot,
};

class MatchState: public QObject
{
    Q_OBJECT

public:
    MatchState ();
    ~MatchState ();
    quint64 clockNS () const;
    const QRectF& areaRef () const;
    const QHash<quint32, Unit>& unitsRef () const;
    QHash<quint32, Unit>::iterator createUnit (Unit::Type type, Unit::Team team, const QPointF& position, qreal direction);
    Unit& addUnit(quint32 id, Unit::Type type, Unit::Team team, const QPointF& position, qreal direction);
    void trySelect (Unit::Team team, const QPointF& point, bool add);
    void trySelect (Unit::Team team, const QRectF& rect, bool add);
    std::optional<QPointF> selectionCenter () const;
    void attackEnemy (Unit::Team attacker_team, const QPointF& point);
    void move (const QPointF& point);
    void stop ();
    void autoAction (Unit::Team attacker_team, const QPointF& point);
    void tick ();
    qreal unitRadius (Unit::Type type) const;
    qreal unitDiameter (Unit::Type type) const;
    qreal unitMaxVelocity (Unit::Type type) const;
    qreal unitMaxAngularVelocity (Unit::Type type) const;
    int unitHitBarCount (Unit::Type type) const;
    int unitMaxHP (Unit::Type type) const;
    const AttackDescription& unitPrimaryAttackDescription (Unit::Type type) const;
    quint64 animationPeriodNS (Unit::Type type) const;
    void LoadState(const QVector<QPair<quint32, Unit> >& other, const QVector<QPair<quint32, quint32> >& to_delete);

signals:
    void soundEventEmitted (SoundEvent event);

private:
    static bool intersectRectangleCircle (const QRectF& rect, const QPointF& center, qreal radius);
    static qreal normalizeOrientation (qreal orientation);

private:
    void clearSelection ();
    void rotateUnit (Unit& unit, qreal dt, qreal dest_orientation);
    bool checkUnitSelection (const Unit& unit, const QPointF& point) const;
    bool checkUnitSelection (const Unit& unit, const QRectF& rect) const;
    void applyAction (const MoveAction& action);
    void applyAction (const AttackAction& action);

private:
    quint64 clock_ns = 0;
    QRectF area = {-64, -32, 128, 64};
    QHash<quint32, Unit> units;
    quint32 next_id = 0;
    std::mt19937 random_generator;
};
