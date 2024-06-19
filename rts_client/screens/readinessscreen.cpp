#include "readinessscreen.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>


ReadinessScreen::ReadinessScreen (QWidget* parent)
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
            ready_button = new QPushButton ("&Ready", this);
            connect (ready_button, &QPushButton::clicked, this, &ReadinessScreen::readynessClickCallback);
            layout->addWidget (ready_button);
        }
        {
            ready_label = new QLabel ("Ready", this);
            ready_label->hide ();
            layout->addWidget (ready_label);
        }
        layout->addSpacing (10);
        {
            QPushButton* button = new QPushButton ("&Quit", this);
            connect (button, &QPushButton::clicked, this, &ReadinessScreen::quitRequested);
            layout->addWidget (button);
        }
        decoration_grid->addLayout (layout, 1, 1);
    }
    setLayout (decoration_grid);
}
ReadinessScreen::~ReadinessScreen ()
{
}

void ReadinessScreen::readynessClickCallback ()
{
    ready_button->hide ();
    ready_label->show ();
    emit readinessRequested ();
}
