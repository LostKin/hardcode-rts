#include "authorizationwidget.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QDebug>

AuthorizationWidget::AuthorizationWidget (QWidget* parent)
    : QWidget (parent)
{
    QGridLayout* grid = new QGridLayout;
    {
        QLabel* label = new QLabel ("Endpoint (host:port):", this);
        grid->addWidget (label, 1, 1);
    }
    {
        QHBoxLayout* endpoint_layout = new QHBoxLayout;
        {
            // TODO: Implement DNS resolving
            host_line_edit = new QLineEdit ("127.0.0.1", this);
            host_line_edit->setPlaceholderText ("Host");
            connect (host_line_edit, SIGNAL (returnPressed ()), this, SLOT (loginClicked ()));
            endpoint_layout->addWidget (host_line_edit, 1);
        }
        {
            QLabel* label = new QLabel (":", this);
            endpoint_layout->addWidget (label);
        }
        {
            port_spin_box = new QSpinBox (this);
            port_spin_box->setRange (1, 65535);
            port_spin_box->setValue (1331);
            endpoint_layout->addWidget (port_spin_box);
        }
        grid->addLayout (endpoint_layout, 1, 2);
    }
    {
        QLabel* label = new QLabel ("Login:", this);
        grid->addWidget (label, 2, 1);
    }
    {
        login_line_edit = new QLineEdit ("john", this);
        connect (login_line_edit, SIGNAL (returnPressed ()), this, SLOT (loginClicked ()));
        grid->addWidget (login_line_edit, 2, 2);
    }
    {
        QLabel* label = new QLabel ("Password:", this);
        grid->addWidget (label, 3, 1);
    }
    {
        password_line_edit = new QLineEdit ("secret", this);
        connect (password_line_edit, SIGNAL (returnPressed ()), this, SLOT (loginClicked ()));
        password_line_edit->setEchoMode (QLineEdit::Password);
        grid->addWidget (password_line_edit, 3, 2);
    }
    {
        QHBoxLayout* hlayout = new QHBoxLayout;
        hlayout->addStretch (1);
        {
            QPushButton* button = new QPushButton ("Login", this);
            connect (button, SIGNAL (clicked ()), this, SLOT (loginClicked ()));
            hlayout->addWidget (button);
        }
        grid->addLayout (hlayout, 4, 1, 1, 2);
    }
    grid->setRowStretch (0, 1);
    grid->setRowStretch (5, 1);
    grid->setColumnStretch (0, 1);
    grid->setColumnStretch (2, 2);
    grid->setColumnStretch (3, 1);
    setLayout (grid);
}
AuthorizationWidget::~AuthorizationWidget ()
{
}

void AuthorizationWidget::closeEvent (QCloseEvent*)
{
    emit windowsCloseRequested ();
}
void AuthorizationWidget::loginClicked ()
{
    QString host = host_line_edit->text ();
    quint16 port = port_spin_box->value ();
    QString login = login_line_edit->text ();
    QString password = password_line_edit->text ();

    emit loginRequested (host, port, login, password);
}
