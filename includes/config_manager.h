#pragma once

#include "constants.h"
#include <QFile>
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>

class ConfigManager : public QObject {
    Q_OBJECT

  private:
    explicit ConfigManager(QObject *parent = nullptr);
    QJsonObject getDefaultConfig();

    QJsonObject m_configJson;
    QFileSystemWatcher m_configWatcher;

  signals:
    void configUpdated();

  private slots:
    void onConfigChange();

  public:
    static ConfigManager &instance();

    bool readConfigJson();

    void writeConfigJson(QJsonObject configJson);

    ConfigManager(const ConfigManager &) = delete;

    ConfigManager operator=(const ConfigManager &) = delete;
};
