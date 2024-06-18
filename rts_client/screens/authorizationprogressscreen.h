#pragma once

#include <QWidget>


class AuthorizationProgressScreen: public QWidget
{
    Q_OBJECT

public:
    AuthorizationProgressScreen (const QString& message, QWidget* parent = nullptr);
    ~AuthorizationProgressScreen ();

signals:
    void cancelRequested ();
};
