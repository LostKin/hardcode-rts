#pragma once

#include <QWidget>


class RoleSelectionScreen: public QWidget
{
    Q_OBJECT

public:
    RoleSelectionScreen (QWidget* parent = nullptr);
    ~RoleSelectionScreen ();

signals:
    void selectRolePlayerRequested ();
    void spectateRequested ();
    void quitRequested ();
};
