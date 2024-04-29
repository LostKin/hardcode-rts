#include "uniticonset.h"

#include "unitgenerator.h"

#include <QOpenGLTexture>


UnitIconSet::UnitIconSet ()
{
    static constexpr QColor icon_color (0xdd, 0xdd, 0xdd);

    seal = loadTexture2D (UnitGenerator::loadUnitFromSVGTemplate (":/images/units/seal/unit_tmpl.svg", "standing", icon_color));
    crusader = loadTexture2D (UnitGenerator::loadUnitFromSVGTemplate (":/images/units/crusader/unit_tmpl.svg", "standing", icon_color));
    goon = loadTexture2D (UnitGenerator::loadUnitFromSVGTemplate (":/images/units/goon/unit_tmpl.svg", "standing", icon_color));
    beetle = loadTexture2D (UnitGenerator::loadUnitFromSVGTemplate (":/images/units/beetle/unit_tmpl.svg", "standing", icon_color));
    contaminator = loadTexture2D (UnitGenerator::loadUnitFromSVGTemplate (":/images/units/contaminator/unit_tmpl.svg", "standing", icon_color));
}

QSharedPointer<QOpenGLTexture> UnitIconSet::loadTexture2D (const QImage& image)
{
    QOpenGLTexture* texture = new QOpenGLTexture (image);
    texture->setMinificationFilter (QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter (QOpenGLTexture::LinearMipMapLinear);
    return QSharedPointer<QOpenGLTexture> (texture);
}
