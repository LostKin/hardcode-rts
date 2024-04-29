#pragma once

#include "matchstate.h"

#include <QSharedPointer>

class QOpenGLTexture;
class QOpenGLFunctions;
class ColoredTexturedRenderer;
class TexturedRenderer;
class CoordMap;


class EffectRenderer
{
public:
    EffectRenderer ();
    void drawExplosion (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer, const Explosion& explosion, quint64 clock_ns,
                        const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);
    void drawMissile (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Missile& missile, quint64 clock_ns,
                      const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    static QSharedPointer<QOpenGLTexture> loadTexture2D (const QString& path);
    static quint64 explosionAnimationPeriodNS ();
    static quint64 missileAnimationPeriodNS (Missile::Type type);

private:
    struct {
        struct {
            QSharedPointer<QOpenGLTexture> explosion1;
            QSharedPointer<QOpenGLTexture> explosion2;
        } explosion;
        struct {
            QSharedPointer<QOpenGLTexture> rocket1;
            QSharedPointer<QOpenGLTexture> rocket2;
        } goon_rocket;
        struct {
            QSharedPointer<QOpenGLTexture> missile1;
            QSharedPointer<QOpenGLTexture> missile2;
        } pestilence_missile;
        struct {
            QSharedPointer<QOpenGLTexture> splash;
        } pestilence_splash;
    } textures;
};
