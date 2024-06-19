#include "mainwindow.h"

#include <QGridLayout>
#include <QPushButton>


MainWindow::MainWindow (QWidget* parent)
    : QWidget (parent)
{
    layout = new QGridLayout;
    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (0);
    layout->setColumnStretch (0, 1);
    layout->setRowStretch (0, 1);
    setLayout (layout);
}

void MainWindow::setWidget (QWidget* widget)
{
    layout->addWidget (widget, 0, 0);
    widget->show ();
    if (this->widget)
        this->widget->deleteLater ();
    this->widget = widget;
}
