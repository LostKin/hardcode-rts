#include "logoverlay.h"

#include <QFontDatabase>
#include <QPainter>
#include <QPicture>


LogOverlay::LogOverlay ()
    : shade_color (0, 0, 0, 128)
    , text_color (0, 255, 0)
    , arrows_color (128, 128, 255)
{
    {
        const char* env_str = getenv ("RTS_SHOW_LOG");
        show = env_str && !strcmp (env_str, "1");
    }
    {
        font.setStyleHint (QFont::TypeWriter);
        QStringList families = QFontDatabase::families ();
        if (families.contains ("Anka/Coder"))
            font.setFamily ("Anka/Coder");
        else
            font.setFamily ("Courier New");
        font.setPointSize (12);
    }
}

void LogOverlay::toggleShow ()
{
    show = !show;
}
void LogOverlay::pageUp ()
{
    offset = qMin (lines.size () - visible_line_count, offset + visible_line_count/2);
}
void LogOverlay::pageDown ()
{
    offset = qMax (0, offset - visible_line_count/2);
}
void LogOverlay::lineUp ()
{
    offset = qMin (lines.size () - visible_line_count, offset + 1);
}
void LogOverlay::lineDown ()
{
    offset = qMax (0, offset - 1);
}
void LogOverlay::resize (const QSize& size)
{
    this->size = size;
    QRect r (QPoint (0, 0), size);
    shade_rect = r.marginsAdded (QMargins (-10, -10, -10, -10));
    arrows_rect = r.marginsAdded (QMargins (-20, -20, -20, -20));

    {
        QPicture pic;
        QPainter p (&pic);
        p.setFont (font);
        QSizeF single_glyph_size = p.boundingRect (QRectF (), Qt::AlignLeft | Qt::AlignTop, "A").size ();
        qreal glyph_h = p.boundingRect (QRectF (), Qt::AlignLeft | Qt::AlignTop, "A\nA").height () - single_glyph_size.height ();
        visible_line_count = qFloor (arrows_rect.height ()/glyph_h);
        text_height = p.boundingRect (QRect (), Qt::AlignLeft | Qt::AlignTop, QString ("A\n").repeated (visible_line_count - 1) + "A").height ();
        text_rect = arrows_rect.marginsAdded (QMargins (-single_glyph_size.width ()*2, 0, -single_glyph_size.width ()*2, 0));
        text_rect.translate (0, (text_rect.height () - text_height)/2);
    }
}
void LogOverlay::draw (QPaintDevice* paint_device)
{
    if (!show || !visible_line_count)
        return;

    QPainter p (paint_device);

    p.fillRect (shade_rect, shade_color);
    p.setPen (text_color);
    p.setFont (font);
    int first_line = qMax (int (lines.size () - visible_line_count - offset), int (0));
    QString text;
    int max_line = qMin (int (first_line + visible_line_count), int (lines.size ()));
    for (int i = first_line; i < max_line; ++i) {
        if (i > first_line)
            text += '\n';
        text += lines[i];
    }
    p.drawText (text_rect, Qt::AlignLeft | Qt::AlignTop, text);
    p.setPen (arrows_color);
    if (first_line > 0)
        p.drawText (arrows_rect, Qt::AlignLeft | Qt::AlignTop, "⭡");
    if ((first_line + visible_line_count) < lines.size ())
        p.drawText (arrows_rect, Qt::AlignLeft | Qt::AlignBottom, "⭣");
}
void LogOverlay::log (const QString& message)
{
    QStringList new_lines = message.split ('\n');
    if (offset)
        offset += new_lines.size ();
    lines += new_lines;
    if (lines.size () > 8192)
        lines.remove (0, lines.size () - 4096);
}
