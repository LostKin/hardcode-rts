#include "roleselectionprogressscreen.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>


RoleSelectionProgressScreen::RoleSelectionProgressScreen (QWidget* parent)
    : QWidget (parent)
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
    setLayout (layout);
}
RoleSelectionProgressScreen::~RoleSelectionProgressScreen ()
{
}
