#include "roomwidget.h"

#include "matchstate.h"

#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <math.h>


static constexpr qreal SQRT_2 = 1.4142135623731;
static constexpr qreal SQRT_1_2 = 0.70710678118655;
static constexpr qreal PI_X_3_4 = 3.0/4.0*M_PI;
static constexpr qreal PI_X_1_4 = 1.0/4.0*M_PI;


RoomWidget::RoomWidget (QWidget* parent)
    : OpenGLWidget (parent)
{
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
    awaitTeamSelection (RoomWidget::Team::Red);
}
void RoomWidget::joinBlueTeamRequestedHandler () {
    awaitTeamSelection (RoomWidget::Team::Blue);
}
void RoomWidget::spectateRequestedHandler () {
    awaitTeamSelection (RoomWidget::Team::Spectator);
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
}
void RoomWidget::awaitTeamSelection (Team team)
{
    this->team = team;
    pressed_button = ButtonId::None;
    match_timer.stop ();
    state = State::TeamSelectionRequested;
}
void RoomWidget::queryReadiness (Team team)
{
    this->team = team;
    pressed_button = ButtonId::None;
    match_timer.stop ();
    state = State::ConfirmingReadiness;
}
void RoomWidget::ready (Team team)
{
    this->team = team;
    pressed_button = ButtonId::None;
    match_timer.stop ();
    state = State::Ready;
}
void RoomWidget::awaitMatch (Team team)
{
    this->team = team;
    pressed_button = ButtonId::None;
    match_countdown_start.start ();
    match_timer.stop ();
    state = State::AwaitingMatch;
}
void RoomWidget::startMatch (Team team)
{
    this->team = team;
    pressed_button = ButtonId::None;
    match_state = QSharedPointer<MatchState> (new MatchState);
    {
        match_state->addUnit (Unit (Unit::Type::Seal, Unit::Team::Red, QPointF (10, 30), 0));
        match_state->addUnit (Unit (Unit::Type::Seal, Unit::Team::Red, QPointF (80, 30), 0));
        match_state->addUnit (Unit (Unit::Type::Seal, Unit::Team::Blue, QPointF (100, 50), 0));
        match_state->addUnit (Unit (Unit::Type::Crusader, Unit::Team::Blue, QPointF (-200, -80), 0));
    }
    viewport_scale = 1.0;
    viewport_center = {};
    match_timer.start (20);
    last_frame.restart ();
    state = State::MatchStarted;
}
void RoomWidget::loadTextures ()
{
    textures.grass = loadTexture2DRectangle (":/images/grass.png");
    textures.ground = loadTexture2D (":/images/ground.png");
    textures.hud = loadTexture2DRectangle (":/images/hud.png");

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

    textures.units.seal.red.plain = loadTexture2D (":/images/units/seal/red-plain.png");
    textures.units.seal.red.selected = loadTexture2D (":/images/units/seal/red-selected.png");
    textures.units.seal.blue.plain = loadTexture2D (":/images/units/seal/blue-plain.png");
    textures.units.seal.blue.selected = loadTexture2D (":/images/units/seal/blue-selected.png");

    textures.units.crusader.red.plain = loadTexture2D (":/images/units/crusader/red-plain.png");
    textures.units.crusader.red.selected = loadTexture2D (":/images/units/crusader/red-selected.png");
    textures.units.crusader.blue.plain = loadTexture2D (":/images/units/crusader/blue-plain.png");
    textures.units.crusader.blue.selected = loadTexture2D (":/images/units/crusader/blue-selected.png");
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
    arena_center = arena_viewport.center ();
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
    switch (state) {
    case State::MatchStarted: {
        switch (event->key ()) {
        case Qt::Key_G:
            match_state->move (toMapCoords (cursor_position));
            break;
        case Qt::Key_S:
            match_state->stop ();
            break;
        }
    } break;
    default: {
    }
    }
}
void RoomWidget::keyReleaseEvent (QKeyEvent *event)
{
    (void) event;

    switch (state) {
    case State::MatchStarted: {
    } break;
    default: {
    }
    }
}
void RoomWidget::mouseMoveEvent (QMouseEvent *event)
{
    cursor_position = event->pos ();
}
void RoomWidget::mousePressEvent (QMouseEvent *event)
{
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
        switch (event->button ()) {
        case Qt::LeftButton: {
            match_state->trySelect (toMapCoords (cursor_position));
        } break;
        case Qt::RightButton: {
            match_state->autoAction (toMapCoords (cursor_position));
        } break;
        default: {
        }
        }
    } break;
    default: {
    }
    }
}
void RoomWidget::mouseReleaseEvent (QMouseEvent *event)
{
    switch (state) {
    case State::TeamSelection: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 30), textures.buttons.join_blue_team)) {
                if (pressed_button == ButtonId::JoinBlueTeam) {
                    emit joinBlueTeamRequested ();
                }
            } else if (pointInsideButton (cursor_position, QPoint (30, 130), textures.buttons.join_red_team)) {
                if (pressed_button == ButtonId::JoinRedTeam) {
                    emit joinRedTeamRequested ();
                }
            } else if (pointInsideButton (cursor_position, QPoint (30, 230), textures.buttons.spectate)) {
                if (pressed_button == ButtonId::Spectate) {
                    emit spectateRequested ();
                }
            } else if (pointInsideButton (cursor_position, QPoint (30, 400), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit) {
                    emit quitRequested ();
                }
            }
        }
    } break;
    case State::TeamSelectionRequested: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 230), textures.buttons.cancel)) {
                if (pressed_button == ButtonId::Cancel) {
                    emit cancelJoinTeamRequested ();
                }
            } else if (pointInsideButton (cursor_position, QPoint (30, 400), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit) {
                    emit quitRequested ();
                }
            }
        }
    } break;
    case State::ConfirmingReadiness: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 30), textures.buttons.ready)) {
                if (pressed_button == ButtonId::Ready) {
                    emit readinessRequested ();
                }
            } else if (pointInsideButton (cursor_position, QPoint (30, 200), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit) {
                    emit quitRequested ();
                }
            }
        }
    } break;
    case State::Ready: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 200), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit) {
                    emit quitRequested ();
                }
            }
        }
    } break;
    case State::AwaitingMatch: {
        if (event->button () == Qt::LeftButton) {
            if (pointInsideButton (cursor_position, QPoint (30, 200), textures.buttons.quit)) {
                if (pressed_button == ButtonId::Quit) {
                    emit quitRequested ();
                }
            }
        }
    } break;
    default: {
    }
    }
    pressed_button = ButtonId::None;
}
void RoomWidget::wheelEvent (QWheelEvent *event)
{
    int dy = event->angleDelta ().y ()/120;
    switch (state) {
    case State::MatchStarted: {
        viewport_scale_power += dy;
        viewport_scale_power = qBound (-10, viewport_scale_power, 10);
        viewport_scale = pow (1.125, viewport_scale_power);
    } break;
    default: {
    }
    }
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
        QPointF center = arena_center - viewport_center;
        const GLfloat vertices[] = {
            GLfloat (center.x () + viewport_scale*area.left ()), GLfloat (center.y () + viewport_scale*area.top ()),
            GLfloat (center.x () + viewport_scale*area.right ()), GLfloat (center.y () + viewport_scale*area.top ()),
            GLfloat (center.x () + viewport_scale*area.right ()), GLfloat (center.y () + viewport_scale*area.bottom ()),
            GLfloat (center.x () + viewport_scale*area.left ()), GLfloat (center.y () + viewport_scale*area.bottom ()),
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

    const QHash<uint32_t, Unit>& units = match_state->unitsRef ();
    for (QHash<uint32_t, Unit>::const_iterator it = units.constBegin (); it != units.constEnd (); ++it)
        drawUnit (it.value ());

    drawRectangle (0, 0, textures.hud.get ());
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
}
QPointF RoomWidget::toMapCoords(const QPointF& point)
{
    return (viewport_center - arena_center + point)/viewport_scale;
}
QPointF RoomWidget::toScreenCoords (const QPointF& point)
{
    return arena_center - viewport_center + viewport_scale*point;
}
bool RoomWidget::pointInsideButton (const QPoint& point, const QPoint& button_pos, QSharedPointer<QOpenGLTexture>& texture)
{
    return
        point.x () >= button_pos.x () &&
        point.x () < (button_pos.x () + texture->width ()) &&
        point.y () >= button_pos.y () &&
        point.y () < (button_pos.y () + texture->height ());
}
void RoomWidget::drawUnit (const Unit& unit)
{
    qreal sprite_rib;
    UnitTextureSet* texture_set;
    switch (unit.type) {
    case Unit::Type::Seal: {
        sprite_rib = 50.0;
        texture_set = &textures.units.seal;
    } break;
    case Unit::Type::Crusader: {
        sprite_rib = 50.0;
        texture_set = &textures.units.crusader;
    } break;
    default: {
    } return;
    }

    QOpenGLTexture* texture;
    if (unit.team == Unit::Team::Red)
        texture = unit.selected ? texture_set->red.selected.get () : texture_set->red.plain.get ();
    else if (unit.team == Unit::Team::Blue)
        texture = unit.selected ? texture_set->blue.selected.get () : texture_set->blue.plain.get ();
    else
        return;

    QPointF center = toScreenCoords (unit.position);
    QRectF rect (-sprite_rib*0.5, -sprite_rib*0.5, sprite_rib, sprite_rib);

    qreal a1_sin, a1_cos;
    qreal a2_sin, a2_cos;
    qreal a3_sin, a3_cos;
    qreal a4_sin, a4_cos;
    sincos (unit.orientation + PI_X_3_4, &a1_sin, &a1_cos);
    sincos (unit.orientation + PI_X_1_4, &a2_sin, &a2_cos);
    sincos (unit.orientation - PI_X_1_4, &a3_sin, &a3_cos);
    sincos (unit.orientation - PI_X_3_4, &a4_sin, &a4_cos);
    qreal scale = viewport_scale*sprite_rib*SQRT_2;

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
