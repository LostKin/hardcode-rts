#pragma once

#include <QWidget>


class QPushButton;
class QLabel;


class ReadinessScreen: public QWidget
{
    Q_OBJECT

public:
    ReadinessScreen (QWidget* parent = nullptr);
    ~ReadinessScreen ();

signals:
    void readinessRequested ();
    void quitRequested ();

private slots:
    void readynessClickCallback ();

private:
    QPushButton* ready_button;
    QLabel* ready_label;
};
