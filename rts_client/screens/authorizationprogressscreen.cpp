#include "authorizationprogressscreen.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>


AuthorizationProgressScreen::AuthorizationProgressScreen (const QString& message, QWidget* parent)
    : QWidget (parent)
{
    QGridLayout* layout = new QGridLayout;
    layout->setRowStretch (0, 1);
    layout->setRowStretch (3, 1);
    layout->setColumnStretch (0, 1);
    layout->setColumnStretch (2, 1);
    {
        QLabel* label = new QLabel (message, this);
        layout->addWidget (label, 1, 1);
    }
    {
        QHBoxLayout* hlayout = new QHBoxLayout;
        hlayout->addStretch (1);
        {
            QPushButton* button = new QPushButton ("&Cancel", this);
            connect (button, SIGNAL (clicked ()), this, SIGNAL (cancelRequested ()));
            hlayout->addWidget (button);
        }
        hlayout->addStretch (1);
        layout->addLayout (hlayout, 2, 1);
    }
    setLayout (layout);
}
AuthorizationProgressScreen::~AuthorizationProgressScreen ()
{
}
