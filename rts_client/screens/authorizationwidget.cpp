#include "authorizationwidget.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QDebug>
#include <QHeaderView>

enum class CredsRole {
    Host = Qt::UserRole,
    Port = Qt::UserRole + 1,
    Login = Qt::UserRole + 2,
    Password = Qt::UserRole + 3,
};

AuthorizationWidget::AuthorizationWidget (const QVector<AuthorizationCredentials>& initial_credentials, QWidget* parent)
    : QWidget (parent)
{
    QGridLayout* grid = new QGridLayout;
    {
        QHBoxLayout* layout = new QHBoxLayout;
        {
            table = new QTableWidget (0, 1, this);
            table->setSelectionMode (QAbstractItemView::SingleSelection);
            table->horizontalHeader ()->setStretchLastSection (true);
            table->setHorizontalHeaderLabels ({"Endpoint"});
            int new_idx = 0;
            for (const AuthorizationCredentials& entry: initial_credentials) {
                table->insertRow (new_idx);

                QTableWidgetItem* item = new QTableWidgetItem (entry.login + "@" + entry.host + ":" + QString::number (entry.port));
                item->setFlags (item->flags () & ~Qt::ItemIsEditable);
                item->setData (int (CredsRole::Host), entry.host);
                item->setData (int (CredsRole::Port), entry.port);
                item->setData (int (CredsRole::Login), entry.login);
                item->setData (int (CredsRole::Password), entry.password);
                table->setItem (new_idx, 0, item);

                ++new_idx;
            }
            connect (table, SIGNAL (currentItemChanged (QTableWidgetItem*, QTableWidgetItem*)), this, SLOT (authorizationCredentialsItemSelected (QTableWidgetItem*)));
            connect (table, &QTableWidget::cellDoubleClicked, this, &AuthorizationWidget::handleCellDoubleClick);
            layout->addWidget (table, 1);
        }
        {
            QPushButton* button = new QPushButton (QPixmap (":/images/icons/delete.png"), "", this);
            connect (button, &QPushButton::clicked, this, &AuthorizationWidget::removeCredentialsClicked);
            layout->addWidget (button);
        }
        grid->addLayout (layout, 0, 0, 1, 2);
    }
    {
        QLabel* label = new QLabel ("Endpoint (host:port):", this);
        grid->addWidget (label, 1, 0);
    }
    {
        QHBoxLayout* endpoint_layout = new QHBoxLayout;
        {
            host_line_edit = new QLineEdit ("127.0.0.1", this);
            host_line_edit->setPlaceholderText ("Host");
            connect (host_line_edit, &QLineEdit::returnPressed, this, &AuthorizationWidget::loginClicked);
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
        grid->addLayout (endpoint_layout, 1, 1);
    }
    {
        QLabel* label = new QLabel ("Login:", this);
        grid->addWidget (label, 2, 0);
    }
    {
        login_line_edit = new QLineEdit ("john", this);
        connect (login_line_edit, &QLineEdit::returnPressed, this, &AuthorizationWidget::loginClicked);
        grid->addWidget (login_line_edit, 2, 1);
    }
    {
        QLabel* label = new QLabel ("Password:", this);
        grid->addWidget (label, 3, 0);
    }
    {
        password_line_edit = new QLineEdit ("secret", this);
        connect (password_line_edit, &QLineEdit::returnPressed, this, &AuthorizationWidget::loginClicked);
        password_line_edit->setEchoMode (QLineEdit::Password);
        grid->addWidget (password_line_edit, 3, 1);
    }
    {
        QHBoxLayout* hlayout = new QHBoxLayout;
        {
            QPushButton* button = new QPushButton ("&Add credentials", this);
            connect (button, &QPushButton::clicked, this, &AuthorizationWidget::addCredentialsClicked);
            hlayout->addWidget (button);
        }
        hlayout->addStretch (1);
        {
            QPushButton* button = new QPushButton ("&Connect", this);
            connect (button, &QPushButton::clicked, this, &AuthorizationWidget::loginClicked);
            hlayout->addWidget (button);
        }
        grid->addLayout (hlayout, 4, 0, 1, 2);
    }
    grid->setRowStretch (0, 1);
    grid->setColumnStretch (1, 1);
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

    emit loginRequested ({host, port, login, password});
}
void AuthorizationWidget::emitUpdatedCredentials ()
{
    QVector<AuthorizationCredentials> credentials;
    for (int row_idx = 0; row_idx < table->rowCount (); ++row_idx) {
        QTableWidgetItem* item = table->item (row_idx, 0);
        QString host = item->data (int (CredsRole::Host)).toString ();
        quint16 port = item->data (int (CredsRole::Port)).toInt ();
        QString login = item->data (int (CredsRole::Login)).toString ();
        QString password = item->data (int (CredsRole::Password)).toString ();
        credentials.append ({host, port, login, password});
    }

    emit savedCredentialsUpdated (credentials);
}
void AuthorizationWidget::addCredentialsClicked ()
{
    {
        QString host = host_line_edit->text ();
        quint16 port = port_spin_box->value ();
        QString login = login_line_edit->text ();
        QString password = password_line_edit->text ();

        int new_idx = table->rowCount ();
        table->insertRow (new_idx);

        QTableWidgetItem* item = new QTableWidgetItem (login + "@" + host + ":" + QString::number (port));
        item->setFlags (item->flags () & ~Qt::ItemIsEditable);
        item->setData (int (CredsRole::Host), host);
        item->setData (int (CredsRole::Port), port);
        item->setData (int (CredsRole::Login), login);
        item->setData (int (CredsRole::Password), password);
        table->setItem (new_idx, 0, item);
    }

    emitUpdatedCredentials ();
}
void AuthorizationWidget::removeCredentialsClicked ()
{
    int current_row = table->currentRow ();
    if (current_row < 0)
        return;

    table->removeRow (current_row);

    emitUpdatedCredentials ();
}
void AuthorizationWidget::authorizationCredentialsItemSelected (QTableWidgetItem* item)
{
    QString host = item->data (int (CredsRole::Host)).toString ();
    quint16 port = item->data (int (CredsRole::Port)).toInt ();
    QString login = item->data (int (CredsRole::Login)).toString ();
    QString password = item->data (int (CredsRole::Password)).toString ();

    host_line_edit->setText (host);
    port_spin_box->setValue (port);
    login_line_edit->setText (login);
    password_line_edit->setText (password);
}
void AuthorizationWidget::handleCellDoubleClick (int row, int column)
{
    QTableWidgetItem* item = table->item (row, column);
    if (!item)
        return;

    QString host = item->data (int (CredsRole::Host)).toString ();
    quint16 port = item->data (int (CredsRole::Port)).toUInt ();
    QString login = item->data (int (CredsRole::Login)).toString ();
    QString password = item->data (int (CredsRole::Password)).toString ();

    emit loginRequested ({host, port, login, password});
}
