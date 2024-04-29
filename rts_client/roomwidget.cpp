#include "roomwidget.h"

#include "matchstate.h"
#include "coloredrenderer.h"
#include "coloredtexturedrenderer.h"
#include "texturedrenderer.h"
#include "unitgenerator.h"
#include "unitsetrenderer.h"
#include "hud.h"
#include "actionpanelrenderer.h"
#include "effectrenderer.h"
#include "minimaprenderer.h"

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

static const QString& unitTitle (Unit::Type type)
{
    static const QString seal = "Seal";
    static const QString crusader = "Crusader";
    static const QString goon = "Goon";
    static const QString beetle = "Beetle";
    static const QString contaminator = "Contaminator";
    static const QString unknown = "Unknown";

    switch (type) {
    case Unit::Type::Seal:
        return seal;
    case Unit::Type::Crusader:
        return crusader;
    case Unit::Type::Goon:
        return goon;
    case Unit::Type::Beetle:
        return beetle;
    case Unit::Type::Contaminator:
        return contaminator;
    default:
        return unknown;
    }
}
static const QString& unitAttackClassName (AttackDescription::Type type)
{
    static const QString immediate = "immediate";
    static const QString missile = "missile";
    static const QString splash = "splash";
    static const QString unknown = "unknown";

    switch (type) {
    case AttackDescription::Type::SealShot:
    case AttackDescription::Type::CrusaderChop:
    case AttackDescription::Type::BeetleSlice:
        return immediate;
    case AttackDescription::Type::GoonRocket:
    case AttackDescription::Type::PestilenceMissile:
        return missile;
    case AttackDescription::Type::GoonRocketExplosion:
    case AttackDescription::Type::PestilenceSplash:
        return splash;
    default:
        return unknown;
    }
}
static QColor getHPColor (qreal hp_ratio)
{
    return QColor::fromRgbF (1.0 - hp_ratio, hp_ratio, 0.0);
}
static QPointF operator* (const QPointF& a, const QPointF& b)
{
    return {a.x () * b.x (), a.y () * b.y ()};
}


