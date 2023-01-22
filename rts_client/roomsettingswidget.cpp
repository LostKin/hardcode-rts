#include "roomsettingswidget.h"

#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>


RoomSettingsWidget::RoomSettingsWidget (QWidget* parent)
    : QDialog (parent, Qt::Popup)
{
    setWindowModality (Qt::WindowModal);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setMargin (0);
    {
        QFrame* frame = new QFrame (this);
        frame->setFrameStyle (QFrame::Panel | QFrame::Raised);
        {
            QVBoxLayout* sublayout = new QVBoxLayout;
            {
                QLabel* label = new QLabel ("Enter room name:", this);
                sublayout->addWidget (label);
            }
            {
                room_name_line_edit = new QLineEdit (this);
                sublayout->addWidget (room_name_line_edit);
            }
            {
                QHBoxLayout* hbox = new QHBoxLayout;
                hbox->addStretch (1);
                {
                    QPushButton* button = new QPushButton ("Create", this);
                    connect (button, SIGNAL (clicked ()), this, SLOT (createClicked ()));
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
RoomSettingsWidget::~RoomSettingsWidget ()
{
}

void RoomSettingsWidget::createClicked ()
{
    emit createRequested (room_name_line_edit->text ());
    close ();
}
