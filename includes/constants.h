#pragma once

namespace Config {
const int width = 500;
const int height = 600;

struct Theme {
    QColor textColor;
    QColor backgroundColor;

    Theme(const QColor &_textColor, const QColor &_backgroundColor)
        : textColor(_textColor), backgroundColor(_backgroundColor) {}
};

struct Themes {
    static inline const Theme light{QColor(0, 0, 0),
                                    QColor(255, 255, 255, 150)};
    static inline const Theme dark{QColor(255, 255, 255),
                                   QColor(45, 45, 45, 150)};
};

const QString configPath =
    QCoreApplication::applicationDirPath() + "/config.json";
const int borderRadius = 10;

} // namespace Config
