#include "config_manager.h"
#include "widget.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>

int main(int argc, char *argv[]) {
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);

    ConfigManager::instance().readConfigJson();

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
