#include "config_manager.h";

ConfigManager::ConfigManager(QObject *parent) {
    if (!m_configWatcher.addPath(Config::configPath)) {
        qWarning() << "Failed to add filepath to watcher: "
                   << Config::configPath;
    }

    m_configJson = getDefaultConfig();

    connect(&m_configWatcher, &QFileSystemWatcher::fileChanged, this,
            &ConfigManager::onConfigChange);
}

QJsonObject ConfigManager::getDefaultConfig() {
    QJsonObject defaultConfig;
    defaultConfig["borderRadius"] = 10;
    defaultConfig["theme"] = "light";

    return defaultConfig;
}

void ConfigManager::onConfigChange() {
    qDebug() << "config.json changed";

    emit configUpdated();
}

ConfigManager &ConfigManager::instance() {
    static ConfigManager s_instance;
    return s_instance;
}

bool ConfigManager::readConfigJson() {
    QFile file(Config::configPath);

    if (!file.exists()) {
        qDebug() << "config.json not exists, create default options";
        // TODO:
        // create default config

        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open config.json for reading, using default "
                    "json data";
        return false;
    }

    QByteArray data = file.readAll();

    try {
        QJsonDocument document = QJsonDocument::fromJson(data);

        if (document.isNull()) {
            qDebug() << "Invalid JSON format, creating default config.json";
        }

        m_configJson = document.object();

        return true;
    } catch (std::exception e) {
        qDebug() << "Failed to read config.json, using default json data";
        return false;
    }
}

void ConfigManager::writeConfigJson(QJsonObject configJson) {
    QFile file(Config::configPath);

    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument document(configJson);
        file.write(document.toJson());
        file.close();

        qDebug() << "Succeed to create config.json";
    } else {
        qDebug() << "Failed to create config.json";
    }
}
