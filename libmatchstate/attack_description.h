#pragma once


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
    double range = 0.0;
    double trigger_range = 0.0;
    double missile_velocity = 0.0;
    int64_t damage = 0;
    int64_t duration_ticks = 0;
    bool friendly_fire = true;
    int64_t cooldown_ticks = 0;
};
