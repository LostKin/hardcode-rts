#pragma once

#include "matchstate.h"
#include "coordmap.h"
#include "hud.h"
#include "logoverlay.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QOpenGLTexture>

class ColoredRenderer;
class ColoredTexturedRenderer;
class TexturedRenderer;
class SceneRenderer;
class HUDRenderer;


class RoomWidget: public QOpenGLWidget
{
    Q_OBJECT

private:
    enum class UnitRole {
        Unspecified,
        Spectator,
        Player,
    };

    enum class State {
        AwaitingMatch,
        MatchStarted,
    };

    enum class ButtonId {
        None,
        JoinRedTeam,
        JoinBlueTeam,
        Spectate,
        Ready,
        Cancel,
        Quit,
    };

public:
    RoomWidget (QWidget* parent = nullptr);
    virtual ~RoomWidget ();
    void awaitMatch (Unit::Team team);
    void startMatch (Unit::Team team);
#ifdef LOG_OVERLAY
    inline void log (const QString& message)
    {
        log_overlay.log (message);
    }
#else
    inline void log (const QString& /* message */)
    {
    }
#endif

signals:
    void selectRolePlayerRequested ();
    void spectateRequested ();
    void cancelJoinTeamRequested ();
    void readinessRequested ();
    void quitRequested ();
    void createUnitRequested (Unit::Team team, Unit::Type type, const Position& position);
    void unitActionRequested (quint32 id, const UnitActionVariant& action);

private slots:
    void quitRequestedHandler ();

public slots:
    void startMatchHandler ();
    void startCountDownHandler (Unit::Team team);
    void loadMatchState (const std::vector<std::pair<quint32, Unit>>& units, const std::vector<std::pair<quint32, Corpse>>& corpses, const std::vector<std::pair<quint32, Missile>>& missiles);
    // void unitActionCallback (quint32 id, ActionType type, std::variant<QPointF, quint32> target);

    void unitActionCallback (quint32 id, const UnitActionVariant& action);
    void unitCreateCallback (Unit::Team team, Unit::Type type, const Position& position);

protected:
    virtual void initializeGL () override;
    virtual void resizeGL (int w, int h) override;
    virtual void paintGL () override;
    virtual void keyPressEvent (QKeyEvent* event) override;
    virtual void keyReleaseEvent (QKeyEvent* event) override;
    virtual void mouseMoveEvent (QMouseEvent* event) override;
    virtual void mousePressEvent (QMouseEvent* event) override;
    virtual void mouseReleaseEvent (QMouseEvent* event) override;
    virtual void wheelEvent (QWheelEvent* event) override;

private:
    QSize pixelsSize ();
    void initResources ();
    void updateSize (int w, int h);
    void draw ();
    QSharedPointer<QOpenGLTexture> loadTexture2DRectangle (const QString& path);
    static quint64 moveAnimationPeriodNS (Unit::Type type);
    static quint64 attackAnimationPeriodNS (Unit::Type type);
    static quint64 missileAnimationPeriodNS (Missile::Type type);
    void matchKeyPressEvent (QKeyEvent* event);
    void matchKeyReleaseEvent (QKeyEvent* event);
    void matchMouseMoveEvent (const QPoint& previous_cursor_position, QMouseEvent* event);
    void matchMousePressEvent (QMouseEvent* event);
    void matchMouseReleaseEvent (QMouseEvent* event);
    void matchWheelEvent (QWheelEvent* event);
    void loadTextures ();
    void drawMatch ();
    void drawMatchCursor ();
    void drawCountdownOverlay ();
    void drawLog ();
    void frameUpdate (qreal dt);
    void matchFrameUpdate (qreal dt);
    bool pointInsideButton (const QPoint& point, const QPoint& button_pos, QSharedPointer<QOpenGLTexture>& texture) const;
    bool getActionButtonUnderCursor (const QPoint& cursor_pos, int& row, int& col) const;
    bool getSelectionPanelUnitUnderCursor (const QPoint& cursor_pos, int& row, int& col) const;
    bool getMinimapPositionUnderCursor (const QPoint& cursor_pos, Position& area_pos) const;
    Position getMinimapPositionFromCursor (const QPoint& cursor_pos) const;
    bool cursorIsAboveScene (const QPoint& cursor_pos) const;
    void centerViewportAt (const Position& point);
    void centerViewportAtSelected ();
    void selectGroup (quint64 group);
    void bindSelectionToGroup (quint64 group);
    void groupEvent (quint64 group_num);
    void zoom (int delta);

private slots:
    void tick ();
    void playSound (SoundEvent event);

private:
    static ActionButtonId getActionButtonFromGrid (int row, int col);

private:
    struct {
        struct {
            QSharedPointer<QOpenGLTexture> crosshair;
            QSharedPointer<QOpenGLTexture> friend_selection;
            QSharedPointer<QOpenGLTexture> enemy_selection;
            QSharedPointer<QOpenGLTexture> primary_selection;
            QSharedPointer<QOpenGLTexture> rectangle_selection;
            QSharedPointer<QOpenGLTexture> attack;
            QSharedPointer<QOpenGLTexture> attack_enemy;
        } cursors;
    } textures;

    int pixels_w = 1;
    int pixels_h = 1;
    QOpenGLFunctions gl;
    QMatrix4x4 ortho_matrix; // TODO: Fix this
    HUD hud;
    bool starting_countdown = true;
    ButtonId pressed_button = ButtonId::None;
    QPoint cursor_position = {-1, -1};
    bool minimap_viewport_selection_pressed = false;
    Unit::Team team;
    QElapsedTimer match_countdown_start;
    QSharedPointer<QOpenGLTexture> cursor;
    MatchState match_state;
    int mouse_scroll_border = 10;
    QTimer match_timer;
    QElapsedTimer last_frame;
    std::optional<QPoint> selection_start;
    bool camera_move_modifier_pressed = false;
    bool ctrl_pressed = false;
    bool alt_pressed = false;
    bool shift_pressed = false;
    std::mt19937 random_generator;
    CoordMap coord_map;
    QColor red_player_color = QColor (0xf1, 0x4b, 0x2c);
    QColor blue_player_color = QColor (0x48, 0xc0, 0xbb);
    QSharedPointer<ColoredRenderer> colored_renderer;
    QSharedPointer<ColoredTexturedRenderer> colored_textured_renderer;
    QSharedPointer<TexturedRenderer> textured_renderer;
    QSharedPointer<SceneRenderer> scene_renderer;
    QSharedPointer<HUDRenderer> hud_renderer;
    QFont countdown_font;
#ifdef LOG_OVERLAY
    LogOverlay log_overlay;
#endif
};
