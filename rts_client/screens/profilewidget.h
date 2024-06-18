#pragma once

#include "roomentry.h"

#include <QDialog>

class ProfileWidget: public QDialog
{
    Q_OBJECT

public:
    ProfileWidget (const QString& login, QWidget* parent = nullptr);
    ~ProfileWidget ();

signals:
    void logoutRequested ();
};
