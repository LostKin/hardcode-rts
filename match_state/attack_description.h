#pragma once

#include <QtGlobal>


struct AttackDescription
{
    enum class Type {
        Unknown = 0,
        Immediate,
        Missile,
        Splash,
    };

    Type type = Type::Unknown;
    qreal range = 0.0;
    int damage = 10;
    qreal splash_radius = 0.0;
    int duration_ticks = 0;
};
