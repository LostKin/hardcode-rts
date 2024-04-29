#pragma once

class QString;
class QByteArray;
class QImage;
class QColor;


class UnitGenerator
{
public:
    static QImage loadUnitFromSVGTemplate (const QString& path, const QByteArray& animation_name, const QColor& player_color);
};
