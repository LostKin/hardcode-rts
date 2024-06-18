#include "profilepopup.h"

#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>


ProfilePopup::ProfilePopup (const QString& login, QWidget* parent)
    : QDialog (parent, Qt::Popup)
{
    setWindowModality (Qt::WindowModal);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins (0, 0, 0, 0);
    {
        QFrame* frame = new QFrame (this);
        frame->setFrameStyle (QFrame::Panel | QFrame::Raised);
        {
            QVBoxLayout* sublayout = new QVBoxLayout;
            {
                QLabel* label = new QLabel ("You are logged in as '<b>" + QString (login).toHtmlEscaped () + "</b>'", this);
                sublayout->addWidget (label);
            }
            {
                QHBoxLayout* hbox = new QHBoxLayout;
                hbox->addStretch (1);
                {
                    QPushButton* button = new QPushButton ("Logout", this);
                    connect (button, SIGNAL (clicked ()), this, SIGNAL (logoutRequested ()));
                    connect (button, SIGNAL (clicked ()), this, SLOT (close ()));
                    hbox->addWidget (button);
                }
                sublayout->addLayout (hbox);
            }
            frame->setLayout (sublayout);
        }
        layout->addWidget (frame);
    }
    setLayout (layout);
}
ProfilePopup::~ProfilePopup ()
{
}
