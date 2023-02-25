#pragma once

#include "openglwidget.h"

#include "matchstate.h"

#include <QElapsedTimer>
#include <QTimer>

class Unit;

class RoomWidget: public OpenGLWidget
{
    Q_OBJECT

public:
    struct UnitTextureSet {
        struct {
            QSharedPointer<QOpenGLTexture> standing;
            QSharedPointer<QOpenGLTexture> walking1;
            QSharedPointer<QOpenGLTexture> walking2;
            QSharedPointer<QOpenGLTexture> shooting1;
            QSharedPointer<QOpenGLTexture> shooting2;
        } basic;
        struct {
            QSharedPointer<QOpenGLTexture> plain;
            QSharedPointer<QOpenGLTexture> selected;
        } red;
        struct {
            QSharedPointer<QOpenGLTexture> plain;
            QSharedPointer<QOpenGLTexture> selected;
        } blue;
    };

public:
    RoomWidget (QWidget* parent = nullptr);
    virtual ~RoomWidget ();
    void awaitTeamSelection (Unit::Team team);
    void queryReadiness (Unit::Team team);
    void ready (Unit::Team team);
    void awaitMatch (Unit::Team team);
    void startMatch (Unit::Team team);

signals:
    void joinRedTeamRequested ();
    void joinBlueTeamRequested ();
    void spectateRequested ();
    void cancelJoinTeamRequested ();
    void readinessRequested ();
    void quitRequested ();

private slots:
    void joinRedTeamRequestedHandler ();
    void joinBlueTeamRequestedHandler ();
    void spectateRequestedHandler ();
    void readinessRequestedHandler ();
    void cancelJoinTeamRequestedHandler ();
    void quitRequestedHandler ();

public slots:
    void readinessHandler ();
    void startMatchHandler();
    void startCountDownHandler ();

protected:
    virtual void initResources () override;
    virtual void updateSize (int w, int h) override;
    virtual void draw () override;
    virtual void keyPressEvent (QKeyEvent *event) override;
    virtual void keyReleaseEvent (QKeyEvent *event) override;
    virtual void mouseMoveEvent (QMouseEvent *event) override;
    virtual void mousePressEvent (QMouseEvent *event) override;
    virtual void mouseReleaseEvent (QMouseEvent *event) override;
    virtual void wheelEvent (QWheelEvent *event) override;

private:
    void loadTextures ();
    void drawTeamSelection ();
    void drawTeamSelectionRequested ();
    void drawConfirmingReadiness ();
    void drawReady ();
    void drawAwaitingMatch ();
    void drawMatchStarted ();
    void frameUpdate (qreal dt);
    void matchFrameUpdate (qreal dt);
    QPointF toMapCoords (const QPointF& point);
    QPointF toScreenCoords (const QPointF& point);
    bool pointInsideButton (const QPoint& point, const QPoint& button_pos, QSharedPointer<QOpenGLTexture>& texture);
    void centerViewportAtSelected ();
    void drawHUD ();
    void drawUnit (const Unit& unit);
    void drawUnitPathToTarget (const Unit& unit);

private slots:
    void tick ();
    void playSound (SoundEvent event);

private:
    enum class State {
        TeamSelection,
        TeamSelectionRequested,
        ConfirmingReadiness,
        Ready,
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

private:
    QFont font;

    struct {
        QSharedPointer<QOpenGLTexture> grass;
        QSharedPointer<QOpenGLTexture> ground;
        struct {
            QSharedPointer<QOpenGLTexture> crosshair;
        } cursors;
        struct {
            QSharedPointer<QOpenGLTexture> join_team_requested;
            QSharedPointer<QOpenGLTexture> ready;
            QSharedPointer<QOpenGLTexture> starting_in_0;
            QSharedPointer<QOpenGLTexture> starting_in_1;
            QSharedPointer<QOpenGLTexture> starting_in_2;
            QSharedPointer<QOpenGLTexture> starting_in_3;
            QSharedPointer<QOpenGLTexture> starting_in_4;
            QSharedPointer<QOpenGLTexture> starting_in_5;
        } labels;
        struct {
            QSharedPointer<QOpenGLTexture> join_red_team;
            QSharedPointer<QOpenGLTexture> join_blue_team;
            QSharedPointer<QOpenGLTexture> spectate;
            QSharedPointer<QOpenGLTexture> ready;
            QSharedPointer<QOpenGLTexture> cancel;
            QSharedPointer<QOpenGLTexture> quit;
            QSharedPointer<QOpenGLTexture> join_red_team_pressed;
            QSharedPointer<QOpenGLTexture> join_blue_team_pressed;
            QSharedPointer<QOpenGLTexture> spectate_pressed;
            QSharedPointer<QOpenGLTexture> ready_pressed;
            QSharedPointer<QOpenGLTexture> cancel_pressed;
            QSharedPointer<QOpenGLTexture> quit_pressed;
        } buttons;
        struct {
            UnitTextureSet seal;
            UnitTextureSet crusader;
        } units;
    } textures;

    int w = 1;
    int h = 1;
    QRect arena_viewport = {0, 0, 1, 1};
    QPointF arena_viewport_center = {0, 0};
    State state = State::TeamSelection;
    ButtonId pressed_button = ButtonId::None;
    QPoint cursor_position = {-1, -1};
    Unit::Team team;
    QElapsedTimer match_countdown_start;
    QSharedPointer<QOpenGLTexture> cursor;
    QSharedPointer<MatchState> match_state;
    qreal map_to_screen_factor = 1.0;
    qreal viewport_scale = 1.0;
    QRectF arena_viewport_map_coords = {0, 0, 1, 1};
    int mouse_scroll_border = 10;
    int viewport_scale_power = 0;
    QPointF viewport_center = {0.0, 0.0};
    QTimer match_timer;
    QElapsedTimer last_frame;
    std::optional<QPoint> selection_start;
    bool ctrl_pressed = false;
    bool alt_pressed = false;
    bool shift_pressed = false;
    std::mt19937 random_generator;
};
