#include "authorizationprogresswidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>


AuthorizationProgressWidget::AuthorizationProgressWidget (const QString& message, QWidget* parent)
    : QWidget (parent)
{
    QVBoxLayout* layout = new QVBoxLayout;
    {
        QLabel* label = new QLabel (message, this);
        layout->addWidget (label);
    }
    {
        QHBoxLayout* hlayout = new QHBoxLayout;
        hlayout->addStretch (1);
        {
            QPushButton* button = new QPushButton ("Cancel", this);
            connect (button, SIGNAL (clicked ()), this, SIGNAL (cancelRequested ()));
            hlayout->addWidget (button);
        }
        hlayout->addStretch (1);
        layout->addLayout (hlayout);
    }
    setLayout (layout);
}
AuthorizationProgressWidget::~AuthorizationProgressWidget ()
{
}
