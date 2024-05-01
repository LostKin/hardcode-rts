#include "roomwidget.h"

#include "matchstate.h"
#include "coloredrenderer.h"
#include "coloredtexturedrenderer.h"
#include "texturedrenderer.h"
#include "hud.h"
#include "scenerenderer.h"
#include "hudrenderer.h"

#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QFile>
#if QT_VERSION >= 0x060000
#include <QSoundEffect>
#else
#include <QSound>
#endif
#include <QGuiApplication>
#include <math.h>
#include <mutex>

static constexpr qreal SQRT_2 = 1.4142135623731;
static constexpr qreal SQRT_1_2 = 0.70710678118655;
static constexpr qreal PI_X_3_4 = 3.0 / 4.0 * M_PI;
static constexpr qreal PI_X_1_4 = 1.0 / 4.0 * M_PI;

static constexpr qint64 group_count = 20;
static constexpr qreal POINTS_PER_VIEWPORT_VERTICALLY = 20.0; // At zoom x1.0
#define MAP_TO_SCREEN_FACTOR (coord_map.arena_viewport.height () / POINTS_PER_VIEWPORT_VERTICALLY)

static const QMap<SoundEvent, QStringList> sound_map = {
    {SoundEvent::SealAttack, {":/audio/units/seal/attack1.wav", ":/audio/units/seal/attack2.wav"}},
    {SoundEvent::CrusaderAttack, {":/audio/units/crusader/attack1.wav", ":/audio/units/crusader/attack2.wav"}},
    {SoundEvent::BeetleAttack, {":/audio/units/beetle/attack.wav"}},
    {SoundEvent::RocketStart, {":/audio/units/goon/rocket-start.wav"}},
    {SoundEvent::RocketExplosion, {":/audio/effects/rocket-explosion.wav"}},
    {SoundEvent::PestilenceSplash, {":/audio/effects/pestilence-splash1.wav", ":/audio/effects/pestilence-splash2.wav"}},
    {SoundEvent::PestilenceMissileStart, {":/audio/units/contaminator/pestilence.wav"}},
    {SoundEvent::SpawnBeetle, {":/audio/units/contaminator/spawn-beetle.wav"}},
};

static QPointF operator* (const QPointF& a, const QPointF& b)
{
    return {a.x () * b.x (), a.y () * b.y ()};
}


RoomWidget::RoomWidget (QWidget* parent)
    : OpenGLWidget (parent)
{
    setAttribute (Qt::WA_KeyCompression, false);

    last_frame.invalidate ();
    setMouseTracking (true);
    setCursor (QCursor (Qt::BlankCursor));
    connect (&match_timer, SIGNAL (timeout ()), this, SLOT (tick ()));
    connect (this, &RoomWidget::selectRolePlayerRequested, this, &RoomWidget::selectRolePlayerRequestedHandler);
    connect (this, &RoomWidget::spectateRequested, this, &RoomWidget::spectateRequestedHandler);
    connect (this, &RoomWidget::cancelJoinTeamRequested, this, &RoomWidget::cancelJoinTeamRequestedHandler);
    connect (this, &RoomWidget::quitRequested, this, &RoomWidget::quitRequestedHandler);
    connect (this, &RoomWidget::readinessRequested, this, &RoomWidget::readinessRequestedHandler);
}
RoomWidget::~RoomWidget ()
{
}

void RoomWidget::unitActionCallback (quint32 id, const std::variant<StopAction, MoveAction, AttackAction, CastAction>& action)
{
    // void RoomWidget::unitActionCallback (quint32 id, ActionType type, std::variant<QPointF, quint32> target) {
    emit unitActionRequested (id, action);
}

void RoomWidget::unitCreateCallback (Unit::Team team, Unit::Type type, QPointF position)
{
    emit createUnitRequested (team, type, position);
}

void RoomWidget::selectRolePlayerRequestedHandler ()
{
    awaitRoleSelection (UnitRole::Player);
}
void RoomWidget::spectateRequestedHandler ()
{
    awaitRoleSelection (UnitRole::Spectator);
}
void RoomWidget::cancelJoinTeamRequestedHandler ()
{
    state = State::RoleSelection;
}
void RoomWidget::readinessRequestedHandler ()
{
    ready (this->team);
}
void RoomWidget::readinessHandler ()
{
    queryReadiness (this->team);
}
void RoomWidget::quitRequestedHandler ()
{
}

void RoomWidget::startCountDownHandler (Unit::Team team)
{
    awaitMatch (team);
}

