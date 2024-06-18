#include "mainwindow.h"

#include <QGridLayout>
#include <QPushButton>


MainWindow::MainWindow (QWidget* parent)
    : QWidget (parent)
{
    layout = new QGridLayout;
    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (0);
    setLayout (layout);
}

void MainWindow::setWidget (QWidget* widget, bool fullscreen)
{
    if (fullscreen) {
        layout->setColumnStretch (0, 0);
        layout->setColumnStretch (1, 1);
        layout->setColumnStretch (2, 0);
        layout->setRowStretch (0, 0);
        layout->setRowStretch (1, 1);
        layout->setRowStretch (2, 0);
    } else {
        layout->setColumnStretch (0, 1);
        layout->setColumnStretch (1, 0);
        layout->setColumnStretch (2, 1);
        layout->setRowStretch (0, 1);
        layout->setRowStretch (1, 0);
        layout->setRowStretch (2, 1);
    }
    layout->addWidget (widget, 1, 1);
    widget->show ();
    if (this->widget)
        this->widget->deleteLater ();
    this->widget = widget;
}
