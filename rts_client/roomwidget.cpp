#include "roomwidget.h"

#include "matchstate.h"

#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QSound>
#include <QGuiApplication>
#include <math.h>
#include <mutex>


static constexpr qreal SQRT_2 = 1.4142135623731;
static constexpr qreal SQRT_1_2 = 0.70710678118655;
static constexpr qreal PI_X_3_4 = 3.0/4.0*M_PI;
static constexpr qreal PI_X_1_4 = 1.0/4.0*M_PI;


static constexpr qreal POINTS_PER_VIEWPORT_VERTICALLY = 20.0; // At zoom x1.0
#define POINTS_VIEWPORT_HEIGHT 20
#define MAP_TO_SCREEN_FACTOR (arena_viewport.height ()/POINTS_PER_VIEWPORT_VERTICALLY)


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

RoomWidget::RoomWidget (QWidget* parent)
    : OpenGLWidget (parent), font ("NotCourier", 16)
{
    setAttribute (Qt::WA_KeyCompression, false);
    font.setStyleHint (QFont::TypeWriter);

    last_frame.invalidate ();
    setMouseTracking (true);
    setCursor (QCursor (Qt::BlankCursor));
    connect (&match_timer, SIGNAL (timeout ()), this, SLOT (tick ()));
    connect(this, &RoomWidget::joinRedTeamRequested, this, &RoomWidget::joinRedTeamRequestedHandler);
    connect(this, &RoomWidget::joinBlueTeamRequested, this, &RoomWidget::joinBlueTeamRequestedHandler);
    connect(this, &RoomWidget::spectateRequested, this, &RoomWidget::spectateRequestedHandler);
    connect(this, &RoomWidget::cancelJoinTeamRequested, this, &RoomWidget::cancelJoinTeamRequestedHandler);
    connect(this, &RoomWidget::quitRequested, this, &RoomWidget::quitRequestedHandler);
    connect(this, &RoomWidget::readinessRequested, this, &RoomWidget::readinessRequestedHandler);
}
RoomWidget::~RoomWidget ()
{
}

void RoomWidget::joinRedTeamRequestedHandler () {
    awaitTeamSelection (Unit::Team::Red);
}
void RoomWidget::joinBlueTeamRequestedHandler () {
    awaitTeamSelection (Unit::Team::Blue);
}
void RoomWidget::spectateRequestedHandler () {
    awaitTeamSelection (Unit::Team::Spectator);
}
void RoomWidget::cancelJoinTeamRequestedHandler () {
    state = State::TeamSelection;
}
void RoomWidget::readinessRequestedHandler () {
    ready (this->team);
}
void RoomWidget::readinessHandler () {
    queryReadiness (this->team);
}
void RoomWidget::quitRequestedHandler () {
    return;
}

void RoomWidget::startCountDownHandler () {
    awaitMatch (this->team);
}

