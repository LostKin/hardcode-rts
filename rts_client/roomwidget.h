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
    void createUnitRequested (Unit::Team team, Unit::Type type, QPointF position);
    // void unitActionRequested (quint32 id, ActionType type, std::variant<QPointF, quint32> target);

    void unitActionRequested (quint32 id, const std::variant<StopAction, MoveAction, AttackAction, CastAction>& action);

private slots:
    void joinRedTeamRequestedHandler ();
    void joinBlueTeamRequestedHandler ();
    void spectateRequestedHandler ();
    void readinessRequestedHandler ();
    void cancelJoinTeamRequestedHandler ();
    void quitRequestedHandler ();

public slots:
    void readinessHandler ();
    void startMatchHandler ();
    void startCountDownHandler ();
    void loadMatchState (QVector<QPair<quint32, Unit>> units, QVector<QPair<quint32, Missile>> missiles);
    // void unitActionCallback (quint32 id, ActionType type, std::variant<QPointF, quint32> target);

    void unitActionCallback (quint32 id, const std::variant<StopAction, MoveAction, AttackAction, CastAction>& action);
    void unitCreateCallback (Unit::Team team, Unit::Type type, QPointF position);

protected:
    virtual void initResources () override;
    virtual void updateSize (int w, int h) override;
    virtual void draw () override;
    virtual void keyPressEvent (QKeyEvent* event) override;
    virtual void keyReleaseEvent (QKeyEvent* event) override;
    virtual void mouseMoveEvent (QMouseEvent* event) override;
    virtual void mousePressEvent (QMouseEvent* event) override;
    virtual void mouseReleaseEvent (QMouseEvent* event) override;
    virtual void wheelEvent (QWheelEvent* event) override;

private:
    static quint64 moveAnimationPeriodNS (Unit::Type type);
    static quint64 attackAnimationPeriodNS (Unit::Type type);
    static quint64 missileAnimationPeriodNS (Missile::Type type);
    static quint64 explosionAnimationPeriodNS ();
    void matchKeyPressEvent (QKeyEvent* event);
    void matchKeyReleaseEvent (QKeyEvent* event);
    void matchMouseMoveEvent (QMouseEvent* event);
    void matchMousePressEvent (QMouseEvent* event);
    void matchMouseReleaseEvent (QMouseEvent* event);
    void matchWheelEvent (QWheelEvent* event);
    void loadTextures ();
    void drawTeamSelection ();
    void drawTeamSelectionRequested ();
    void drawConfirmingReadiness ();
    void drawReady ();
    void drawAwaitingMatch ();
    void drawMatchStarted ();
    void frameUpdate (qreal dt);
    void matchFrameUpdate (qreal dt);
    QPointF toMapCoords (const QPointF& point) const;
    QRectF toMapCoords (const QRectF& rect) const;
    QPointF toScreenCoords (const QPointF& point) const;
    bool pointInsideButton (const QPoint& point, const QPoint& button_pos, QSharedPointer<QOpenGLTexture>& texture) const;
    bool getActionButtonUnderCursor (const QPoint& cursor_pos, int& row, int& col) const;
    bool getSelectionPanelUnitUnderCursor (const QPoint& cursor_pos, int& row, int& col) const;
    bool getMinimapPositionUnderCursor (const QPoint& cursor_pos, QPointF& area_pos) const;
    bool cursorIsAboveMajorMap (const QPoint& cursor_pos) const;
    void centerViewportAt (const QPointF& point);
    void centerViewportAtSelected ();
    void drawHUD ();
    void drawMinimap ();
    void drawSelectionPanel (const QRect& rect, size_t selected_count, const Unit* last_selected_unit);
    void drawUnit (const Unit& unit);
    void drawIcon (const Unit& unit, int x, int y, int w, int h, bool framed = false);
    void drawTabs (int x, int y, int w, int h);
    void drawUnitHPBar (const Unit& unit);
    void drawMissile (const Missile& missile);
    void drawExplosion (const Explosion& explosion);
    void drawUnitPathToTarget (const Unit& unit);
    void drawActionButton (const QRect& rect, bool pressed, QOpenGLTexture* texture);
    void drawActionButtonShade (const QRect& rect, bool pressed, qreal remaining);
    void selectGroup (quint64 group);
    void bindSelectionToGroup (quint64 group);
    void groupEvent (quint64 group_num);
    QVector<QPair<quint32, const Unit*>> buildOrderedSelection ();

private slots:
    void tick ();
    void playSound (SoundEvent event);

private:
    struct UnitTextureTeam {
        QSharedPointer<QOpenGLTexture> standing;
        QSharedPointer<QOpenGLTexture> walking1;
        QSharedPointer<QOpenGLTexture> walking2;
        QSharedPointer<QOpenGLTexture> shooting1;
        QSharedPointer<QOpenGLTexture> shooting2;
    };
    struct UnitTextureSet {
        UnitTextureTeam red;
        UnitTextureTeam blue;
    };

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

    enum class ActionButtonId {
        None,
        Move,
        Stop,
        Hold,
        Attack,
        Pestilence,
        Spawn,
    };

private:
    static ActionButtonId getActionButtonFromGrid (int row, int col);

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
            UnitTextureSet goon;
            UnitTextureSet beetle;
            UnitTextureSet contaminator;
            QSharedPointer<QOpenGLTexture> contaminator_cooldown_shade;
        } units;
        struct {
            QSharedPointer<QOpenGLTexture> seal;
            QSharedPointer<QOpenGLTexture> crusader;
            QSharedPointer<QOpenGLTexture> goon;
            QSharedPointer<QOpenGLTexture> beetle;
            QSharedPointer<QOpenGLTexture> contaminator;
            QSharedPointer<QOpenGLTexture> frame;
            QSharedPointer<QOpenGLTexture> tabs;
        } unit_icons;
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
        } effects;
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
        } actions;
    } textures;

    int w = 1;
    int h = 1;
    struct {
        int margin = 1;
        int stroke_width = 1;
        QSize action_button_size = {1, 1};
        QRect minimap_panel_rect = {0, 0, 1, 1};
        QRect action_panel_rect = {0, 0, 1, 1};
        QRect selection_panel_rect = {0, 0, 1, 1};
        int selection_panel_icon_rib = 1;
        QPoint selection_panel_icon_grid_pos = {0, 0};
        QRectF minimap_screen_area = {0, 0, 1, 1};
    } hud;
    QRect arena_viewport = {0, 0, 1, 1};
    QPointF arena_viewport_center = {0, 0};
    State state = State::TeamSelection;
    ButtonId pressed_button = ButtonId::None;
    QPoint cursor_position = {-1, -1};
    ActionButtonId pressed_action_button = ActionButtonId::None;
    ActionButtonId current_action_button = ActionButtonId::None;
    bool minimap_viewport_selection_pressed = false;
    Unit::Team team;
    QElapsedTimer match_countdown_start;
    QSharedPointer<QOpenGLTexture> cursor;
    QSharedPointer<MatchState> match_state;
    qreal map_to_screen_factor = 1.0;
    qreal viewport_scale = 1.0;
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