void RoomWidget::startMatchHandler ()
{
    qDebug () << "Start match handler started";
    startMatch (this->team);
    // emit createUnitRequested();
}
void RoomWidget::awaitRoleSelection (UnitRole role)
{
    pressed_button = ButtonId::None;
    match_timer.stop ();
    state = State::RoleSelectionRequested;
}
void RoomWidget::queryReadiness (Unit::Team team)
{
    this->team = team;
    pressed_button = ButtonId::None;
    match_timer.stop ();
    state = State::ConfirmingReadiness;
}
void RoomWidget::ready (Unit::Team team)
{
    this->team = team;
    pressed_button = ButtonId::None;
    match_timer.stop ();
    state = State::Ready;
}
void RoomWidget::awaitMatch (Unit::Team team)
{
    this->team = team;
    pressed_button = ButtonId::None;
    match_countdown_start.start ();
    match_timer.stop ();
    state = State::AwaitingMatch;
}
void RoomWidget::startMatch (Unit::Team team)
{
    this->team = team;
    pressed_button = ButtonId::None;
    match_state = QSharedPointer<MatchState> (new MatchState (true));
    connect (&*match_state, &MatchState::unitActionRequested, this, &RoomWidget::unitActionCallback);
    connect (&*match_state, &MatchState::unitCreateRequested, this, &RoomWidget::unitCreateCallback);
    connect (&*match_state, SIGNAL (soundEventEmitted (SoundEvent)), this, SLOT (playSound (SoundEvent)));
    coord_map.viewport_scale_power = 0;
    coord_map.viewport_scale = 1.0;
    coord_map.viewport_center = {};
    match_timer.start (20);
    last_frame.restart ();
    state = State::MatchStarted;
}
void RoomWidget::loadMatchState (QVector<QPair<quint32, Unit>> units, QVector<QPair<quint32, Missile>> missiles)
{
    match_state->LoadState (units, missiles);
    return;
}
quint64 RoomWidget::moveAnimationPeriodNS (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return 500'000'000;
    case Unit::Type::Crusader:
        return 300'000'000;
    case Unit::Type::Goon:
        return 500'000'000;
    case Unit::Type::Beetle:
        return 300'000'000;
    case Unit::Type::Contaminator:
        return 800'000'000;
    default:
        return 0;
    }
}
quint64 RoomWidget::attackAnimationPeriodNS (Unit::Type type)
{
    switch (type) {
    case Unit::Type::Seal:
        return 100'000'000;
    case Unit::Type::Crusader:
        return 700'000'000;
    case Unit::Type::Goon:
        return 200'000'000;
    case Unit::Type::Beetle:
        return 700'000'000;
    case Unit::Type::Contaminator:
        return 700'000'000;
    default:
        return 0;
    }
}
quint64 RoomWidget::missileAnimationPeriodNS (Missile::Type type)
{
    switch (type) {
    case Missile::Type::Rocket:
        return 200'000'000;
    case Missile::Type::Pestilence:
        return 500'000'000;
    default:
        return 0;
    }
}
ActionButtonId RoomWidget::getActionButtonFromGrid (int row, int col)
{
    switch (row) {
    case 0:
        switch (col) {
        case 0:
            return ActionButtonId::Move;
        case 1:
            return ActionButtonId::Stop;
        case 2:
            return ActionButtonId::Hold;
        case 4:
            return ActionButtonId::Attack;
        default:
            break;
        }
        break;
    case 2:
        switch (col) {
        case 0:
            return ActionButtonId::Pestilence;
        case 1:
            return ActionButtonId::Spawn;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return ActionButtonId::None;
}
void RoomWidget::loadTextures ()
{
    textures.grass = loadTexture2DRectangle (":/images/grass.png");

    textures.cursors.crosshair = loadTexture2DRectangle (":/images/cursors/crosshair.png");

    textures.labels.join_team_requested = loadTexture2DRectangle (":/images/labels/join-team-requested.png");
    textures.labels.ready = loadTexture2DRectangle (":/images/labels/ready.png");
    textures.labels.starting_in_0 = loadTexture2DRectangle (":/images/labels/starting-in-0.png");
    textures.labels.starting_in_1 = loadTexture2DRectangle (":/images/labels/starting-in-1.png");
    textures.labels.starting_in_2 = loadTexture2DRectangle (":/images/labels/starting-in-2.png");
    textures.labels.starting_in_3 = loadTexture2DRectangle (":/images/labels/starting-in-3.png");
    textures.labels.starting_in_4 = loadTexture2DRectangle (":/images/labels/starting-in-4.png");
    textures.labels.starting_in_5 = loadTexture2DRectangle (":/images/labels/starting-in-5.png");

    textures.buttons.join_as_player = loadTexture2DRectangle (":/images/buttons/join-as-player.png");
    textures.buttons.spectate = loadTexture2DRectangle (":/images/buttons/spectate.png");
    textures.buttons.ready = loadTexture2DRectangle (":/images/buttons/ready.png");
    textures.buttons.cancel = loadTexture2DRectangle (":/images/buttons/cancel.png");
    textures.buttons.quit = loadTexture2DRectangle (":/images/buttons/quit.png");
    textures.buttons.join_as_player_pressed = loadTexture2DRectangle (":/images/buttons/join-as-player-pressed.png");
    textures.buttons.spectate_pressed = loadTexture2DRectangle (":/images/buttons/spectate-pressed.png");
    textures.buttons.ready_pressed = loadTexture2DRectangle (":/images/buttons/ready-pressed.png");
    textures.buttons.cancel_pressed = loadTexture2DRectangle (":/images/buttons/cancel-pressed.png");
    textures.buttons.quit_pressed = loadTexture2DRectangle (":/images/buttons/quit-pressed.png");
}
void RoomWidget::initResources ()
{
    if (!(colored_renderer = ColoredRenderer::Create ())) {
        qDebug () << "TODO: Handle error";
    }
    if (!(colored_textured_renderer = ColoredTexturedRenderer::Create ())) {
        qDebug () << "TODO: Handle error";
    }
    if (!(textured_renderer = TexturedRenderer::Create ())) {
        qDebug () << "TODO: Handle error";
    }

    scene_renderer = QSharedPointer<SceneRenderer>::create ();
    hud_renderer = QSharedPointer<HUDRenderer>::create ();

    loadTextures ();

    cursor = textures.cursors.crosshair;
}
void RoomWidget::updateSize (int w, int h)
{
    coord_map.viewport_size = {w, h};
    coord_map.arena_viewport = {0, 0, w, h - 220};
    coord_map.arena_viewport_center = QRectF (coord_map.arena_viewport).center ();

    if (w >= 3656)
        hud.margin = 24;
    else if (w >= 2560)
        hud.margin = 18;
    else
        hud.margin = 12;
    hud.stroke_width = (w >= 3656) ? 4 : 2;
    hud.action_button_size = {int (h * 0.064), int (h * 0.064)};
    {
        int area_w = h * 0.30;
        int area_h = h * 0.24;
        hud.minimap_panel_rect = {hud.margin, h - area_h - hud.margin, area_w, area_h};
    }
    {
        int area_w = hud.action_button_size.width () * 5;
        int area_h = hud.action_button_size.height () * 3;
        hud.action_panel_rect = {w - area_w - hud.margin, h - area_h - hud.margin, area_w, area_h};
    }
    {
        static constexpr int icon_column_count = 10;
        static constexpr int icon_row_count = 3;

        int area_w = w - hud.minimap_panel_rect.width () - hud.action_panel_rect.width () - hud.margin * 4;
        int area_h = h * 0.18;
        hud.selection_panel_rect = {hud.margin * 2 + hud.minimap_panel_rect.width (), h - area_h - hud.margin, area_w, area_h};

        int icon_rib1 = hud.selection_panel_rect.height () * 0.3;
        int icon_rib2 = (hud.selection_panel_rect.width () - (hud.selection_panel_rect.height () - icon_rib1 * icon_row_count) / 2 * 2) / icon_column_count;
        int icon_rib = qMin (icon_rib1, icon_rib2);
        int hmargin = (hud.selection_panel_rect.width () - icon_rib * icon_column_count) / 2;
        int vmargin = (hud.selection_panel_rect.height () - icon_rib * icon_row_count) / 2;

        hud.selection_panel_icon_rib = icon_rib;
        hud.selection_panel_icon_grid_pos = {hud.selection_panel_rect.x () + hmargin, hud.selection_panel_rect.y () + vmargin};
    }
    if (match_state) {
        const QRectF& area = match_state->areaRef ();
        qreal aspect = area.width () / area.height ();
        QPointF center = QRectF (hud.minimap_panel_rect).center ();
        QSizeF size = hud.minimap_panel_rect.size ();
        if (size.width () / size.height () < aspect) {
            size.setHeight (size.height () / aspect);
        } else {
            size.setWidth (size.width () * aspect);
        }
        hud.minimap_screen_area = {center.x () - size.width () * 0.5, center.y () - size.height () * 0.5, size.width (), size.height ()};
    } else {
        hud.minimap_screen_area = hud.minimap_panel_rect;
    }
}
void RoomWidget::draw ()
{
    if (cursor_position.x () == -1 && cursor_position.y () == -1)
        cursor_position = QWidget::mapFromGlobal (QCursor::pos ());

    switch (state) {
    case State::RoleSelection:
        drawRoleSelection ();
        break;
    case State::RoleSelectionRequested:
        drawRoleSelectionRequested ();
        break;
    case State::ConfirmingReadiness:
        drawConfirmingReadiness ();
        break;
    case State::Ready:
        drawReady ();
        break;
    case State::AwaitingMatch:
        drawAwaitingMatch ();
        break;
    case State::MatchStarted:
        drawMatchStarted ();
        break;
    default:
        break;
    }

    textured_renderer->fillRectangle (*this, cursor_position.x () - cursor->width () / 2, cursor_position.y () - cursor->height () / 2, cursor.get (), ortho_matrix);
}
void RoomWidget::keyPressEvent (QKeyEvent* event)
{
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers ();
    ctrl_pressed = modifiers & Qt::ControlModifier;
    shift_pressed = modifiers & Qt::ShiftModifier;
    alt_pressed = modifiers & Qt::AltModifier;
    switch (state) {
    case State::MatchStarted: {
        matchKeyPressEvent (event);
    } break;
    default: {
    }
    }
}
void RoomWidget::keyReleaseEvent (QKeyEvent* event)
{
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers ();
    ctrl_pressed = modifiers & Qt::ControlModifier;
    shift_pressed = modifiers & Qt::ShiftModifier;
    alt_pressed = modifiers & Qt::AltModifier;
    switch (state) {
    case State::MatchStarted: {
        matchKeyReleaseEvent (event);
    } break;
    default: {
    }
    }
}
void RoomWidget::mouseMoveEvent (QMouseEvent* event)
{
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers ();
    ctrl_pressed = modifiers & Qt::ControlModifier;
    shift_pressed = modifiers & Qt::ShiftModifier;
    alt_pressed = modifiers & Qt::AltModifier;
    cursor_position = event->pos ();
    switch (state) {
    case State::MatchStarted: {
        matchMouseMoveEvent (event);
    } break;
    default: {
    }
    }
}
void RoomWidget::mousePressEvent (QMouseEvent* event)
{
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers ();
    ctrl_pressed = modifiers & Qt::ControlModifier;
    shift_pressed = modifiers & Qt::ShiftModifier;
    alt_pressed = modifiers & Qt::AltModifier;
    switch (state) {
    case State::RoleSelection: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 30), textures.buttons.join_as_player))
                pressed_button = ButtonId::JoinBlueTeam;
            else if (pointInsideButton (cursor_position, QPoint (30, 230), textures.buttons.spectate))
                pressed_button = ButtonId::Spectate;
            else if (pointInsideButton (cursor_position, QPoint (30, 400), textures.buttons.quit))
                pressed_button = ButtonId::Quit;
        }
    } break;
    case State::RoleSelectionRequested: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 230), textures.buttons.cancel))
                pressed_button = ButtonId::Cancel;
            else if (pointInsideButton (cursor_position, QPoint (30, 400), textures.buttons.quit))
                pressed_button = ButtonId::Quit;
        }
    } break;
    case State::ConfirmingReadiness: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 30), textures.buttons.ready))
                pressed_button = ButtonId::Ready;
            else if (pointInsideButton (cursor_position, QPoint (30, 200), textures.buttons.quit))
                pressed_button = ButtonId::Quit;
        }
    } break;
    case State::Ready: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 200), textures.buttons.quit))
                pressed_button = ButtonId::Quit;
        }
    } break;
    case State::AwaitingMatch: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 200), textures.buttons.quit))
                pressed_button = ButtonId::Quit;
        }
    } break;
    case State::MatchStarted: {
        matchMousePressEvent (event);
    } break;
    default: {
    }
    }
}
void RoomWidget::mouseReleaseEvent (QMouseEvent* event)
{
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers ();
    ctrl_pressed = modifiers & Qt::ControlModifier;
    shift_pressed = modifiers & Qt::ShiftModifier;
    alt_pressed = modifiers & Qt::AltModifier;
    switch (state) {
    case State::RoleSelection: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 30), textures.buttons.join_as_player)) {
                if (pressed_button == ButtonId::JoinBlueTeam)
                    emit selectRolePlayerRequested ();
            } else if (pointInsideButton (cursor_position, QPoint (30, 230), textures.buttons.spectate)) {
                if (pressed_button == ButtonId::Spectate)
                    emit spectateRequested ();
            } else if (pointInsideButton (cursor_position, QPoint (30, 400), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit)
                    emit quitRequested ();
            }
        }
    } break;
    case State::RoleSelectionRequested: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 230), textures.buttons.cancel)) {
                if (pressed_button == ButtonId::Cancel)
                    emit cancelJoinTeamRequested ();
            } else if (pointInsideButton (cursor_position, QPoint (30, 400), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit)
                    emit quitRequested ();
            }
        }
    } break;
    case State::ConfirmingReadiness: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 30), textures.buttons.ready)) {
                if (pressed_button == ButtonId::Ready)
                    emit readinessRequested ();
            } else if (pointInsideButton (cursor_position, QPoint (30, 200), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit)
                    emit quitRequested ();
            }
        }
    } break;
    case State::Ready: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 200), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit)
                    emit quitRequested ();
            }
        }
    } break;
    case State::AwaitingMatch: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 200), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit)
                    emit quitRequested ();
            }
        }
    } break;
    case State::MatchStarted: {
        matchMouseReleaseEvent (event);
    } break;
    default: {
    }
    }
    pressed_button = ButtonId::None;
}
void RoomWidget::wheelEvent (QWheelEvent* event)
{
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers ();
    ctrl_pressed = modifiers & Qt::ControlModifier;
    shift_pressed = modifiers & Qt::ShiftModifier;
    alt_pressed = modifiers & Qt::AltModifier;
    switch (state) {
    case State::MatchStarted: {
        matchWheelEvent (event);
    } break;
    default: {
    }
    }
}
void RoomWidget::matchKeyPressEvent (QKeyEvent* event)
{
    switch (event->key ()) {
    case Qt::Key_A:
        match_state->attackEnemy (team, coord_map.toMapCoords (cursor_position));
        break;
    case Qt::Key_E:
        match_state->cast (CastAction::Type::Pestilence, team, coord_map.toMapCoords (cursor_position));
        break;
    case Qt::Key_T:
        match_state->cast (CastAction::Type::SpawnBeetle, team, coord_map.toMapCoords (cursor_position));
        break;
    case Qt::Key_G:
        match_state->move (coord_map.toMapCoords (cursor_position));
        break;
    case Qt::Key_S:
        match_state->stop ();
        break;
    case Qt::Key_F:
        if (ctrl_pressed)
            centerViewportAtSelected ();
        break;
    case Qt::Key_1:
    case Qt::Key_Exclam:
        groupEvent (1);
        break;
    case Qt::Key_2:
    case Qt::Key_At:
        groupEvent (2);
        break;
    case Qt::Key_3:
    case Qt::Key_NumberSign:
        groupEvent (3);
        break;
    case Qt::Key_4:
    case Qt::Key_Dollar:
        groupEvent (4);
        break;
    case Qt::Key_5:
    case Qt::Key_Percent:
        groupEvent (5);
        break;
    case Qt::Key_6:
    case Qt::Key_AsciiCircum:
        groupEvent (6);
        break;
    case Qt::Key_7:
    case Qt::Key_Ampersand:
        groupEvent (7);
        break;
    case Qt::Key_8:
    case Qt::Key_Asterisk:
        groupEvent (8);
        break;
    case Qt::Key_9:
    case Qt::Key_ParenLeft:
    case Qt::Key_QuoteLeft:
    case Qt::Key_AsciiTilde:
        groupEvent (9);
        break;
    case Qt::Key_0:
    case Qt::Key_ParenRight:
        groupEvent (10);
        break;
    case Qt::Key_F1:
        match_state->selectAll (team);
        break;
    case Qt::Key_F3:
        emit createUnitRequested (this->team, Unit::Type::Crusader, coord_map.toMapCoords (cursor_position));
        break;
    case Qt::Key_F4:
        emit createUnitRequested (this->team, Unit::Type::Seal, coord_map.toMapCoords (cursor_position));
        break;
    case Qt::Key_F5:
        emit createUnitRequested (this->team, Unit::Type::Goon, coord_map.toMapCoords (cursor_position));
        break;
    case Qt::Key_F6:
        emit createUnitRequested (this->team, Unit::Type::Contaminator, coord_map.toMapCoords (cursor_position));
        break;
    }
}
void RoomWidget::matchKeyReleaseEvent (QKeyEvent* event)
{
    switch (event->key ()) {
    }
}
void RoomWidget::matchMouseMoveEvent (QMouseEvent* /* event */)
{
    if (minimap_viewport_selection_pressed) {
        QPointF area_pos;
        if (getMinimapPositionUnderCursor (cursor_position, area_pos))
            centerViewportAt (area_pos);
    }
}
void RoomWidget::matchMousePressEvent (QMouseEvent* event)
{
    int row, col;
    QPointF area_pos;
    if (getActionButtonUnderCursor (cursor_position, row, col)) {
        switch (event->button ()) {
        case Qt::LeftButton: {
            if (hud.current_action_button == ActionButtonId::None &&
                hud.pressed_action_button == ActionButtonId::None)
                hud.pressed_action_button = getActionButtonFromGrid (row, col);
        } break;
        default: {
        }
        }
    } else if (getSelectionPanelUnitUnderCursor (cursor_position, row, col)) {
        QVector<QPair<quint32, const Unit*>> selection = match_state->buildOrderedSelection ();
        if (selection.size () > 1) {
            int i = row * 10 + col;
            if (i < selection.size ()) {
                if (ctrl_pressed) {
                    if (shift_pressed)
                        match_state->trimSelection (selection[i].second->type, true);
                    else
                        match_state->trimSelection (selection[i].second->type, false);
                } else {
                    if (shift_pressed)
                        match_state->deselect (selection[i].first);
                    else
                        match_state->select (selection[i].first, false);
                }
            }
        }
    } else if (getMinimapPositionUnderCursor (cursor_position, area_pos)) {
        switch (event->button ()) {
        case Qt::LeftButton: {
            centerViewportAt (area_pos);
            minimap_viewport_selection_pressed = true;
        } break;
        case Qt::RightButton: {
            match_state->autoAction (team, area_pos);
        } break;
        default: {
        }
        }
    } else if (cursorIsAboveMajorMap (cursor_position)) {
        switch (event->button ()) {
        case Qt::LeftButton: {
            if (ctrl_pressed) {
                match_state->trySelectByType (team, coord_map.toMapCoords (cursor_position), coord_map.toMapCoords (coord_map.arena_viewport), shift_pressed);
            } else {
                selection_start = cursor_position;
            }
        } break;
        case Qt::RightButton: {
            match_state->autoAction (team, coord_map.toMapCoords (cursor_position));
        } break;
        default: {
        }
        }
    }
}
void RoomWidget::matchMouseReleaseEvent (QMouseEvent* event)
{
    if (selection_start.has_value ()) {
        QPointF p1 = coord_map.toMapCoords (*selection_start);
        QPointF p2 = coord_map.toMapCoords (cursor_position);
        if (match_state->fuzzyMatchPoints (p1, p2)) {
            match_state->trySelect (team, p1, shift_pressed);
        } else {
            match_state->trySelect (team, {qMin (p1.x (), p2.x ()), qMin (p1.y (), p2.y ()), qAbs (p1.x () - p2.x ()), qAbs (p1.y () - p2.y ())}, shift_pressed);
        }
    } else {
        int row, col;
        if (getActionButtonUnderCursor (cursor_position, row, col)) {
            switch (event->button ()) {
            case Qt::LeftButton: {
                if (hud.current_action_button == ActionButtonId::None &&
                    hud.pressed_action_button != ActionButtonId::None &&
                    hud.pressed_action_button == getActionButtonFromGrid (row, col)) {
                    // TODO: Stateful actions
                }
            } break;
            default: {
            }
            }
        } else if (cursorIsAboveMajorMap (cursor_position)) {
            switch (event->button ()) {
            case Qt::LeftButton: {
            } break;
            default: {
            }
            }
        }
    }
    if (event->button () == Qt::LeftButton) {
        hud.pressed_action_button = ActionButtonId::None;
        minimap_viewport_selection_pressed = false;
        selection_start.reset ();
    }
}
void RoomWidget::matchWheelEvent (QWheelEvent* event)
{
    zoom ((event->angleDelta ().y () > 0) ? 1 : -1);
}
void RoomWidget::drawRoleSelection ()
{
    textured_renderer->fillRectangle (*this, 0, 0, coord_map.viewport_size.width (), coord_map.viewport_size.height (), textures.grass.get (), ortho_matrix);

    textured_renderer->fillRectangle (*this, 30, 30, (pressed_button == ButtonId::JoinBlueTeam) ? textures.buttons.join_as_player_pressed.get () : textures.buttons.join_as_player.get (), ortho_matrix);
    textured_renderer->fillRectangle (*this, 30, 230, (pressed_button == ButtonId::Spectate) ? textures.buttons.spectate_pressed.get () : textures.buttons.spectate.get (), ortho_matrix);
    textured_renderer->fillRectangle (*this, 30, 400, (pressed_button == ButtonId::Quit) ? textures.buttons.quit_pressed.get () : textures.buttons.quit.get (), ortho_matrix);
}
void RoomWidget::drawRoleSelectionRequested ()
{
    textured_renderer->fillRectangle (*this, 0, 0, coord_map.viewport_size.width (), coord_map.viewport_size.height (), textures.grass.get (), ortho_matrix);

    textured_renderer->fillRectangle (*this, 30, 30, textures.labels.join_team_requested.get (), ortho_matrix);

    textured_renderer->fillRectangle (*this, 30, 230, (pressed_button == ButtonId::Cancel) ? textures.buttons.cancel_pressed.get () : textures.buttons.cancel.get (), ortho_matrix);
    textured_renderer->fillRectangle (*this, 30, 400, (pressed_button == ButtonId::Quit) ? textures.buttons.quit_pressed.get () : textures.buttons.quit.get (), ortho_matrix);
}
void RoomWidget::drawConfirmingReadiness ()
{
    textured_renderer->fillRectangle (*this, 0, 0, coord_map.viewport_size.width (), coord_map.viewport_size.height (), textures.grass.get (), ortho_matrix);

    textured_renderer->fillRectangle (*this, 30, 30, (pressed_button == ButtonId::Ready) ? textures.buttons.ready_pressed.get () : textures.buttons.ready.get (), ortho_matrix);
    textured_renderer->fillRectangle (*this, 30, 200, (pressed_button == ButtonId::Quit) ? textures.buttons.quit_pressed.get () : textures.buttons.quit.get (), ortho_matrix);
}
void RoomWidget::drawReady ()
{
    textured_renderer->fillRectangle (*this, 0, 0, coord_map.viewport_size.width (), coord_map.viewport_size.height (), textures.grass.get (), ortho_matrix);

    textured_renderer->fillRectangle (*this, 30, 30, textures.labels.ready.get (), ortho_matrix);

    textured_renderer->fillRectangle (*this, 30, 200, (pressed_button == ButtonId::Quit) ? textures.buttons.quit_pressed.get () : textures.buttons.quit.get (), ortho_matrix);
}
void RoomWidget::drawAwaitingMatch ()
{
    textured_renderer->fillRectangle (*this, 0, 0, coord_map.viewport_size.width (), coord_map.viewport_size.height (), textures.grass.get (), ortho_matrix);

    qint64 nsecs_elapsed = match_countdown_start.nsecsElapsed ();
    QOpenGLTexture* label_texture;
    if (nsecs_elapsed < 1000000000LL)
        label_texture = textures.labels.starting_in_5.get ();
    else if (nsecs_elapsed < 2000000000LL)
        label_texture = textures.labels.starting_in_4.get ();
    else if (nsecs_elapsed < 3000000000LL)
        label_texture = textures.labels.starting_in_3.get ();
    else if (nsecs_elapsed < 4000000000LL)
        label_texture = textures.labels.starting_in_2.get ();
    else if (nsecs_elapsed < 5000000000LL)
        label_texture = textures.labels.starting_in_1.get ();
    else
        label_texture = textures.labels.starting_in_0.get ();

    textured_renderer->fillRectangle (*this, 30, 30, label_texture, ortho_matrix);

    textured_renderer->fillRectangle (*this, 30, 200, (pressed_button == ButtonId::Quit) ? textures.buttons.quit_pressed.get () : textures.buttons.quit.get (), ortho_matrix);
}
void RoomWidget::drawMatchStarted ()
{
    if (last_frame.isValid ())
        frameUpdate (last_frame.nsecsElapsed () * 0.000000001);
    last_frame.restart ();

    scene_renderer->draw (*this, *colored_renderer, *colored_textured_renderer, *textured_renderer,
                          *match_state, team, cursor_position, selection_start,
                          ortho_matrix, coord_map);

    hud_renderer->draw (*this, *colored_renderer, *colored_textured_renderer, *textured_renderer, *this,
                        hud, *match_state, team, ortho_matrix, coord_map);
}
void RoomWidget::tick ()
{
    switch (state) {
    case State::MatchStarted: {
        match_state->tick ();
    } break;
    default: {
    }
    }
}
void RoomWidget::playSound (SoundEvent event)
{
    QMap<SoundEvent, QStringList>::const_iterator sound_it = sound_map.find (event);
    if (sound_it == sound_map.end ())
        return;
    const QStringList& sound_files = *sound_it;
    if (!sound_files.size ())
        return;
    // QSound::play (sound_files[random_generator ()%sound_files.size ()]);
}
void RoomWidget::frameUpdate (qreal dt)
{
    switch (state) {
    case State::MatchStarted: {
        matchFrameUpdate (dt);
    } break;
    default: {
    }
    }
}
void RoomWidget::matchFrameUpdate (qreal dt)
{
    if (cursor_position.x () == -1 || coord_map.viewport_size.width () == 1)
        return;

    int dx = 0;
    int dy = 0;
    if (cursor_position.x () <= mouse_scroll_border)
        --dx;
    if (cursor_position.x () >= (coord_map.viewport_size.width () - mouse_scroll_border - 1))
        ++dx;
    if (cursor_position.y () <= mouse_scroll_border)
        --dy;
    if (cursor_position.y () >= (coord_map.viewport_size.height () - mouse_scroll_border - 1))
        ++dy;
    qreal off = 4000.0;
    if (dx && dy)
        off *= SQRT_1_2;
    qreal scale = coord_map.viewport_scale * MAP_TO_SCREEN_FACTOR;
    coord_map.viewport_center += QPointF (dx * off * dt, dy * off * dt)/scale;
    const QRectF& area = match_state->areaRef ();
    if (coord_map.viewport_center.x () < area.left ())
        coord_map.viewport_center.setX (area.left ());
    else if (coord_map.viewport_center.x () > area.right ())
        coord_map.viewport_center.setX (area.right ());
    if (coord_map.viewport_center.y () < area.top ())
        coord_map.viewport_center.setY (area.top ());
    else if (coord_map.viewport_center.y () > area.bottom ())
        coord_map.viewport_center.setY (area.bottom ());
}
bool RoomWidget::pointInsideButton (const QPoint& point, const QPoint& button_pos, QSharedPointer<QOpenGLTexture>& texture) const
{
    return point.x () >= button_pos.x () &&
           point.x () < (button_pos.x () + texture->width ()) &&
           point.y () >= button_pos.y () &&
           point.y () < (button_pos.y () + texture->height ());
}
bool RoomWidget::getActionButtonUnderCursor (const QPoint& cursor_pos, int& row, int& col) const
{
    row = (cursor_pos.y () - hud.action_panel_rect.y ());
    col = (cursor_pos.x () - hud.action_panel_rect.x ());
    if (row < 0 || col < 0)
        return false;
    row /= hud.action_button_size.height ();
    col /= hud.action_button_size.width ();
    return row < 3 && col < 5;
}
bool RoomWidget::getSelectionPanelUnitUnderCursor (const QPoint& cursor_pos, int& row, int& col) const
{
    row = (cursor_pos.y () - hud.selection_panel_icon_grid_pos.y ());
    col = (cursor_pos.x () - hud.selection_panel_icon_grid_pos.x ());
    if (row < 0 || col < 0)
        return false;
    row /= hud.selection_panel_icon_rib;
    col /= hud.selection_panel_icon_rib;
    return row < 3 && col < 10;
}
bool RoomWidget::getMinimapPositionUnderCursor (const QPoint& cursor_pos, QPointF& area_pos) const
{
    if (cursor_pos.x () >= hud.minimap_screen_area.left () &&
        cursor_pos.x () <= hud.minimap_screen_area.right () &&
        cursor_pos.y () >= hud.minimap_screen_area.top () &&
        cursor_pos.y () <= hud.minimap_screen_area.bottom ()) {
        const QRectF& area = match_state->areaRef ();
        area_pos = area.topLeft () + (cursor_pos - hud.minimap_screen_area.topLeft ()) * QPointF (area.width () / hud.minimap_screen_area.width (), area.height () / hud.minimap_screen_area.height ());
        return true;
    }
    return false;
}
bool RoomWidget::cursorIsAboveMajorMap (const QPoint& cursor_pos) const
{
    int margin_x2 = hud.margin * 2;
    if (cursor_pos.x () < hud.minimap_panel_rect.width () + margin_x2)
        return cursor_pos.y () < (coord_map.viewport_size.height () - hud.minimap_panel_rect.height () - margin_x2);
    else if (cursor_pos.x () >= (coord_map.viewport_size.width () - hud.action_panel_rect.width () - margin_x2))
        return cursor_pos.y () < (coord_map.viewport_size.height () - hud.action_panel_rect.height () - margin_x2);
    else
        return cursor_pos.y () < (coord_map.viewport_size.height () - hud.selection_panel_rect.height () - margin_x2);
}
void RoomWidget::centerViewportAt (const QPointF& point)
{
    coord_map.viewport_center = point;
}
void RoomWidget::centerViewportAtSelected ()
{
    std::optional<QPointF> center = match_state->selectionCenter ();
    if (center.has_value ())
        centerViewportAt (*center);
}
void RoomWidget::groupEvent (quint64 group_num)
{
    if (alt_pressed) {
        match_state->moveSelectionToGroup (group_num, shift_pressed);
    } else if (ctrl_pressed) {
        match_state->bindSelectionToGroup (group_num);
    } else {
        if (shift_pressed)
            match_state->addSelectionToGroup (group_num);
        else
            match_state->selectGroup (group_num);
    }
}
void RoomWidget::zoom (int delta)
{
    coord_map.viewport_scale_power += delta;
    coord_map.viewport_scale_power = qBound (-10, coord_map.viewport_scale_power, 10);
    QPointF offset_before = coord_map.toMapCoords (cursor_position) - coord_map.viewport_center;
    coord_map.viewport_scale = pow (1.125, coord_map.viewport_scale_power);
    QPointF offset_after = coord_map.toMapCoords (cursor_position) - coord_map.viewport_center;

    coord_map.viewport_center -= offset_after - offset_before;
}
