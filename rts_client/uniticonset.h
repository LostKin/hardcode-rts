#pragma once

#include <QSharedPointer>

class QOpenGLTexture;
class QImage;


class UnitIconSet
{
public:
    UnitIconSet ();

private:
    static QSharedPointer<QOpenGLTexture> loadTexture2D (const QImage& image);

public:
    QSharedPointer<QOpenGLTexture> seal;
    QSharedPointer<QOpenGLTexture> crusader;
    QSharedPointer<QOpenGLTexture> goon;
    QSharedPointer<QOpenGLTexture> beetle;
    QSharedPointer<QOpenGLTexture> contaminator;
};
