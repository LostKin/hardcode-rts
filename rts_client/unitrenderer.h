#pragma once

#include <QSharedPointer>

class QOpenGLTexture;
class QString;
class Unit;


class UnitRenderer
{
public:
    UnitRenderer (const QString& name);
    void loadResources (const QColor& team_color);
    void draw (const Unit& unit);

private:
    static QImage loadUnitFromSVGTemplate (const QString& path, const QByteArray& animation_name, const QColor& player_color);
    static QSharedPointer<QOpenGLTexture> loadTexture2D (const QImage& image);

private:
    QString name;
    QSharedPointer<QOpenGLTexture> standing;
    QSharedPointer<QOpenGLTexture> walking1;
    QSharedPointer<QOpenGLTexture> walking2;
    QSharedPointer<QOpenGLTexture> shooting1;
    QSharedPointer<QOpenGLTexture> shooting2;
};
