#pragma once
#include <QColor>
#include <QString>

namespace Utils {
inline QString colorToRgba(const QColor &color, int alpha = -1) {
    int r, b, g, a;
    color.getRgb(&r, &g, &b, &a);
    int finalAlpha = (alpha != -1) ? alpha : a;
    return QString("rgba(%1, %2, %3, %4)").arg(r).arg(g).arg(b).arg(alpha);
}
} // namespace Utils
