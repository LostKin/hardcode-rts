#include "roleselectionprogressscreen.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>


RoleSelectionProgressScreen::RoleSelectionProgressScreen (QWidget* parent)
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
            QLabel* label = new QLabel ("Join team requested.\nAwaiting response...", this);
            layout->addWidget (label);
        }
        layout->addSpacing (10);
        {
            QPushButton* button = new QPushButton ("&Cancel", this);
            connect (button, &QPushButton::clicked, this, &RoleSelectionProgressScreen::quitRequested);
            layout->addWidget (button);
        }
        layout->addSpacing (10);
        {
            QPushButton* button = new QPushButton ("&Quit", this);
            connect (button, &QPushButton::clicked, this, &RoleSelectionProgressScreen::cancelRequested);
            layout->addWidget (button);
        }
        decoration_grid->addLayout (layout, 1, 1);
    }
    setLayout (decoration_grid);
}
RoleSelectionProgressScreen::~RoleSelectionProgressScreen ()
{
}
