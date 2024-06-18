#pragma once

#include "roomentry.h"

#include <QDialog>


class ProfilePopup: public QDialog
{
    Q_OBJECT

public:
    ProfilePopup (const QString& login, QWidget* parent = nullptr);
    ~ProfilePopup ();

signals:
    void logoutRequested ();
};
