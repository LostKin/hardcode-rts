#pragma once

#include "authorizationcredentials.h"

#include <QWidget>


class QLineEdit;
class QSpinBox;
class QTableWidget;
class QTableWidgetItem;
class AuthorizationCredentials;


class AuthorizationScreen: public QWidget
{
    Q_OBJECT

public:
    AuthorizationScreen (const QVector<AuthorizationCredentials>& initial_credentials, QWidget* parent = nullptr);
    ~AuthorizationScreen ();
    void start ();

signals:
    void windowsCloseRequested ();
    void loginRequested (const AuthorizationCredentials& credentials);
    void savedCredentialsUpdated (const QVector<AuthorizationCredentials>& credentials);

protected:
    void closeEvent (QCloseEvent*) override;

private:
    void emitUpdatedCredentials ();

private slots:
    void loginClicked ();
    void addCredentialsClicked ();
    void removeCredentialsClicked ();
    void authorizationCredentialsItemSelected (QTableWidgetItem* item);
    void handleCellDoubleClick (int row, int column);

private:
    QTableWidget* table;
    QLineEdit* host_line_edit;
    QSpinBox* port_spin_box;
    QLineEdit* login_line_edit;
    QLineEdit* password_line_edit;
};
