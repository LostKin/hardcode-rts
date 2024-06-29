#include "effectrenderer.h"

#include "coordmap.h"
#include "coloredtexturedrenderer.h"
#include "texturedrenderer.h"

#include <QOpenGLTexture>


static constexpr qreal SQRT_2 = 1.4142135623731;
static constexpr qreal PI_X_3_4 = 3.0 / 4.0 * M_PI;
static constexpr qreal PI_X_1_4 = 1.0 / 4.0 * M_PI;


EffectRenderer::EffectRenderer ()
{
    textures.explosion.explosion1 = loadTexture2D (":/images/effects/explosion/explosion1.png");
    textures.explosion.explosion2 = loadTexture2D (":/images/effects/explosion/explosion2.png");
    textures.goon_rocket.rocket1 = loadTexture2D (":/images/effects/goon-rocket/rocket1.png");
    textures.goon_rocket.rocket2 = loadTexture2D (":/images/effects/goon-rocket/rocket2.png");
    textures.pestilence_missile.missile1 = loadTexture2D (":/images/effects/pestilence-missile/missile1.png");
    textures.pestilence_missile.missile2 = loadTexture2D (":/images/effects/pestilence-missile/missile2.png");
    textures.pestilence_splash.splash = loadTexture2D (":/images/effects/pestilence-splash/splash.png");
}

void EffectRenderer::drawExplosion (QOpenGLFunctions& gl, ColoredTexturedRenderer& colored_textured_renderer, const Explosion& explosion, quint64 clock_ns,
                                    const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    const AttackDescription& attack_description = MatchState::effectAttackDescription (AttackDescription::Type::GoonRocketExplosion);
    qreal sprite_scale;
    switch (explosion.type) {
    case Explosion::Type::Fire:
        sprite_scale = 0.85;
        break;
    case Explosion::Type::Pestilence:
        sprite_scale = 0.6;
        break;
    default:
        return;
    }

    qreal orientation = 0.0;
    GLfloat alpha = explosion.remaining_ticks * 0.5 / attack_description.duration_ticks;

    quint64 period = explosionAnimationPeriodNS ();
    quint64 phase = clock_ns % period;
    QOpenGLTexture* texture;
    switch (explosion.type) {
    case Explosion::Type::Fire:
        texture = (phase < period / 2) ? textures.explosion.explosion1.get () : textures.explosion.explosion2.get ();
        break;
    case Explosion::Type::Pestilence:
        texture = textures.pestilence_splash.splash.get ();
        break;
    default:
        return;
    }

    Position center = coord_map.toScreenCoords (explosion.position);

    qreal a1_sin, a1_cos;
    qreal a2_sin, a2_cos;
    qreal a3_sin, a3_cos;
    qreal a4_sin, a4_cos;
    sincos (orientation + PI_X_3_4, &a1_sin, &a1_cos);
    sincos (orientation + PI_X_1_4, &a2_sin, &a2_cos);
    sincos (orientation - PI_X_1_4, &a3_sin, &a3_cos);
    sincos (orientation - PI_X_3_4, &a4_sin, &a4_cos);
    qreal scale = coord_map.viewport_scale * sprite_scale * MatchState::explosionDiameter (explosion.type) * SQRT_2 * coord_map.arena_viewport.height () / coord_map.POINTS_PER_VIEWPORT_VERTICALLY;

    const GLfloat colors[] = {
        1,
        1,
        1,
        alpha,
        1,
        1,
        1,
        alpha,
        1,
        1,
        1,
        alpha,
        1,
        1,
        1,
        alpha,
    };

    const GLfloat vertices[] = {
        GLfloat (center.x () + scale * a1_cos),
        GLfloat (center.y () + scale * a1_sin),
        GLfloat (center.x () + scale * a2_cos),
        GLfloat (center.y () + scale * a2_sin),
        GLfloat (center.x () + scale * a3_cos),
        GLfloat (center.y () + scale * a3_sin),
        GLfloat (center.x () + scale * a4_cos),
        GLfloat (center.y () + scale * a4_sin),
    };

    static const GLfloat texture_coords[] = {
        0,
        1,
        1,
        1,
        1,
        0,
        0,
        0,
    };

    static const GLuint indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    colored_textured_renderer.draw (gl, GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, texture, ortho_matrix);
}
void EffectRenderer::drawMissile (QOpenGLFunctions& gl, TexturedRenderer& textured_renderer, const Missile& missile, quint64 clock_ns,
                                  const QMatrix4x4& ortho_matrix, const CoordMap& coord_map)
{
    qreal sprite_scale = 0.5;

    quint64 period;
    quint64 phase;
    QOpenGLTexture* texture;
    switch (missile.type) {
    case Missile::Type::Rocket:
        period = missileAnimationPeriodNS (Missile::Type::Rocket);
        phase = clock_ns % period;
        texture = (phase < period / 2) ? textures.goon_rocket.rocket1.get () : textures.goon_rocket.rocket2.get ();
        break;
    case Missile::Type::Pestilence:
        period = missileAnimationPeriodNS (Missile::Type::Pestilence);
        phase = clock_ns % period;
        texture = (phase < period / 2) ? textures.pestilence_missile.missile1.get () : textures.pestilence_missile.missile2.get ();
        break;
    default:
        return;
    }

    Position center = coord_map.toScreenCoords (missile.position);

    qreal a1_sin, a1_cos;
    qreal a2_sin, a2_cos;
    qreal a3_sin, a3_cos;
    qreal a4_sin, a4_cos;
    sincos (missile.orientation + PI_X_3_4, &a1_sin, &a1_cos);
    sincos (missile.orientation + PI_X_1_4, &a2_sin, &a2_cos);
    sincos (missile.orientation - PI_X_1_4, &a3_sin, &a3_cos);
    sincos (missile.orientation - PI_X_3_4, &a4_sin, &a4_cos);
    qreal scale = coord_map.viewport_scale * sprite_scale * MatchState::missileDiameter (Missile::Type::Rocket) * SQRT_2 * coord_map.arena_viewport.height () / coord_map.POINTS_PER_VIEWPORT_VERTICALLY;

    const GLfloat vertices[] = {
        GLfloat (center.x () + scale * a1_cos),
        GLfloat (center.y () + scale * a1_sin),
        GLfloat (center.x () + scale * a2_cos),
        GLfloat (center.y () + scale * a2_sin),
        GLfloat (center.x () + scale * a3_cos),
        GLfloat (center.y () + scale * a3_sin),
        GLfloat (center.x () + scale * a4_cos),
        GLfloat (center.y () + scale * a4_sin),
    };

    static const GLfloat texture_coords[] = {
        0,
        1,
        1,
        1,
        1,
        0,
        0,
        0,
    };

    static const GLuint indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    textured_renderer.draw (gl, GL_TRIANGLES, vertices, texture_coords, 6, indices, texture, ortho_matrix);
}
QSharedPointer<QOpenGLTexture> EffectRenderer::loadTexture2D (const QString& path)
{
    QOpenGLTexture* texture = new QOpenGLTexture (QImage (path));
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
quint64 EffectRenderer::explosionAnimationPeriodNS ()
{
    return 400'000'000;
}
quint64 EffectRenderer::missileAnimationPeriodNS (Missile::Type type)
{
    switch (type) {
    case Missile::Type::Rocket:
        return 200'000'000;
    case Missile::Type::Pestilence:
        return 100'000'000;
    default:
        return 0;
    }
}
