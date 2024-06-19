#pragma once

#include <QWidget>


class RoleSelectionProgressScreen: public QWidget
{
    Q_OBJECT

public:
    RoleSelectionProgressScreen (QWidget* parent = nullptr);
    ~RoleSelectionProgressScreen ();

signals:
    void quitRequested ();
    void cancelRequested ();
};