void RoomWidget::startMatchHandler() {
    qDebug () << "Start match handler started";
    startMatch (this->team);
    emit createUnitRequested();
}
void RoomWidget::awaitTeamSelection (Unit::Team team)
{
    this->team = team;
    pressed_button = ButtonId::None;
    match_timer.stop ();
    state = State::TeamSelectionRequested;
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
    match_state = QSharedPointer<MatchState> (new MatchState);
    connect (&*match_state, SIGNAL (soundEventEmitted (SoundEvent)), this, SLOT (playSound (SoundEvent)));
    viewport_scale_power = 0;
    viewport_scale = 1.0;
    viewport_center = {};
    match_timer.start (20);
    last_frame.restart ();
    state = State::MatchStarted;
}
void RoomWidget::loadMatchState (QVector<QPair<quint32, Unit> > units, const QVector<QPair<quint32, quint32> >& to_delete) {
    match_state->LoadState(units, to_delete);
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
quint64 RoomWidget::explosionAnimationPeriodNS ()
{
    return 400'000'000;
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

    textures.buttons.join_red_team = loadTexture2DRectangle (":/images/buttons/join-red-team.png");
    textures.buttons.join_blue_team = loadTexture2DRectangle (":/images/buttons/join-blue-team.png");
    textures.buttons.spectate = loadTexture2DRectangle (":/images/buttons/spectate.png");
    textures.buttons.ready = loadTexture2DRectangle (":/images/buttons/ready.png");
    textures.buttons.cancel = loadTexture2DRectangle (":/images/buttons/cancel.png");
    textures.buttons.quit = loadTexture2DRectangle (":/images/buttons/quit.png");
    textures.buttons.join_red_team_pressed = loadTexture2DRectangle (":/images/buttons/join-red-team-pressed.png");
    textures.buttons.join_blue_team_pressed = loadTexture2DRectangle (":/images/buttons/join-blue-team-pressed.png");
    textures.buttons.spectate_pressed = loadTexture2DRectangle (":/images/buttons/spectate-pressed.png");
    textures.buttons.ready_pressed = loadTexture2DRectangle (":/images/buttons/ready-pressed.png");
    textures.buttons.cancel_pressed = loadTexture2DRectangle (":/images/buttons/cancel-pressed.png");
    textures.buttons.quit_pressed = loadTexture2DRectangle (":/images/buttons/quit-pressed.png");

    {
        textures.units.seal.red.standing = loadTexture2D (":/images/units/seal/red/standing.png", true);
        textures.units.seal.red.walking1 = loadTexture2D (":/images/units/seal/red/walking1.png", true);
        textures.units.seal.red.walking2 = loadTexture2D (":/images/units/seal/red/walking2.png", true);
        textures.units.seal.red.shooting1 = loadTexture2D (":/images/units/seal/red/attacking1.png", true);
        textures.units.seal.red.shooting2 = loadTexture2D (":/images/units/seal/red/attacking2.png", true);

        textures.units.seal.blue.standing = loadTexture2D (":/images/units/seal/blue/standing.png", true);
        textures.units.seal.blue.walking1 = loadTexture2D (":/images/units/seal/blue/walking1.png", true);
        textures.units.seal.blue.walking2 = loadTexture2D (":/images/units/seal/blue/walking2.png", true);
        textures.units.seal.blue.shooting1 = loadTexture2D (":/images/units/seal/blue/attacking1.png", true);
        textures.units.seal.blue.shooting2 = loadTexture2D (":/images/units/seal/blue/attacking2.png", true);
    }
    {
        textures.units.crusader.red.standing = loadTexture2D (":/images/units/crusader/red/standing.png", true);
        textures.units.crusader.red.walking1 = loadTexture2D (":/images/units/crusader/red/walking1.png", true);
        textures.units.crusader.red.walking2 = loadTexture2D (":/images/units/crusader/red/walking2.png", true);
        textures.units.crusader.red.shooting1 = loadTexture2D (":/images/units/crusader/red/attacking1.png", true);
        textures.units.crusader.red.shooting2 = loadTexture2D (":/images/units/crusader/red/attacking2.png", true);

        textures.units.crusader.blue.standing = loadTexture2D (":/images/units/crusader/blue/standing.png", true);
        textures.units.crusader.blue.walking1 = loadTexture2D (":/images/units/crusader/blue/walking1.png", true);
        textures.units.crusader.blue.walking2 = loadTexture2D (":/images/units/crusader/blue/walking2.png", true);
        textures.units.crusader.blue.shooting1 = loadTexture2D (":/images/units/crusader/blue/attacking1.png", true);
        textures.units.crusader.blue.shooting2 = loadTexture2D (":/images/units/crusader/blue/attacking2.png", true);
    }
    {
        textures.units.goon.red.standing = loadTexture2D (":/images/units/goon/red/standing.png", true);
        textures.units.goon.red.walking1 = loadTexture2D (":/images/units/goon/red/walking1.png", true);
        textures.units.goon.red.walking2 = loadTexture2D (":/images/units/goon/red/walking2.png", true);
        textures.units.goon.red.shooting1 = loadTexture2D (":/images/units/goon/red/attacking1.png", true);
        textures.units.goon.red.shooting2 = loadTexture2D (":/images/units/goon/red/attacking2.png", true);

        textures.units.goon.blue.standing = loadTexture2D (":/images/units/goon/blue/standing.png", true);
        textures.units.goon.blue.walking1 = loadTexture2D (":/images/units/goon/blue/walking1.png", true);
        textures.units.goon.blue.walking2 = loadTexture2D (":/images/units/goon/blue/walking2.png", true);
        textures.units.goon.blue.shooting1 = loadTexture2D (":/images/units/goon/blue/attacking1.png", true);
        textures.units.goon.blue.shooting2 = loadTexture2D (":/images/units/goon/blue/attacking2.png", true);
    }
    {
        textures.units.beetle.red.standing = loadTexture2D (":/images/units/beetle/red/standing.png", true);
        textures.units.beetle.red.walking1 = loadTexture2D (":/images/units/beetle/red/walking1.png", true);
        textures.units.beetle.red.walking2 = loadTexture2D (":/images/units/beetle/red/walking2.png", true);
        textures.units.beetle.red.shooting1 = loadTexture2D (":/images/units/beetle/red/attacking1.png", true);
        textures.units.beetle.red.shooting2 = loadTexture2D (":/images/units/beetle/red/attacking2.png", true);

        textures.units.beetle.blue.standing = loadTexture2D (":/images/units/beetle/blue/standing.png", true);
        textures.units.beetle.blue.walking1 = loadTexture2D (":/images/units/beetle/blue/walking1.png", true);
        textures.units.beetle.blue.walking2 = loadTexture2D (":/images/units/beetle/blue/walking2.png", true);
        textures.units.beetle.blue.shooting1 = loadTexture2D (":/images/units/beetle/blue/attacking1.png", true);
        textures.units.beetle.blue.shooting2 = loadTexture2D (":/images/units/beetle/blue/attacking2.png", true);
    }
    {
        textures.units.contaminator.red.standing = loadTexture2D (":/images/units/contaminator/red/standing.png", true);
        textures.units.contaminator.red.walking1 = loadTexture2D (":/images/units/contaminator/red/walking1.png", true);
        textures.units.contaminator.red.walking2 = loadTexture2D (":/images/units/contaminator/red/walking2.png", true);
        textures.units.contaminator.red.shooting1 = loadTexture2D (":/images/units/contaminator/red/attacking1.png", true);
        textures.units.contaminator.red.shooting2 = loadTexture2D (":/images/units/contaminator/red/attacking2.png", true);

        textures.units.contaminator.blue.standing = loadTexture2D (":/images/units/contaminator/blue/standing.png", true);
        textures.units.contaminator.blue.walking1 = loadTexture2D (":/images/units/contaminator/blue/walking1.png", true);
        textures.units.contaminator.blue.walking2 = loadTexture2D (":/images/units/contaminator/blue/walking2.png", true);
        textures.units.contaminator.blue.shooting1 = loadTexture2D (":/images/units/contaminator/blue/attacking1.png", true);
        textures.units.contaminator.blue.shooting2 = loadTexture2D (":/images/units/contaminator/blue/attacking2.png", true);
    }
    {
        textures.effects.explosion.explosion1 = loadTexture2D (":/images/effects/explosion/explosion1.png", true);
        textures.effects.explosion.explosion2 = loadTexture2D (":/images/effects/explosion/explosion2.png", true);
        textures.effects.goon_rocket.rocket1 = loadTexture2D (":/images/effects/goon-rocket/rocket1.png", true);
        textures.effects.goon_rocket.rocket2 = loadTexture2D (":/images/effects/goon-rocket/rocket2.png", true);
        textures.effects.pestilence_missile.missile1 = loadTexture2D (":/images/effects/pestilence-missile/missile1.png", true);
        textures.effects.pestilence_missile.missile2 = loadTexture2D (":/images/effects/pestilence-missile/missile2.png", true);
        textures.effects.pestilence_splash.splash = loadTexture2D (":/images/effects/pestilence-splash/splash.png", true);
    }
}
void RoomWidget::initResources ()
{
    loadTextures ();

    cursor = textures.cursors.crosshair;
}
void RoomWidget::updateSize (int w, int h)
{
    this->w = w;
    this->h = h;
    arena_viewport = {0, 0, w, h - 220};
    arena_viewport_center = QRectF (arena_viewport).center ();
    map_to_screen_factor = arena_viewport.height ()/POINTS_PER_VIEWPORT_VERTICALLY;
}
void RoomWidget::draw ()
{
    if (cursor_position.x () == -1 && cursor_position.y () == -1)
        cursor_position = QWidget::mapFromGlobal (QCursor::pos ());

    switch (state) {
    case State::TeamSelection:
        drawTeamSelection ();
        break;
    case State::TeamSelectionRequested:
        drawTeamSelectionRequested ();
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

    drawRectangle (cursor_position.x () - cursor->width ()/2, cursor_position.y () - cursor->height ()/2, cursor.get ());
}
void RoomWidget::keyPressEvent (QKeyEvent *event)
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
void RoomWidget::keyReleaseEvent (QKeyEvent *event)
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
void RoomWidget::mouseMoveEvent (QMouseEvent *event)
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
void RoomWidget::mousePressEvent (QMouseEvent *event)
{
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers ();
    ctrl_pressed = modifiers & Qt::ControlModifier;
    shift_pressed = modifiers & Qt::ShiftModifier;
    alt_pressed = modifiers & Qt::AltModifier;
    switch (state) {
    case State::TeamSelection: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 30), textures.buttons.join_blue_team))
                pressed_button = ButtonId::JoinBlueTeam;
            else if (pointInsideButton (cursor_position, QPoint (30, 130), textures.buttons.join_red_team))
                pressed_button = ButtonId::JoinRedTeam;
            else if (pointInsideButton (cursor_position, QPoint (30, 230), textures.buttons.spectate))
                pressed_button = ButtonId::Spectate;
            else if (pointInsideButton (cursor_position, QPoint (30, 400), textures.buttons.quit))
                pressed_button = ButtonId::Quit;
        }
    } break;
    case State::TeamSelectionRequested: {
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
void RoomWidget::mouseReleaseEvent (QMouseEvent *event)
{
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers ();
    ctrl_pressed = modifiers & Qt::ControlModifier;
    shift_pressed = modifiers & Qt::ShiftModifier;
    alt_pressed = modifiers & Qt::AltModifier;
    switch (state) {
    case State::TeamSelection: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 30), textures.buttons.join_blue_team)) {
                if (pressed_button == ButtonId::JoinBlueTeam)
                    emit joinBlueTeamRequested ();
            } else if (pointInsideButton (cursor_position, QPoint (30, 130), textures.buttons.join_red_team)) {
                if (pressed_button == ButtonId::JoinRedTeam)
                    emit joinRedTeamRequested ();
            } else if (pointInsideButton (cursor_position, QPoint (30, 230), textures.buttons.spectate)) {
                if (pressed_button == ButtonId::Spectate)
                    emit spectateRequested ();
            } else if (pointInsideButton (cursor_position, QPoint (30, 400), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit)
                    emit quitRequested ();
            }
        }
    } break;
    case State::TeamSelectionRequested: {
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
void RoomWidget::wheelEvent (QWheelEvent *event)
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
void RoomWidget::matchKeyPressEvent (QKeyEvent *event)
{
    switch (event->key ()) {
    case Qt::Key_A:
        match_state->attackEnemy (team, toMapCoords (cursor_position));
        break;
    case Qt::Key_E:
        match_state->cast (CastAction::Type::Pestilence, team, toMapCoords (cursor_position));
        break;
    case Qt::Key_T:
        match_state->cast (CastAction::Type::SpawnBeetle, team, toMapCoords (cursor_position));
        break;
    case Qt::Key_G:
        match_state->move (toMapCoords (cursor_position));
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
    }
}
void RoomWidget::matchKeyReleaseEvent (QKeyEvent *event)
{
    switch (event->key ()) {
    }
}
void RoomWidget::matchMouseMoveEvent (QMouseEvent* /* event */)
{
}
void RoomWidget::matchMousePressEvent (QMouseEvent *event)
{
    switch (event->button ()) {
    case Qt::LeftButton: {
        if (ctrl_pressed) {
            match_state->trySelectByType (team, toMapCoords (cursor_position), toMapCoords (arena_viewport), shift_pressed);
        } else {
            selection_start = cursor_position;
        }
    } break;
    case Qt::RightButton: {
        match_state->autoAction (team, toMapCoords (cursor_position));
    } break;
    default: {
    }
    }
}
void RoomWidget::matchMouseReleaseEvent (QMouseEvent *event)
{
    switch (event->button ()) {
    case Qt::LeftButton: {
        if (selection_start.has_value ()) {
            if (*selection_start == cursor_position) {
                match_state->trySelect (team, toMapCoords (cursor_position), shift_pressed);
            } else {
                QPointF p1 = toMapCoords (*selection_start);
                QPointF p2 = toMapCoords (cursor_position);
                match_state->trySelect (team, {qMin (p1.x (), p2.x ()), qMin (p1.y (), p2.y ()), qAbs (p1.x () - p2.x ()), qAbs (p1.y () - p2.y ())}, shift_pressed);
            }
            selection_start.reset ();
        }
    } break;
    default: {
    }
    }
}
void RoomWidget::matchWheelEvent (QWheelEvent *event)
{
    int dy = (event->angleDelta ().y () > 0) ? 1 : -1;
    viewport_scale_power += dy;
    viewport_scale_power = qBound (-10, viewport_scale_power, 10);
    viewport_scale = pow (1.125, viewport_scale_power);
}
void RoomWidget::drawTeamSelection ()
{
    drawRectangle (0, 0, w, h, textures.grass.get ());

    drawRectangle (30, 30, (pressed_button == ButtonId::JoinBlueTeam) ? textures.buttons.join_blue_team_pressed.get () : textures.buttons.join_blue_team.get ());
    drawRectangle (30, 130, (pressed_button == ButtonId::JoinRedTeam) ? textures.buttons.join_red_team_pressed.get () : textures.buttons.join_red_team.get ());
    drawRectangle (30, 230, (pressed_button == ButtonId::Spectate) ? textures.buttons.spectate_pressed.get () : textures.buttons.spectate.get ());
    drawRectangle (30, 400, (pressed_button == ButtonId::Quit) ? textures.buttons.quit_pressed.get () : textures.buttons.quit.get ());
}
void RoomWidget::drawTeamSelectionRequested ()
{
    drawRectangle (0, 0, w, h, textures.grass.get ());

    drawRectangle (30, 30, textures.labels.join_team_requested.get ());

    drawRectangle (30, 230, (pressed_button == ButtonId::Cancel) ? textures.buttons.cancel_pressed.get () : textures.buttons.cancel.get ());
    drawRectangle (30, 400, (pressed_button == ButtonId::Quit) ? textures.buttons.quit_pressed.get () : textures.buttons.quit.get ());
}
void RoomWidget::drawConfirmingReadiness ()
{
    drawRectangle (0, 0, w, h, textures.grass.get ());

    drawRectangle (30, 30, (pressed_button == ButtonId::Ready) ? textures.buttons.ready_pressed.get () : textures.buttons.ready.get ());
    drawRectangle (30, 200, (pressed_button == ButtonId::Quit) ? textures.buttons.quit_pressed.get () : textures.buttons.quit.get ());
}
void RoomWidget::drawReady ()
{
    drawRectangle (0, 0, w, h, textures.grass.get ());

    drawRectangle (30, 30, textures.labels.ready.get ());

    drawRectangle (30, 200, (pressed_button == ButtonId::Quit) ? textures.buttons.quit_pressed.get () : textures.buttons.quit.get ());
}
void RoomWidget::drawAwaitingMatch ()
{
    drawRectangle (0, 0, w, h, textures.grass.get ());

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

    drawRectangle (30, 30, label_texture);

    drawRectangle (30, 200, (pressed_button == ButtonId::Quit) ? textures.buttons.quit_pressed.get () : textures.buttons.quit.get ());
}
void RoomWidget::drawMatchStarted ()
{
    if (last_frame.isValid ())
        frameUpdate (last_frame.nsecsElapsed ()*0.000000001);
    last_frame.restart ();

    const QRectF& area = match_state->areaRef ();

    {
        qreal scale = viewport_scale*MAP_TO_SCREEN_FACTOR;
        QPointF center = arena_viewport_center - viewport_center;
        const GLfloat vertices[] = {
            GLfloat (center.x () + scale*area.left ()), GLfloat (center.y () + scale*area.top ()),
            GLfloat (center.x () + scale*area.right ()), GLfloat (center.y () + scale*area.top ()),
            GLfloat (center.x () + scale*area.right ()), GLfloat (center.y () + scale*area.bottom ()),
            GLfloat (center.x () + scale*area.left ()), GLfloat (center.y () + scale*area.bottom ()),
        };

        const GLfloat texture_coords[] = {
            0, 0,
            1, 0,
            1, 1,
            0, 1,
        };

        const GLuint indices[] = {
            0, 1, 2,
            0, 2, 3,
        };

        drawTextured (GL_TRIANGLES, vertices, texture_coords, 6, indices, textures.ground.get ());
    }

    {
        QPointF top_left = toScreenCoords (area.topLeft ());
        QPointF bottom_right = toScreenCoords (area.bottomRight ());
        QPointF size = bottom_right - top_left;
        drawRectangle (
            top_left.x (), top_left.y (),
            size.x (), size.y (),
            QColor (0, 0, 255, 255)
        );
    }

    const QHash<quint32, Unit>& units = match_state->unitsRef ();
    const QHash<quint32, Missile>& missiles = match_state->missilesRef ();
    const QHash<quint32, Explosion>& explosions = match_state->explosionsRef ();

    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it)
        drawUnit (it.value ());
    for (QHash<quint32, Missile>::const_iterator it = missiles.constBegin (); it != missiles.constEnd (); ++it)
        drawMissile (it.value ());
    for (QHash<quint32, Explosion>::const_iterator it = explosions.constBegin (); it != explosions.constEnd (); ++it)
        drawExplosion (it.value ());
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
        drawRectangle (
            qMin (selection_start->x (), cursor_position.x ()), qMin (selection_start->y (), cursor_position.y ()),
            qAbs (selection_start->x () - cursor_position.x ()), qAbs (selection_start->y () - cursor_position.y ()),
            QColor (0, 255, 0, 255)
        );
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
    QSound::play (sound_files[random_generator ()%sound_files.size ()]);
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
    if (cursor_position.x () == -1 || w == 1)
        return;

    int dx = 0;
    int dy = 0;
    if (cursor_position.x () <= mouse_scroll_border)
        --dx;
    if (cursor_position.x () >= (w - mouse_scroll_border - 1))
        ++dx;
    if (cursor_position.y () <= mouse_scroll_border)
        --dy;
    if (cursor_position.y () >= (h - mouse_scroll_border - 1))
        ++dy;
    qreal off = 4000.0;
    if (dx && dy)
        off *= SQRT_1_2;
    viewport_center += QPointF (dx*off*dt, dy*off*dt);
    const QRectF& area = match_state->areaRef ();
    qreal scale = viewport_scale*MAP_TO_SCREEN_FACTOR;
    if (viewport_center.x () < area.left ()*scale)
        viewport_center.setX (area.left ()*scale);
    else if (viewport_center.x () > area.right ()*scale)
        viewport_center.setX (area.right ()*scale);
    if (viewport_center.y () < area.top ()*scale)
        viewport_center.setY (area.top ()*scale);
    else if (viewport_center.y () > area.bottom ()*scale)
        viewport_center.setY (area.bottom ()*scale);
}
QPointF RoomWidget::toMapCoords (const QPointF& point) const
{
    qreal scale = viewport_scale*MAP_TO_SCREEN_FACTOR;
    return (viewport_center - arena_viewport_center + point)/scale;
}
QRectF RoomWidget::toMapCoords (const QRectF& rect) const
{
    return QRectF (toMapCoords (rect.topLeft ()), toMapCoords (rect.bottomRight ()));
}
QPointF RoomWidget::toScreenCoords (const QPointF& point) const
{
    qreal scale = viewport_scale*MAP_TO_SCREEN_FACTOR;
    return arena_viewport_center - viewport_center + scale*point;
}
bool RoomWidget::pointInsideButton (const QPoint& point, const QPoint& button_pos, QSharedPointer<QOpenGLTexture>& texture) const
{
    return
        point.x () >= button_pos.x () &&
        point.x () < (button_pos.x () + texture->width ()) &&
        point.y () >= button_pos.y () &&
        point.y () < (button_pos.y () + texture->height ());
}
void RoomWidget::centerViewportAtSelected ()
{
    std::optional<QPointF> center = match_state->selectionCenter ();
    if (center.has_value ()) {
        qreal scale = viewport_scale*MAP_TO_SCREEN_FACTOR;
        viewport_center = *center*scale;
    }
}
void RoomWidget::drawHUD ()
{
    QColor margin_color (0, 0, 255);
    QColor panel_color (0x0c, 0x72, 0x73);
    QColor panel_inner_color (0, 0xaa, 0xaa);
    int margin = 12;
    QSize action_button_size (h*0.064, h*0.064);
    QSize minimap_panel_size (h*0.30, h*0.24);
    QSize action_panel_size (action_button_size.width ()*5, action_button_size.height ()*3);
    QSize unit_panel_size (h*0.12, h*0.18);
    QSize selection_panel_size (w - minimap_panel_size.width () - action_panel_size.width () - unit_panel_size.width () - margin*5, h*0.18);

    {
        int area_w = minimap_panel_size.width () + margin*2;
        int area_h = minimap_panel_size.height () + margin*2;
        fillRectangle (0, h - area_h, area_w, area_h, margin_color);
    }
    {
        int area_w = action_panel_size.width () + margin*2;
        int area_h = action_panel_size.height () + margin*2;
        fillRectangle (w - area_w, h - area_h, area_w, area_h, margin_color);
    }
    {
        fillRectangle (minimap_panel_size.width () + margin*2, h - selection_panel_size.height () - margin*2,
                       w - minimap_panel_size.width () - action_panel_size.width () - margin*4, selection_panel_size.height () + margin*2,
                       margin_color);
    }
    {
        int area_w = minimap_panel_size.width ();
        int area_h = minimap_panel_size.height ();
        fillRectangle (margin, h - area_h - margin, area_w, area_h, panel_color);
    }
    {
        int area_w = selection_panel_size.width ();
        int area_h = selection_panel_size.height ();
        fillRectangle (margin*2 + minimap_panel_size.width (), h - area_h - margin, area_w, area_h, panel_color);
    }
    {
        int area_w = unit_panel_size.width ();
        int area_h = unit_panel_size.height ();
        fillRectangle (w - area_w - action_panel_size.width () - margin * 2, h - area_h - margin, area_w, area_h, panel_color);
    }
    {
        int area_w = action_panel_size.width ();
        int area_h = action_panel_size.height ();
        fillRectangle (w - area_w - margin, h - area_h - margin, area_w, area_h, QColor (panel_color));
        fillRectangle (w - area_w - margin, h - area_h - margin, action_button_size.width (), action_button_size.height (), panel_inner_color);
        fillRectangle (w - area_w - margin + action_button_size.width ()*2, h - area_h - margin, action_button_size.width (), action_button_size.height (), panel_inner_color);
        fillRectangle (w - area_w - margin + action_button_size.width ()*4, h - area_h - margin, action_button_size.width (), action_button_size.height (), panel_inner_color);
        fillRectangle (w - area_w - margin + action_button_size.width (), h - area_h - margin + action_button_size.height (), action_button_size.width (), action_button_size.height (), panel_inner_color);
        fillRectangle (w - area_w - margin + action_button_size.width ()*3, h - area_h - margin + action_button_size.height (), action_button_size.width (), action_button_size.height (), panel_inner_color);
        fillRectangle (w - area_w - margin, h - area_h - margin + action_button_size.height ()*2, action_button_size.width (), action_button_size.height (), panel_inner_color);
        fillRectangle (w - area_w - margin + action_button_size.width ()*2, h - area_h - margin + action_button_size.height ()*2, action_button_size.width (), action_button_size.height (), panel_inner_color);
        fillRectangle (w - area_w - margin + action_button_size.width ()*4, h - area_h - margin + action_button_size.height ()*2, action_button_size.width (), action_button_size.height (), panel_inner_color);
    }

    size_t selected_count = 0;
    const Unit* last_selected_unit = nullptr;
    const QHash<quint32, Unit>& units = match_state->unitsRef ();
    for (QHash<quint32, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it) {
        if (it->selected) {
            ++selected_count;
            last_selected_unit = &*it;
        }
    }
    if (selected_count == 1) {
        const Unit& unit = *last_selected_unit;
        const AttackDescription& primary_attack_description = match_state->unitPrimaryAttackDescription (unit.type);
        int area_w = selection_panel_size.width ();
        int area_h = selection_panel_size.height ();
        QRectF rect (margin*2 + minimap_panel_size.width (), h - area_h - margin, area_w, area_h);

        QPainter p (this);
        p.setFont (font);
        p.setPen (QColor (0xff, 0xff, 0xff));
        p.drawText (rect, Qt::AlignHCenter,
                    unitTitle (unit.type) + "\n"
                    "\n"
                    "HP: " + QString::number (unit.hp) + "/" + QString::number (match_state->unitMaxHP (unit.type)) + "\n"
                    "Attack: class = " + unitAttackClassName (primary_attack_description.type) + "; range = " + QString::number (primary_attack_description.range));
    } else if (selected_count) {
        int area_w = selection_panel_size.width ();
        int area_h = selection_panel_size.height ();
        QRectF rect (margin*2 + minimap_panel_size.width (), h - area_h - margin, area_w, area_h);

        QPainter p (this);
        p.setFont (font);
        p.setPen (QColor (0xff, 0xff, 0xff));
        p.drawText (rect, Qt::AlignHCenter, "[Multiple units]");
    }
}
void RoomWidget::drawUnit (const Unit& unit)
{
    qreal sprite_scale;
    UnitTextureSet* texture_set;
    switch (unit.type) {
    case Unit::Type::Seal: {
        sprite_scale = 1.6;
        texture_set = &textures.units.seal;
    } break;
    case Unit::Type::Crusader: {
        sprite_scale = 2.0;
        texture_set = &textures.units.crusader;
    } break;
    case Unit::Type::Goon: {
        sprite_scale = 1.6;
        texture_set = &textures.units.goon;
    } break;
    case Unit::Type::Beetle: {
        sprite_scale = 1.6;
        texture_set = &textures.units.beetle;
    } break;
    case Unit::Type::Contaminator: {
        sprite_scale = 1.6;
        texture_set = &textures.units.contaminator;
    } break;
    default: {
    } return;
    }

    UnitTextureTeam* texture_team;
    switch (unit.team) {
    case Unit::Team::Red:
        texture_team = &texture_set->red;
        break;
    case Unit::Team::Blue:
        texture_team = &texture_set->blue;
        break;
    default:
        return;
    }

    QOpenGLTexture* texture;
    if (unit.attack_remaining_ticks) {
        quint64 clock_ns = match_state->clockNS ();
        quint64 period = attackAnimationPeriodNS (unit.type);
        quint64 phase = (clock_ns + unit.phase_offset)%period;
        texture = (phase < period/2) ? texture_team->shooting1.get () : texture_team->shooting2.get ();
    } else if (std::holds_alternative<MoveAction> (unit.action) || std::holds_alternative<AttackAction> (unit.action)) {
        quint64 clock_ns = match_state->clockNS ();
        quint64 period = moveAnimationPeriodNS (unit.type);
        quint64 phase = (clock_ns + unit.phase_offset)%period;
        texture = (phase < period/2) ? texture_team->walking1.get () : texture_team->walking2.get ();
    } else {
        texture = texture_team->standing.get ();
    }

    QPointF center = toScreenCoords (unit.position);

    qreal a1_sin, a1_cos;
    qreal a2_sin, a2_cos;
    qreal a3_sin, a3_cos;
    qreal a4_sin, a4_cos;
    sincos (unit.orientation + PI_X_3_4, &a1_sin, &a1_cos);
    sincos (unit.orientation + PI_X_1_4, &a2_sin, &a2_cos);
    sincos (unit.orientation - PI_X_1_4, &a3_sin, &a3_cos);
    sincos (unit.orientation - PI_X_3_4, &a4_sin, &a4_cos);
    qreal scale = viewport_scale*sprite_scale*match_state->unitDiameter (unit.type)*SQRT_2*MAP_TO_SCREEN_FACTOR;

    const GLfloat vertices[] = {
        GLfloat (center.x () + scale*a1_cos), GLfloat (center.y () + scale*a1_sin),
        GLfloat (center.x () + scale*a2_cos), GLfloat (center.y () + scale*a2_sin),
        GLfloat (center.x () + scale*a3_cos), GLfloat (center.y () + scale*a3_sin),
        GLfloat (center.x () + scale*a4_cos), GLfloat (center.y () + scale*a4_sin),
    };

    const GLfloat texture_coords[] = {
        0, 1,
        1, 1,
        1, 0,
        0, 0,
    };

    const GLuint indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    drawTextured (GL_TRIANGLES, vertices, texture_coords, 6, indices, texture);
    if (unit.selected)
        drawCircle (center.x (), center.y (), scale*0.25, {0, 255, 0});
}
void RoomWidget::drawUnitHPBar (const Unit& unit)
{
    if (unit.hp < match_state->unitMaxHP (unit.type)) {
        QPointF center = toScreenCoords (unit.position);
        qreal hp_ratio = qreal (unit.hp)/match_state->unitMaxHP (unit.type);
        qreal hitbar_height = viewport_scale*MAP_TO_SCREEN_FACTOR*0.16;
        // TODO: Split into bars? int hit_bar_count = match_state->unitHitBarCount (unit.type);
        qreal radius = viewport_scale*match_state->unitDiameter (unit.type)*MAP_TO_SCREEN_FACTOR*0.42;

        {
            const GLfloat vertices[] = {
                GLfloat (center.x () - radius), GLfloat (center.y () - radius),
                GLfloat (center.x () + radius*(-1.0 + hp_ratio*2.0)), GLfloat (center.y () - radius),
                GLfloat (center.x () + radius*(-1.0 + hp_ratio*2.0)), GLfloat (center.y () - radius - hitbar_height),
                GLfloat (center.x () - radius), GLfloat (center.y () - radius - hitbar_height),
            };

            const GLfloat colors[] = {
                0, 0, 1, 1,
                0, 0, 1, 1,
                0, 0, 1, 1,
                0, 0, 1, 1,
            };

            drawColored (GL_TRIANGLE_FAN, 4, vertices, colors);
        }

        {
            const GLfloat vertices[] = {
                GLfloat (center.x () - radius), GLfloat (center.y () - radius),
                GLfloat (center.x () + radius), GLfloat (center.y () - radius),
                GLfloat (center.x () + radius), GLfloat (center.y () - radius - hitbar_height),
                GLfloat (center.x () - radius), GLfloat (center.y () - radius - hitbar_height),
            };

            const GLfloat colors[] = {
                0, 1, 0, 1,
                0, 1, 0, 1,
                0, 1, 0, 1,
                0, 1, 0, 1,
            };

            drawColored (GL_LINE_LOOP, 4, vertices, colors);
        }
    }
}
void RoomWidget::drawMissile (const Missile& missile)
{
    quint64 clock_ns = match_state->clockNS ();
    qreal sprite_scale = 1.0;

    quint64 period = missileAnimationPeriodNS (Missile::Type::Rocket);
    quint64 phase = clock_ns%period;
    QOpenGLTexture* texture;
    switch (missile.type) {
    case Missile::Type::Rocket:
        texture = (phase < period/2) ? textures.effects.goon_rocket.rocket1.get () : textures.effects.goon_rocket.rocket2.get ();
        break;
    case Missile::Type::Pestilence:
        texture = (phase < period/2) ? textures.effects.pestilence_missile.missile1.get () : textures.effects.pestilence_missile.missile2.get ();
        break;
    default:
        return;
    }

    QPointF center = toScreenCoords (missile.position);

    qreal a1_sin, a1_cos;
    qreal a2_sin, a2_cos;
    qreal a3_sin, a3_cos;
    qreal a4_sin, a4_cos;
    sincos (missile.orientation + PI_X_3_4, &a1_sin, &a1_cos);
    sincos (missile.orientation + PI_X_1_4, &a2_sin, &a2_cos);
    sincos (missile.orientation - PI_X_1_4, &a3_sin, &a3_cos);
    sincos (missile.orientation - PI_X_3_4, &a4_sin, &a4_cos);
    qreal scale = viewport_scale*sprite_scale*match_state->missileDiameter (Missile::Type::Rocket)*SQRT_2*MAP_TO_SCREEN_FACTOR;

    const GLfloat vertices[] = {
        GLfloat (center.x () + scale*a1_cos), GLfloat (center.y () + scale*a1_sin),
        GLfloat (center.x () + scale*a2_cos), GLfloat (center.y () + scale*a2_sin),
        GLfloat (center.x () + scale*a3_cos), GLfloat (center.y () + scale*a3_sin),
        GLfloat (center.x () + scale*a4_cos), GLfloat (center.y () + scale*a4_sin),
    };

    const GLfloat texture_coords[] = {
        0, 1,
        1, 1,
        1, 0,
        0, 0,
    };

    const GLuint indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    drawTextured (GL_TRIANGLES, vertices, texture_coords, 6, indices, texture);
}
void RoomWidget::drawExplosion (const Explosion& explosion)
{
    const AttackDescription& attack_description = match_state->effectAttackDescription (AttackDescription::Type::GoonRocketExplosion);
    qreal sprite_scale;
    switch (explosion.type) {
    case Explosion::Type::Fire:
        sprite_scale = 1.7;
        break;
    case Explosion::Type::Pestilence:
        sprite_scale = 1.2;
        break;
    default:
        return;
    }

    qreal orientation = 0.0;
    quint64 clock_ns = match_state->clockNS ();
    GLfloat alpha = explosion.remaining_ticks*0.5/attack_description.duration_ticks;

    quint64 period = explosionAnimationPeriodNS ();
    quint64 phase = clock_ns%period;
    QOpenGLTexture* texture;
    switch (explosion.type) {
    case Explosion::Type::Fire:
        texture = (phase < period/2) ? textures.effects.explosion.explosion1.get () : textures.effects.explosion.explosion2.get ();
        break;
    case Explosion::Type::Pestilence:
        texture = textures.effects.pestilence_splash.splash.get ();
        break;
    default:
        return;
    }

    QPointF center = toScreenCoords (explosion.position);

    qreal a1_sin, a1_cos;
    qreal a2_sin, a2_cos;
    qreal a3_sin, a3_cos;
    qreal a4_sin, a4_cos;
    sincos (orientation + PI_X_3_4, &a1_sin, &a1_cos);
    sincos (orientation + PI_X_1_4, &a2_sin, &a2_cos);
    sincos (orientation - PI_X_1_4, &a3_sin, &a3_cos);
    sincos (orientation - PI_X_3_4, &a4_sin, &a4_cos);
    qreal scale = viewport_scale*sprite_scale*match_state->explosionDiameter (explosion.type)*SQRT_2*MAP_TO_SCREEN_FACTOR;

    const GLfloat colors[] = {
        1, 1, 1, alpha,
        1, 1, 1, alpha,
        1, 1, 1, alpha,
        1, 1, 1, alpha,
    };

    const GLfloat vertices[] = {
        GLfloat (center.x () + scale*a1_cos), GLfloat (center.y () + scale*a1_sin),
        GLfloat (center.x () + scale*a2_cos), GLfloat (center.y () + scale*a2_sin),
        GLfloat (center.x () + scale*a3_cos), GLfloat (center.y () + scale*a3_sin),
        GLfloat (center.x () + scale*a4_cos), GLfloat (center.y () + scale*a4_sin),
    };

    static const GLfloat texture_coords[] = {
        0, 1,
        1, 1,
        1, 0,
        0, 0,
    };

    static const GLuint indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    drawColoredTextured (GL_TRIANGLES, vertices, colors, texture_coords, 6, indices, texture);
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

    QPointF current = toScreenCoords (unit.position);
    QPointF target = toScreenCoords (*target_position);

    const GLfloat vertices[] = {
        GLfloat (current.x ()), GLfloat (current.y ()),
        GLfloat (target.x ()), GLfloat (target.y ()),
    };

    static const GLfloat attack_colors[] = {
        1, 0, 0, 1,
        1, 0, 0, 1,
    };

    static const GLfloat move_colors[] = {
        0, 1, 0, 1,
        0, 1, 0, 1,
    };

    drawColored (GL_LINES, 2, vertices, std::holds_alternative<AttackAction> (unit.action) ? attack_colors : move_colors);
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
