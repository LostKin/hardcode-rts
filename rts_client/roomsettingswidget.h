#pragma once

#include <QDialog>

class QLineEdit;

class RoomSettingsWidget: public QDialog
{
    Q_OBJECT

public:
    RoomSettingsWidget (QWidget* parent = nullptr);
    ~RoomSettingsWidget ();

signals:
    void createRequested (const QString& name);

private slots:
    void createClicked ();

private:
    QLineEdit* room_name_line_edit;
};
