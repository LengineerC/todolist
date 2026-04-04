#include "config_manager.h"
#include "db_manager.h"
#include "widget.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>

int main(int argc, char *argv[]) {
#if defined(Q_OS_WIN)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::Floor);
#elif defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    QApplication a(argc, argv);

    ConfigManager::instance().readConfigJson();
    DbManager::instance().init();

    int fontId = QFontDatabase::addApplicationFont(":/fonts/main_font");
    if (fontId != -1) {
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        QFont defaultFont(family);
        QApplication::setFont(defaultFont);
    }

    Widget w;
    w.show();
    return a.exec();
}
