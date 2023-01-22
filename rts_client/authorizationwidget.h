#pragma once

#include <QWidget>

class QLineEdit;
class QSpinBox;

class AuthorizationWidget: public QWidget
{
    Q_OBJECT

public:
    AuthorizationWidget (QWidget* parent = nullptr);
    ~AuthorizationWidget ();
    void start ();

signals:
    void windowsCloseRequested ();
    void loginRequested (const QString& host, quint16 port, const QString& login, const QString& password);

protected:
    void closeEvent (QCloseEvent*) override;

private slots:
    void loginClicked ();

private:
    QLineEdit* host_line_edit;
    QSpinBox* port_spin_box;
    QLineEdit* login_line_edit;
    QLineEdit* password_line_edit;
};
