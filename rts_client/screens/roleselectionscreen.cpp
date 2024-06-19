#include "roleselectionscreen.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>


RoleSelectionScreen::RoleSelectionScreen (QWidget* parent)
    : QWidget (parent)
{
    QGridLayout* decoration_grid = new QGridLayout;
    decoration_grid->setRowStretch (0, 1);
    decoration_grid->setRowStretch (2, 1);
    decoration_grid->setColumnStretch (0, 1);
    decoration_grid->setColumnStretch (2, 1);
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
        decoration_grid->addLayout (layout, 1, 1);
    }
    setLayout (decoration_grid);
}
RoleSelectionScreen::~RoleSelectionScreen ()
{
}
