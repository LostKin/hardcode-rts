#pragma once

#include <QtGlobal>

struct AttackDescription {
    enum class Type {
        Unknown = 0,
        SealShot,
        CrusaderChop,
        GoonRocket,
        GoonRocketExplosion,
        BeetleSlice,
        PestilenceMissile,
        PestilenceSplash,
        SpawnBeetle,
    };

    Type type = Type::Unknown;
    qreal range = 0.0;
    qreal trigger_range = 0.0;
    qreal missile_velocity = 0.0;
    qint64 damage = 0;
    qint64 duration_ticks = 0;
    bool friendly_fire = true;
    qint64 cooldown_ticks = 0;
};
