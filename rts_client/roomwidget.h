#pragma once

#include "openglwidget.h"

#include <QElapsedTimer>
#include <QTimer>

class MatchState;
class Unit;

class RoomWidget: public OpenGLWidget
{
    Q_OBJECT

public:
    enum class Team {
        Red,
        Blue,
        Spectator,
    };
    struct UnitTextureSet {
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
    void awaitTeamSelection (Team team);
    void queryReadiness (Team team);
    void ready (Team team);
    void awaitMatch (Team team);
    void startMatch (Team team);

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
    void drawUnit (const Unit& unit);

private slots:
    void tick ();

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
    struct {
        QSharedPointer<QOpenGLTexture> grass;
        QSharedPointer<QOpenGLTexture> ground;
        QSharedPointer<QOpenGLTexture> hud;
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
    QPoint arena_center = {0, 0};
    State state = State::TeamSelection;
    ButtonId pressed_button = ButtonId::None;
    QPoint cursor_position = {-1, -1};
    Team team;
    QElapsedTimer match_countdown_start;
    QSharedPointer<QOpenGLTexture> cursor;
    QSharedPointer<MatchState> match_state;
    qreal viewport_scale = 1.0;
    int mouse_scroll_border = 10;
    int viewport_scale_power = 0;
    QPointF viewport_center = {0.0, 0.0};
    QTimer match_timer;
    QElapsedTimer last_frame;
};
