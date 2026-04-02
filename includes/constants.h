#pragma once
#include <qcolor.h>
#include <qcoreapplication.h>

namespace Config {
inline const int width = 500;
inline const int height = 600;

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

    static inline Theme getTheme(QString theme) {
        return (theme == "dark") ? Themes::dark : Themes::light;
    }
};

inline QString getConfigPath() {
    return QCoreApplication::applicationDirPath() + "/config.json";
}
inline const int borderRadius = 10;

} // namespace Config
