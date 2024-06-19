#include "roleselectionscreen.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>


RoleSelectionScreen::RoleSelectionScreen (QWidget* parent)
    : QWidget (parent)
{
    QVBoxLayout* layout = new QVBoxLayout;
    {
        QPushButton* button = new QPushButton ("Join as &player", this);
        connect (button, &QPushButton::clicked, this, &RoleSelectionScreen::selectRolePlayerRequested);
        layout->addWidget (button);
    }
    {
        QPushButton* button = new QPushButton ("&Spectate", this);
        connect (button, &QPushButton::clicked, this, &RoleSelectionScreen::spectateRequested);
        layout->addWidget (button);
    }
    layout->addSpacing (10);
    {
        QPushButton* button = new QPushButton ("&Quit", this);
        connect (button, &QPushButton::clicked, this, &RoleSelectionScreen::quitRequested);
        layout->addWidget (button);
    }
    setLayout (layout);
}
RoleSelectionScreen::~RoleSelectionScreen ()
{
}