RoomWidget::RoomWidget (QWidget* parent)
    : OpenGLWidget (parent)
    , font ("NotCourier", 16)
    , group_number_font ("NotCourier", 10)
    , group_size_font ("NotCourier", 8)
{
    hud = QSharedPointer<HUD>::create ();
    setAttribute (Qt::WA_KeyCompression, false);
    font.setStyleHint (QFont::TypeWriter);

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
    return;
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
    textures.ground = loadTexture2D (":/images/ground.png");

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

    static constexpr QColor icon_color (0xdd, 0xdd, 0xdd);

    textures.unit_icons.seal = loadTexture2D (UnitGenerator::loadUnitFromSVGTemplate (":/images/units/seal/unit_tmpl.svg", "standing", icon_color));
    textures.unit_icons.crusader = loadTexture2D (UnitGenerator::loadUnitFromSVGTemplate (":/images/units/crusader/unit_tmpl.svg", "standing", icon_color));
    textures.unit_icons.goon = loadTexture2D (UnitGenerator::loadUnitFromSVGTemplate (":/images/units/goon/unit_tmpl.svg", "standing", icon_color));
    textures.unit_icons.beetle = loadTexture2D (UnitGenerator::loadUnitFromSVGTemplate (":/images/units/beetle/unit_tmpl.svg", "standing", icon_color));
    textures.unit_icons.contaminator = loadTexture2D (UnitGenerator::loadUnitFromSVGTemplate (":/images/units/contaminator/unit_tmpl.svg", "standing", icon_color));
    textures.unit_icons.frame = loadTexture2D (":/images/icons/frame.png"); // TODO: Refine drawing
    textures.unit_icons.tabs = loadTexture2D (":/images/icons/tabs.png"); // TODO: Refine drawing
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

    unit_set_renderer = QSharedPointer<UnitSetRenderer>::create (red_player_color, blue_player_color);
    action_panel_renderer = QSharedPointer<ActionPanelRenderer>::create ();
    effect_renderer = QSharedPointer<EffectRenderer>::create ();
    minimap_renderer = QSharedPointer<MinimapRenderer>::create ();

    loadTextures ();

    cursor = textures.cursors.crosshair;
}
void RoomWidget::updateSize (int w, int h)
{
    coord_map.viewport_size = {w, h};
    coord_map.arena_viewport = {0, 0, w, h - 220};
    coord_map.arena_viewport_center = QRectF (coord_map.arena_viewport).center ();

    if (w >= 3656)
        hud->margin = 24;
    else if (w >= 2560)
        hud->margin = 18;
    else
        hud->margin = 12;
    hud->stroke_width = (w >= 3656) ? 4 : 2;
    hud->action_button_size = {int (h * 0.064), int (h * 0.064)};
    {
        int area_w = h * 0.30;
        int area_h = h * 0.24;
        hud->minimap_panel_rect = {hud->margin, h - area_h - hud->margin, area_w, area_h};
    }
    {
        int area_w = hud->action_button_size.width () * 5;
        int area_h = hud->action_button_size.height () * 3;
        hud->action_panel_rect = {w - area_w - hud->margin, h - area_h - hud->margin, area_w, area_h};
    }
    {
        static constexpr int icon_column_count = 10;
        static constexpr int icon_row_count = 3;

        int area_w = w - hud->minimap_panel_rect.width () - hud->action_panel_rect.width () - hud->margin * 4;
        int area_h = h * 0.18;
        hud->selection_panel_rect = {hud->margin * 2 + hud->minimap_panel_rect.width (), h - area_h - hud->margin, area_w, area_h};

        int icon_rib1 = hud->selection_panel_rect.height () * 0.3;
        int icon_rib2 = (hud->selection_panel_rect.width () - (hud->selection_panel_rect.height () - icon_rib1 * icon_row_count) / 2 * 2) / icon_column_count;
        int icon_rib = qMin (icon_rib1, icon_rib2);
        int hmargin = (hud->selection_panel_rect.width () - icon_rib * icon_column_count) / 2;
        int vmargin = (hud->selection_panel_rect.height () - icon_rib * icon_row_count) / 2;

        hud->selection_panel_icon_rib = icon_rib;
        hud->selection_panel_icon_grid_pos = {hud->selection_panel_rect.x () + hmargin, hud->selection_panel_rect.y () + vmargin};
    }
    if (match_state) {
        const QRectF& area = match_state->areaRef ();
        qreal aspect = area.width () / area.height ();
        QPointF center = QRectF (hud->minimap_panel_rect).center ();
        QSizeF size = hud->minimap_panel_rect.size ();
        if (size.width () / size.height () < aspect) {
            size.setHeight (size.height () / aspect);
        } else {
            size.setWidth (size.width () * aspect);
        }
        hud->minimap_screen_area = {center.x () - size.width () * 0.5, center.y () - size.height () * 0.5, size.width (), size.height ()};
    } else {
        hud->minimap_screen_area = hud->minimap_panel_rect;
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
            if (hud->current_action_button == ActionButtonId::None &&
                hud->pressed_action_button == ActionButtonId::None)
                hud->pressed_action_button = getActionButtonFromGrid (row, col);
        } break;
        default: {
        }
        }
    } else if (getSelectionPanelUnitUnderCursor (cursor_position, row, col)) {
        QVector<QPair<quint32, const Unit*>> selection = buildOrderedSelection ();
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
                if (hud->current_action_button == ActionButtonId::None &&
                    hud->pressed_action_button != ActionButtonId::None &&
                    hud->pressed_action_button == getActionButtonFromGrid (row, col)) {
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
        hud->pressed_action_button = ActionButtonId::None;
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

    const QRectF& area = match_state->areaRef ();

    {
        qreal scale = coord_map.viewport_scale * MAP_TO_SCREEN_FACTOR;
        QPointF center = coord_map.arena_viewport_center - coord_map.viewport_center*scale;
        const GLfloat vertices[] = {
            GLfloat (center.x () + scale * area.left ()),
            GLfloat (center.y () + scale * area.top ()),
            GLfloat (center.x () + scale * area.right ()),
            GLfloat (center.y () + scale * area.top ()),
            GLfloat (center.x () + scale * area.right ()),
            GLfloat (center.y () + scale * area.bottom ()),
            GLfloat (center.x () + scale * area.left ()),
            GLfloat (center.y () + scale * area.bottom ()),
        };

        static const GLfloat texture_coords[] = {
            0,
            0,
            1,
            0,
            1,
            1,
            0,
            1,
        };

        static const GLuint indices[] = {
            0,
            1,
            2,
            0,
            2,
            3,
        };

        textured_renderer->draw (*this, GL_TRIANGLES, vertices, texture_coords, 6, indices, textures.ground.get (), ortho_matrix);
    }

    const QHash<quint32, Unit>& units = match_state->unitsRef ();
    const QHash<quint32, Missile>& missiles = match_state->missilesRef ();
    const QHash<quint32, Explosion>& explosions = match_state->explosionsRef ();

    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it)
        drawUnit (it.value ());
    for (QHash<quint32, Missile>::const_iterator it = missiles.constBegin (); it != missiles.constEnd (); ++it)
        effect_renderer->drawMissile (*this, *textured_renderer, it.value (), match_state->clockNS (), ortho_matrix, coord_map);
    for (QHash<quint32, Explosion>::const_iterator it = explosions.constBegin (); it != explosions.constEnd (); ++it)
        effect_renderer->drawExplosion (*this, *colored_textured_renderer, it.value (), match_state->clockNS (), ortho_matrix, coord_map);
    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it)
        drawUnitHPBar (it.value ());
    glEnable (GL_LINE_SMOOTH);
    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it) {
        const Unit& unit = *it;
        if (unit.team == team && unit.selected)
            drawUnitPathToTarget (unit);
    }
    glDisable (GL_LINE_SMOOTH);

    if (selection_start.has_value () && *selection_start != cursor_position) {
        colored_renderer->drawRectangle (
            *this,
            qMin (selection_start->x (), cursor_position.x ()), qMin (selection_start->y (), cursor_position.y ()),
            qAbs (selection_start->x () - cursor_position.x ()), qAbs (selection_start->y () - cursor_position.y ()),
            QColor (0, 255, 0, 255),
            ortho_matrix);
    }

    drawHUD ();
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
    row = (cursor_pos.y () - hud->action_panel_rect.y ());
    col = (cursor_pos.x () - hud->action_panel_rect.x ());
    if (row < 0 || col < 0)
        return false;
    row /= hud->action_button_size.height ();
    col /= hud->action_button_size.width ();
    return row < 3 && col < 5;
}
bool RoomWidget::getSelectionPanelUnitUnderCursor (const QPoint& cursor_pos, int& row, int& col) const
{
    row = (cursor_pos.y () - hud->selection_panel_icon_grid_pos.y ());
    col = (cursor_pos.x () - hud->selection_panel_icon_grid_pos.x ());
    if (row < 0 || col < 0)
        return false;
    row /= hud->selection_panel_icon_rib;
    col /= hud->selection_panel_icon_rib;
    return row < 3 && col < 10;
}
bool RoomWidget::getMinimapPositionUnderCursor (const QPoint& cursor_pos, QPointF& area_pos) const
{
    if (cursor_pos.x () >= hud->minimap_screen_area.left () &&
        cursor_pos.x () <= hud->minimap_screen_area.right () &&
        cursor_pos.y () >= hud->minimap_screen_area.top () &&
        cursor_pos.y () <= hud->minimap_screen_area.bottom ()) {
        const QRectF& area = match_state->areaRef ();
        area_pos = area.topLeft () + (cursor_pos - hud->minimap_screen_area.topLeft ()) * QPointF (area.width () / hud->minimap_screen_area.width (), area.height () / hud->minimap_screen_area.height ());
        return true;
    }
    return false;
}
bool RoomWidget::cursorIsAboveMajorMap (const QPoint& cursor_pos) const
{
    int margin_x2 = hud->margin * 2;
    if (cursor_pos.x () < hud->minimap_panel_rect.width () + margin_x2)
        return cursor_pos.y () < (coord_map.viewport_size.height () - hud->minimap_panel_rect.height () - margin_x2);
    else if (cursor_pos.x () >= (coord_map.viewport_size.width () - hud->action_panel_rect.width () - margin_x2))
        return cursor_pos.y () < (coord_map.viewport_size.height () - hud->action_panel_rect.height () - margin_x2);
    else
        return cursor_pos.y () < (coord_map.viewport_size.height () - hud->selection_panel_rect.height () - margin_x2);
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
void RoomWidget::drawHUD ()
{
    static const QColor stroke_color (0, 0, 0);
    static const QColor margin_color (0x2f, 0x90, 0x92);
    static const QColor panel_color (0x21, 0x2f, 0x3a);
    static const QColor panel_inner_color (0, 0xaa, 0xaa);

    int margin = hud->margin;

    {
        int area_w = hud->minimap_panel_rect.width () + margin * 2;
        int area_h = hud->minimap_panel_rect.height () + margin * 2;
        colored_renderer->fillRectangle (*this, 0, coord_map.viewport_size.height () - area_h, area_w, area_h, margin_color, ortho_matrix);
    }
    {
        int area_w = hud->action_panel_rect.width () + margin * 2;
        int area_h = hud->action_panel_rect.height () + margin * 2;
        colored_renderer->fillRectangle (*this, coord_map.viewport_size.width () - area_w, coord_map.viewport_size.height () - area_h, area_w, area_h, margin_color, ortho_matrix);
    }
    {
        colored_renderer->fillRectangle (*this,
                                         hud->minimap_panel_rect.width () + margin * 2, coord_map.viewport_size.height () - hud->selection_panel_rect.height () - margin * 2,
                                         coord_map.viewport_size.width () - hud->minimap_panel_rect.width () - hud->action_panel_rect.width () - margin * 4, hud->selection_panel_rect.height () + margin * 2,
                                         margin_color, ortho_matrix);
    }
    colored_renderer->fillRectangle (*this, hud->minimap_panel_rect, panel_color, ortho_matrix);
    minimap_renderer->draw (*this, *colored_renderer,
                            *hud, *match_state, team,
                            ortho_matrix, coord_map);
    colored_renderer->fillRectangle (*this, hud->selection_panel_rect, panel_color, ortho_matrix);
    {
        const qreal half_stroke_width = hud->stroke_width * 0.5;

        glLineWidth (hud->stroke_width);

        const GLfloat vertices[] = {
            GLfloat (margin - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud->minimap_panel_rect.height () - margin),
            GLfloat (margin + hud->minimap_panel_rect.width () + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud->minimap_panel_rect.height () - margin),
            GLfloat (margin - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (margin + hud->minimap_panel_rect.width () + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (margin),
            GLfloat (coord_map.viewport_size.height () - hud->minimap_panel_rect.height () - margin + half_stroke_width),
            GLfloat (margin),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),
            GLfloat (margin + hud->minimap_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - hud->minimap_panel_rect.height () - margin + half_stroke_width),
            GLfloat (margin + hud->minimap_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),

            GLfloat (margin * 2 + hud->minimap_panel_rect.width () - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud->selection_panel_rect.height () - margin),
            GLfloat (margin * 2 + hud->minimap_panel_rect.width () + hud->selection_panel_rect.width () + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud->selection_panel_rect.height () - margin),
            GLfloat (margin * 2 + hud->minimap_panel_rect.width () - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (margin * 2 + hud->minimap_panel_rect.width () + hud->selection_panel_rect.width () + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (margin * 2 + hud->minimap_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - hud->selection_panel_rect.height () - margin + half_stroke_width),
            GLfloat (margin * 2 + hud->minimap_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),
            GLfloat (margin * 2 + hud->minimap_panel_rect.width () + hud->selection_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - hud->selection_panel_rect.height () - margin + half_stroke_width),
            GLfloat (margin * 2 + hud->minimap_panel_rect.width () + hud->selection_panel_rect.width ()),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),

            GLfloat (coord_map.viewport_size.width () - hud->action_panel_rect.width () - margin - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud->action_panel_rect.height () - margin),
            GLfloat (coord_map.viewport_size.width () - margin + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud->action_panel_rect.height () - margin),
            GLfloat (coord_map.viewport_size.width () - hud->action_panel_rect.width () - margin - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (coord_map.viewport_size.width () - margin + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - margin),
            GLfloat (coord_map.viewport_size.width () - hud->action_panel_rect.width () - margin),
            GLfloat (coord_map.viewport_size.height () - hud->action_panel_rect.height () - margin + half_stroke_width),
            GLfloat (coord_map.viewport_size.width () - hud->action_panel_rect.width () - margin),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),
            GLfloat (coord_map.viewport_size.width () - margin),
            GLfloat (coord_map.viewport_size.height () - hud->action_panel_rect.height () - margin + half_stroke_width),
            GLfloat (coord_map.viewport_size.width () - margin),
            GLfloat (coord_map.viewport_size.height () - margin - half_stroke_width),

            GLfloat (0),
            GLfloat (coord_map.viewport_size.height () - hud->minimap_panel_rect.height () - margin * 2),
            GLfloat (hud->minimap_panel_rect.width () + margin * 2 + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud->minimap_panel_rect.height () - margin * 2),
            GLfloat (hud->minimap_panel_rect.width () + margin * 2),
            GLfloat (coord_map.viewport_size.height () - hud->minimap_panel_rect.height () - margin * 2 + half_stroke_width),
            GLfloat (hud->minimap_panel_rect.width () + margin * 2),
            GLfloat (coord_map.viewport_size.height () - hud->selection_panel_rect.height () - margin * 2 - half_stroke_width),
            GLfloat (hud->minimap_panel_rect.width () + margin * 2 - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud->selection_panel_rect.height () - margin * 2),
            GLfloat (coord_map.viewport_size.width () - hud->action_panel_rect.width () - margin * 2 + half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud->selection_panel_rect.height () - margin * 2),
            GLfloat (coord_map.viewport_size.width () - hud->action_panel_rect.width () - margin * 2),
            GLfloat (coord_map.viewport_size.height () - hud->selection_panel_rect.height () - margin * 2 - half_stroke_width),
            GLfloat (coord_map.viewport_size.width () - hud->action_panel_rect.width () - margin * 2),
            GLfloat (coord_map.viewport_size.height () - hud->action_panel_rect.height () - margin * 2 + half_stroke_width),
            GLfloat (coord_map.viewport_size.width () - hud->action_panel_rect.width () - margin * 2 - half_stroke_width),
            GLfloat (coord_map.viewport_size.height () - hud->action_panel_rect.height () - margin * 2),
            GLfloat (coord_map.viewport_size.width ()),
            GLfloat (coord_map.viewport_size.height () - hud->action_panel_rect.height () - margin * 2),
            GLfloat (coord_map.viewport_size.width ()),
            GLfloat (coord_map.viewport_size.height () - hud->action_panel_rect.height () - margin * 2 + half_stroke_width),
            GLfloat (coord_map.viewport_size.width ()),
            GLfloat (coord_map.viewport_size.height () - half_stroke_width),
            GLfloat (coord_map.viewport_size.width ()),
            GLfloat (coord_map.viewport_size.height ()),
            GLfloat (0),
            GLfloat (coord_map.viewport_size.height ()),
            GLfloat (0),
            GLfloat (coord_map.viewport_size.height () - half_stroke_width),
            GLfloat (0),
            GLfloat (coord_map.viewport_size.height () - hud->minimap_panel_rect.height () - margin * 2 + half_stroke_width),
        };

        const GLfloat colors[] = {
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),

            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),

            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),

            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
            GLfloat (stroke_color.redF ()),
            GLfloat (stroke_color.greenF ()),
            GLfloat (stroke_color.blueF ()),
            GLfloat (1),
        };

        colored_renderer->draw (*this, GL_LINES, 40, vertices, colors, ortho_matrix);

        glLineWidth (1);
    }

    size_t selected_count = 0;
    bool contaminator_selected = false;
    qint64 cast_cooldown_left_ticks = 0x7fffffffffffffffLL;
    quint64 active_actions = 0;
    const Unit* last_selected_unit = nullptr;
    const QHash<quint32, Unit>& units = match_state->unitsRef ();
    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it) {
        if (it->selected) {
            if (std::holds_alternative<AttackAction> (it->action)) {
                active_actions |= 1 << quint64 (ActionButtonId::Attack);
            } else if (std::holds_alternative<MoveAction> (it->action)) {
                active_actions |= 1 << quint64 (ActionButtonId::Move);
            } else if (std::holds_alternative<CastAction> (it->action)) {
                switch (std::get<CastAction> (it->action).type) {
                case CastAction::Type::Pestilence:
                    active_actions |= 1 << quint64 (ActionButtonId::Pestilence);
                    break;
                case CastAction::Type::SpawnBeetle:
                    active_actions |= 1 << quint64 (ActionButtonId::Spawn);
                    break;
                default:
                    break;
                }
            } else {
                active_actions |= 1 << quint64 (ActionButtonId::Stop);
            }

            if (it->type == Unit::Type::Contaminator) {
                contaminator_selected = true;
                cast_cooldown_left_ticks = qMin (cast_cooldown_left_ticks, it->cast_cooldown_left_ticks);
            }
            ++selected_count;
            last_selected_unit = &*it;
        }
    }

    drawSelectionPanel (hud->selection_panel_rect, selected_count, last_selected_unit);
    drawGroupsOverlay ();
    action_panel_renderer->draw (*this, *colored_renderer, *textured_renderer,
                                 *hud, margin, panel_color, selected_count, active_actions, contaminator_selected, cast_cooldown_left_ticks,
                                 ortho_matrix, coord_map);
}
void RoomWidget::drawSelectionPanel (const QRect& rect, size_t selected_count, const Unit* last_selected_unit)
{
    if (selected_count == 1) {
        const Unit& unit = *last_selected_unit;
        const AttackDescription& primary_attack_description = match_state->unitPrimaryAttackDescription (unit.type);

        {
            int margin = rect.height () * 0.2;
            int icon_rib = rect.height () - margin * 2;
            drawIcon (unit, rect.x () + margin, rect.y () + margin, icon_rib, icon_rib);
        }

        {
            int margin = rect.height () * 0.05;
            QPainter p (this);
            p.setFont (font);
            p.setPen (QColor (0xdf, 0xdf, 0xff));
            p.drawText (rect.marginsRemoved ({margin, margin, margin, margin}), Qt::AlignHCenter,
                        unitTitle (unit.type) + "\n"
                                                "\n"
                                                "HP: " +
                            QString::number (unit.hp) + "/" + QString::number (match_state->unitMaxHP (unit.type)) + "\n"
                                                                                                                     "Attack: class = " +
                            unitAttackClassName (primary_attack_description.type) + "; range = " + QString::number (primary_attack_description.range));
        }
    } else if (selected_count) {
        int icon_rib = hud->selection_panel_icon_rib;

        QVector<QPair<quint32, const Unit*>> selection = buildOrderedSelection ();

        if (selection.size () > 30)
            drawTabs (hud->selection_panel_icon_grid_pos.x () - icon_rib, hud->selection_panel_icon_grid_pos.y (), icon_rib, icon_rib);

        for (size_t i = 0; i < size_t (qMin (selection.size (), 30)); ++i) {
            const Unit* unit = selection[i].second;
            size_t row = i / 10;
            size_t col = i % 10;
            drawIcon (*unit, hud->selection_panel_icon_grid_pos.x () + icon_rib * col, hud->selection_panel_icon_grid_pos.y () + icon_rib * row, icon_rib, icon_rib, true);
        }
    }
}
void RoomWidget::drawGroupsOverlay ()
{
    qint64 group_sizes[group_count] = {};
    Unit::Type group_unit_counts[group_count];
    for (qint64 i = 0; i < group_count; ++i)
        group_unit_counts[i] = Unit::Type::Beetle;
    const QHash<quint32, Unit>& units = match_state->unitsRef ();
    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it) {
        quint64 groups = it->groups;
        for (qint64 i = 0; i < group_count; ++i) {
            if (groups & (1 << (i + 1))) {
                group_sizes[i]++;
                group_unit_counts[i] = qMax (group_unit_counts[i], it->type);
            }
        }
    }
    static constexpr QColor group_info_rect_color (0, 0x44, 0);
    static constexpr QColor group_number_rect_color (0x44, 0, 0);
    static constexpr QColor group_border_color (0, 0xff, 0);
    static constexpr QColor group_shade_color (0, 0, 0, 0x22);
    static constexpr QColor group_text_color (0xdf, 0xdf, 0xff);
    int icon_rib = hud->selection_panel_icon_rib;
    QSizeF info_rect_size (icon_rib*0.8, icon_rib*0.4);
    QSizeF number_rect_size (icon_rib*0.6, icon_rib*0.3);
    int group_icon_rib = hud->margin*5/3;
    QSize group_icon_size (group_icon_rib, group_icon_rib);
    {
        for (qint64 col = 0; col < group_count; ++col) {
            qreal number_rect_y = hud->selection_panel_rect.y () - hud->margin*0.5;
            qreal x_center = hud->selection_panel_icon_grid_pos.x () + icon_rib*(col - 5 + 0.5);
            qreal number_rect_y_center = number_rect_y - number_rect_size.height ()*0.5;
            QRectF info_rect = {x_center - info_rect_size.width ()*0.5, number_rect_y_center - info_rect_size.height () + 1, info_rect_size.width (), info_rect_size.height ()};
            QRectF number_rect = {x_center - number_rect_size.width ()*0.5, number_rect_y_center, number_rect_size.width (), number_rect_size.height ()};
            if (group_sizes[col]) {
                colored_renderer->fillRectangle (*this, info_rect, group_info_rect_color, ortho_matrix);
                colored_renderer->fillRectangle (*this, number_rect, group_number_rect_color, ortho_matrix);
                colored_renderer->drawRectangle (*this, info_rect, group_border_color, ortho_matrix);
                colored_renderer->drawRectangle (*this, number_rect, group_border_color, ortho_matrix);
                drawIcon (group_unit_counts[col], 1.0, info_rect.x (), info_rect.y (), group_icon_size.width (), group_icon_size.height ());
            } else {
                colored_renderer->fillRectangle (*this, info_rect, group_shade_color, ortho_matrix);
            }
        }
    }
    {
        int icon_rib = hud->selection_panel_icon_rib;

        QPainter p (this);
        p.setPen (group_text_color);
        for (qint64 col = 0; col < group_count; ++col) {
            if (group_sizes[col]) {
                qreal number_rect_y = hud->selection_panel_rect.y () - hud->margin*0.5;
                qreal info_rect_w = icon_rib*0.8;
                qreal info_rect_h = info_rect_w*0.5;
                qreal number_rect_w = icon_rib*0.6;
                qreal number_rect_h = number_rect_w*0.5;
                qreal x_center = hud->selection_panel_icon_grid_pos.x () + icon_rib * (col - 5 + 0.5);
                qreal number_rect_y_center = number_rect_y - number_rect_h*0.5;
                QRectF info_rect = QRectF (x_center - info_rect_w*0.5, number_rect_y_center - info_rect_h + 1, info_rect_w, info_rect_h).marginsRemoved ({3, 3, 3, 3});
                QRectF number_rect = {x_center - number_rect_w*0.5, number_rect_y_center, number_rect_w, number_rect_h};
                p.setFont (group_size_font);
                p.drawText (info_rect, Qt::AlignRight | Qt::AlignVCenter, QString::number (group_sizes[col]));
                p.setFont (group_number_font);
                p.drawText (number_rect, Qt::AlignHCenter | Qt::AlignVCenter, QString::number (col + 1));
            }
        }
    }
}
void RoomWidget::drawActionPanel (int margin, const QColor& panel_color, int selected_count, quint64 active_actions, bool contaminator_selected, qint64 cast_cooldown_left_ticks)
{
    action_panel_renderer->draw (*this, *colored_renderer, *textured_renderer,
                                 *hud, margin, panel_color, selected_count, active_actions, contaminator_selected, cast_cooldown_left_ticks,
                                 ortho_matrix, coord_map);
}
void RoomWidget::drawUnit (const Unit& unit)
{
    unit_set_renderer->draw (*this, *colored_renderer, *textured_renderer, unit, match_state->clockNS (), ortho_matrix, coord_map);
}
void RoomWidget::drawTabs (int x, int y, int w, int h)
{
    const GLfloat vertices[] = {
        GLfloat (x),
        GLfloat (y + h),
        GLfloat (x),
        GLfloat (y),
        GLfloat (x + w),
        GLfloat (y),
        GLfloat (x + w),
        GLfloat (y + h),
    };

    static const GLuint indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    static const GLfloat texture_coords[] = {
        0.0,
        0.0,
        1.0,
        0.0,
        1.0,
        1.0,
        0.0,
        1.0,
    };

    QColor color = QColor::fromRgbF (1.0, 1.0, 1.0, 1.0);
    const GLfloat colors[] = {
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
    };
    colored_textured_renderer->draw (*this, GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, textures.unit_icons.tabs.get (), ortho_matrix);
}
void RoomWidget::drawIcon (const Unit::Type& unit_type, qreal hp_ratio, qreal x, qreal y, qreal w, qreal h, bool framed)
{
    qreal sprite_scale;
    QOpenGLTexture* texture;
    switch (unit_type) {
    case Unit::Type::Seal:
        sprite_scale = 0.8;
        texture = textures.unit_icons.seal.get ();
        break;
    case Unit::Type::Crusader:
        sprite_scale = 1.1;
        texture = textures.unit_icons.crusader.get ();
        break;
    case Unit::Type::Goon:
        sprite_scale = 0.9;
        texture = textures.unit_icons.goon.get ();
        break;
    case Unit::Type::Beetle:
        sprite_scale = 0.6;
        texture = textures.unit_icons.beetle.get ();
        break;
    case Unit::Type::Contaminator:
        sprite_scale = 1.2;
        texture = textures.unit_icons.contaminator.get ();
        break;
    default:
        return;
    }

    const GLfloat vertices[] = {
        GLfloat (x + w*(0.5 - 0.5*sprite_scale)),
        GLfloat (y + h*(0.5 + 0.5*sprite_scale)),
        GLfloat (x + w*(0.5 - 0.5*sprite_scale)),
        GLfloat (y + h*(0.5 - 0.5*sprite_scale)),
        GLfloat (x + w*(0.5 + 0.5*sprite_scale)),
        GLfloat (y + h*(0.5 - 0.5*sprite_scale)),
        GLfloat (x + w*(0.5 + 0.5*sprite_scale)),
        GLfloat (y + h*(0.5 + 0.5*sprite_scale)),
    };

    static const GLfloat texture_coords[] = {
        GLfloat (0),
        GLfloat (0),
        GLfloat (1),
        GLfloat (0),
        GLfloat (1),
        GLfloat (1),
        GLfloat (0),
        GLfloat (1),
    };

    QColor color = getHPColor (hp_ratio);

    const GLfloat colors[] = {
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
        GLfloat (color.redF ()),
        GLfloat (color.greenF ()),
        GLfloat (color.blueF ()),
        GLfloat (color.alphaF ()),
    };

    static const GLuint indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    if (framed) {
        const GLfloat vertices[] = {
            GLfloat (x),
            GLfloat (y + h),
            GLfloat (x),
            GLfloat (y),
            GLfloat (x + w),
            GLfloat (y),
            GLfloat (x + w),
            GLfloat (y + h),
        };
        static const GLfloat texture_coords[] = {
            0,
            0,
            1,
            0,
            1,
            1,
            0,
            1,
        };
        QColor color = QColor::fromRgbF (1.0, 1.0, 1.0, 1.0);
        const GLfloat colors[] = {
            GLfloat (color.redF ()),
            GLfloat (color.greenF ()),
            GLfloat (color.blueF ()),
            GLfloat (color.alphaF ()),
            GLfloat (color.redF ()),
            GLfloat (color.greenF ()),
            GLfloat (color.blueF ()),
            GLfloat (color.alphaF ()),
            GLfloat (color.redF ()),
            GLfloat (color.greenF ()),
            GLfloat (color.blueF ()),
            GLfloat (color.alphaF ()),
            GLfloat (color.redF ()),
            GLfloat (color.greenF ()),
            GLfloat (color.blueF ()),
            GLfloat (color.alphaF ()),
        };
        colored_textured_renderer->draw (*this, GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, textures.unit_icons.frame.get (), ortho_matrix);
    }

    colored_textured_renderer->draw (*this, GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, texture, ortho_matrix);
}
void RoomWidget::drawIcon (const Unit& unit, qreal x, qreal y, qreal w, qreal h, bool framed)
{
    qreal hp_ratio = qreal (unit.hp) / match_state->unitMaxHP (unit.type);
    drawIcon (unit.type, hp_ratio, x, y, w, h, framed);
}
void RoomWidget::drawUnitHPBar (const Unit& unit)
{
    if (unit.hp < match_state->unitMaxHP (unit.type)) {
        QPointF center = coord_map.toScreenCoords (unit.position);
        qreal hp_ratio = qreal (unit.hp) / match_state->unitMaxHP (unit.type);
        qreal hitbar_height = coord_map.viewport_scale * MAP_TO_SCREEN_FACTOR * 0.16;
        qreal radius = coord_map.viewport_scale * match_state->unitDiameter (unit.type) * MAP_TO_SCREEN_FACTOR * 0.42;

        {
            const GLfloat vertices[] = {
                GLfloat (center.x () - radius),
                GLfloat (center.y () - radius),
                GLfloat (center.x () + radius * (-1.0 + hp_ratio * 2.0)),
                GLfloat (center.y () - radius),
                GLfloat (center.x () + radius * (-1.0 + hp_ratio * 2.0)),
                GLfloat (center.y () - radius - hitbar_height),
                GLfloat (center.x () - radius),
                GLfloat (center.y () - radius - hitbar_height),
            };

            QColor color = getHPColor (hp_ratio);
            color.setRgbF (color.redF (), color.greenF (), color.blueF (), color.alphaF ());

            const GLfloat colors[] = {
                GLfloat (color.redF ()),
                GLfloat (color.greenF ()),
                GLfloat (color.blueF ()),
                GLfloat (color.alphaF ()),
                GLfloat (color.redF ()),
                GLfloat (color.greenF ()),
                GLfloat (color.blueF ()),
                GLfloat (color.alphaF ()),
                GLfloat (color.redF ()),
                GLfloat (color.greenF ()),
                GLfloat (color.blueF ()),
                GLfloat (color.alphaF ()),
                GLfloat (color.redF ()),
                GLfloat (color.greenF ()),
                GLfloat (color.blueF ()),
                GLfloat (color.alphaF ()),
            };

            colored_renderer->draw (*this, GL_TRIANGLE_FAN, 4, vertices, colors, ortho_matrix);
        }

        {
            const GLfloat vertices[] = {
                GLfloat (center.x () - radius),
                GLfloat (center.y () - radius),
                GLfloat (center.x () + radius),
                GLfloat (center.y () - radius),
                GLfloat (center.x () + radius),
                GLfloat (center.y () - radius - hitbar_height),
                GLfloat (center.x () - radius),
                GLfloat (center.y () - radius - hitbar_height),
            };

            static const GLfloat colors[] = {
                0,
                0.8,
                0.8,
                1,
                0,
                0.8,
                0.8,
                1,
                0,
                0.8,
                0.8,
                1,
                0,
                0.8,
                0.8,
                1,
            };

            colored_renderer->draw (*this, GL_LINE_LOOP, 4, vertices, colors, ortho_matrix);
        }
    }
}
void RoomWidget::drawUnitPathToTarget (const Unit& unit)
{
    const QPointF* target_position = nullptr;
    if (std::holds_alternative<MoveAction> (unit.action)) {
        const std::variant<QPointF, quint32>& action_target = std::get<MoveAction> (unit.action).target;
        if (std::holds_alternative<QPointF> (action_target)) {
            target_position = &std::get<QPointF> (action_target);
        } else if (std::holds_alternative<quint32> (action_target)) {
            quint32 target_unit_id = std::get<quint32> (action_target);
            const QHash<quint32, Unit>& units = match_state->unitsRef ();
            QHash<quint32, Unit>::const_iterator target_unit_it = units.find (target_unit_id);
            if (target_unit_it != units.end ()) {
                const Unit& target_unit = *target_unit_it;
                target_position = &target_unit.position;
            }
        }
    } else if (std::holds_alternative<AttackAction> (unit.action)) {
        const std::variant<QPointF, quint32>& action_target = std::get<AttackAction> (unit.action).target;
        if (std::holds_alternative<QPointF> (action_target)) {
            target_position = &std::get<QPointF> (action_target);
        } else if (std::holds_alternative<quint32> (action_target)) {
            quint32 target_unit_id = std::get<quint32> (action_target);
            const QHash<quint32, Unit>& units = match_state->unitsRef ();
            QHash<quint32, Unit>::const_iterator target_unit_it = units.find (target_unit_id);
            if (target_unit_it != units.end ()) {
                const Unit& target_unit = *target_unit_it;
                target_position = &target_unit.position;
            }
        }
    }

    if (!target_position)
        return;

    QPointF current = coord_map.toScreenCoords (unit.position);
    QPointF target = coord_map.toScreenCoords (*target_position);

    const GLfloat vertices[] = {
        GLfloat (current.x ()),
        GLfloat (current.y ()),
        GLfloat (target.x ()),
        GLfloat (target.y ()),
    };

    static const GLfloat attack_colors[] = {
        1,
        0,
        0,
        1,
        1,
        0,
        0,
        1,
    };

    static const GLfloat move_colors[] = {
        0,
        1,
        0,
        1,
        0,
        1,
        0,
        1,
    };

    colored_renderer->draw (*this, GL_LINES, 2, vertices, std::holds_alternative<AttackAction> (unit.action) ? attack_colors : move_colors, ortho_matrix);
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
QVector<QPair<quint32, const Unit*>> RoomWidget::buildOrderedSelection ()
{
    QVector<QPair<quint32, const Unit*>> selection;

    const QHash<quint32, Unit>& units = match_state->unitsRef ();

    static const Unit::Type unit_order[] = {
        Unit::Type::Contaminator,
        Unit::Type::Crusader,
        Unit::Type::Goon,
        Unit::Type::Seal,
        Unit::Type::Beetle,
    };

    for (const Unit::Type unit_type: unit_order) {
        for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it) {
            if (it->selected && it->type == unit_type) {
                selection.append ({it.key (), &*it});
            }
        }
    }

    return selection;
}
