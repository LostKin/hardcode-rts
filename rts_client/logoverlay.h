#pragma once

#include <QString>
#include <QFont>
#include <QRect>
#include <QSize>
#include <QColor>


class LogOverlay
{
public:
    LogOverlay ();
    void toggleShow ();
    void pageUp ();
    void pageDown ();
    void lineUp ();
    void lineDown ();
    void resize (const QSize& size);
    void draw (QPaintDevice* paint_device);
    void log (const QString& message);

private:
    bool show = false;
    QStringList lines;
    int offset = 0;
    QFont font;
    QSize size;
    QRect shade_rect;
    QRect arrows_rect;
    QRect text_rect;
    int visible_line_count = 0;
    int text_height = 0;
    QColor shade_color;
    QColor text_color;
    QColor arrows_color;
};
