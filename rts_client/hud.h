#pragma once

#include "rectangle.h"

#include <QSize>
#include <QRect>
#include <QPoint>


enum class ActionButtonId {
    None,
    Move,
    Stop,
    Hold,
    Attack,
    Pestilence,
    Spawn,
};

class HUD
{
public:
    HUD ();

public:
    int margin = 1;
    int stroke_width = 1;
    QSize action_button_size = {1, 1};
    QRect minimap_panel_rect = {0, 0, 1, 1};
    QRect action_panel_rect = {0, 0, 1, 1};
    QRect selection_panel_rect = {0, 0, 1, 1};
    int selection_panel_icon_rib = 1;
    QPoint selection_panel_icon_grid_pos = {0, 0};
    Rectangle minimap_screen_area = {0, 0, 1, 1};

    ActionButtonId pressed_action_button = ActionButtonId::None;
    ActionButtonId current_action_button = ActionButtonId::None;
};
