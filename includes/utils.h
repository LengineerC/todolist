#pragma once
#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QString>
#include <QSvgRenderer>

namespace Utils {
inline QString colorToRgba(const QColor &color, int alpha = -1) {
    int r, b, g, a;
    color.getRgb(&r, &g, &b, &a);
    int finalAlpha = (alpha != -1) ? alpha : a;
    return QString("rgba(%1, %2, %3, %4)").arg(r).arg(g).arg(b).arg(alpha);
}

inline QIcon getColoredSvg(const QString &path, const QColor &color,
                           const QSize &size = QSize(64, 64)) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QIcon();
    }

    QString svgContent = QString::fromUtf8(file.readAll());
    file.close();

    QString hexRgb = color.name(QColor::HexRgb);
    double opacity = color.alphaF();
    // qDebug() << "svg color: " << color;
    svgContent.replace("<path",
                       QString("<path fill=\"%1\" fill-opacity=\"%2\" ")
                           .arg(hexRgb)
                           .arg(opacity));

    qDebug() << svgContent.toUtf8();
    QSvgRenderer renderer(svgContent.toUtf8());
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    renderer.render(&painter);
    painter.end();

    return QIcon(QPixmap::fromImage(image));
}
} // namespace Utils
