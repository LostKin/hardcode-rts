#pragma once

#include <QWidget>


class QGridLayout;


class MainWindow: public QWidget
{
    Q_OBJECT

public:
    MainWindow (QWidget* parent = nullptr);

    void setWidget (QWidget* widget, bool fullscreen = false);

private:
    QGridLayout* layout;
    QWidget* widget = nullptr;
};
