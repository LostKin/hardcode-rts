#pragma once

#include <QDialog>


class QLineEdit;


class RoomSettingsPopup: public QDialog
{
    Q_OBJECT

public:
    RoomSettingsPopup (QWidget* parent = nullptr);
    ~RoomSettingsPopup ();

signals:
    void createRequested (const QString& name);

private slots:
    void createClicked ();

private:
    QLineEdit* room_name_line_edit;
};
