#pragma once

#include <QRect>
#include <QColor>
#include <QSharedPointer>

class QOpenGLFunctions;
class QOpenGLTexture;
class ColoredRenderer;
class TexturedRenderer;
class HUD;
class QMatrix4x4;
class CoordMap;


class ActionPanelRenderer
{
public:
    ActionPanelRenderer ();
    void draw (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, TexturedRenderer& textured_renderer,
               HUD& hud, int margin, const QColor& panel_color, int selected_count, quint64 active_actions, bool contaminator_selected, qint64 cast_cooldown_left_ticks,
               const QMatrix4x4& ortho_matrix, const CoordMap& coord_map);

private:
    void drawActionButton (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer, TexturedRenderer& textured_renderer,
                           const QRect& rect, bool pressed, QOpenGLTexture* texture, const QMatrix4x4& ortho_matrix);
    void drawActionButtonShade (QOpenGLFunctions& gl, ColoredRenderer& colored_renderer,
                                const QRect& rect, bool pressed, qreal remaining, const QMatrix4x4& ortho_matrix);
    QSharedPointer<QOpenGLTexture> loadTexture2D (const QString& path);

private:
    struct {
        struct {
            QSharedPointer<QOpenGLTexture> move;
            QSharedPointer<QOpenGLTexture> stop;
            QSharedPointer<QOpenGLTexture> hold;
            QSharedPointer<QOpenGLTexture> attack;
            QSharedPointer<QOpenGLTexture> pestilence;
            QSharedPointer<QOpenGLTexture> spawn;
        } basic;
        struct {
            QSharedPointer<QOpenGLTexture> move;
            QSharedPointer<QOpenGLTexture> stop;
            QSharedPointer<QOpenGLTexture> hold;
            QSharedPointer<QOpenGLTexture> attack;
            QSharedPointer<QOpenGLTexture> pestilence;
            QSharedPointer<QOpenGLTexture> spawn;
        } active;
    } textures;
};
