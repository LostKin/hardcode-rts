#pragma once

#include <QHash>
#include <QPointF>
#include <QRectF>
#include <optional>

class Unit
{
public:
    enum class Type {
        Seal,
        Crusader,
    };

    enum class Team {
        Neutral,
        Red,
        Blue,
    };

public:
    Unit (Type type, Team team, const QPointF& position, qreal direction);
    ~Unit ();

public:
    Type type;
    Team team;
    QPointF position;
    qreal orientation = 0.0;
    bool selected = false;
    std::optional<QPointF> move_target;
};

class MatchState
{
public:
    MatchState ();
    ~MatchState ();
    const QRectF& areaRef () const;
    const QHash<uint32_t, Unit>& unitsRef () const;
    void addUnit (const Unit& unit);
    void trySelect (const QPointF& point, bool add = false);
    void trySelect (const QRectF& point, bool add = false);
    void move (const QPointF& point);
    void stop ();
    void autoAction (const QPointF& point);
    void tick ();

private:
    QRectF area = {-200, -200, 400, 400};
    QHash<uint32_t, Unit> units;
    uint32_t next_id = 0;
};
