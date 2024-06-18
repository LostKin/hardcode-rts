#pragma once

#include <QWidget>

class AuthorizationProgressWidget: public QWidget
{
    Q_OBJECT

public:
    AuthorizationProgressWidget (const QString& message, QWidget* parent = nullptr);
    ~AuthorizationProgressWidget ();

signals:
    void cancelRequested ();
};
