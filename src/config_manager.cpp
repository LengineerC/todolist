#include "config_manager.h"

ConfigManager::ConfigManager(QObject *parent) {
    if (!m_configWatcher.addPath(Config::getConfigPath())) {
        qWarning() << "Failed to add filepath to watcher: "
                   << Config::getConfigPath();
    }
    qDebug() << "config path: " << Config::getConfigPath();
    m_configJson = getDefaultConfig();

    connect(&m_configWatcher, &QFileSystemWatcher::fileChanged, this,
            &ConfigManager::onConfigChange);
}

QJsonObject ConfigManager::getDefaultConfig() {
    QJsonObject defaultConfig;
    defaultConfig["borderRadius"] = 10;
    defaultConfig["theme"] = "light";
    defaultConfig["todoWrapMode"] = "force";
    defaultConfig["windowWidth"] = Config::width;
    defaultConfig["windowHeight"] = Config::height;
    defaultConfig["windowX"] = QJsonValue(QJsonValue::Null);
    defaultConfig["windowY"] = QJsonValue(QJsonValue::Null);
    defaultConfig["timerInitialSeconds"] = 1500;

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

QJsonObject ConfigManager::getConfig() { return m_configJson; }

bool ConfigManager::readConfigJson() {
    QFile file(Config::getConfigPath());

    if (!file.exists()) {
        qDebug() << "config.json not exists, create default options";

        m_configJson = getDefaultConfig();
        writeConfigJson(m_configJson);
        m_configWatcher.addPath(Config::getConfigPath());

        return QFile::exists(Config::getConfigPath());
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
            m_configJson = getDefaultConfig();
            writeConfigJson(m_configJson);
            return true;
        }

        QJsonObject mergedConfig = getDefaultConfig();
        const QJsonObject loadedConfig = document.object();
        for (auto it = loadedConfig.constBegin(); it != loadedConfig.constEnd();
             ++it) {
            mergedConfig[it.key()] = it.value();
        }

        m_configJson = mergedConfig;

        return true;
    } catch (std::exception e) {
        qDebug() << "Failed to read config.json, using default json data";
        return false;
    }
}

void ConfigManager::writeConfigJson(QJsonObject configJson) {
    m_configJson = configJson;

    QFile file(Config::getConfigPath());

    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument document(configJson);
        file.write(document.toJson());
        file.close();

        qDebug() << "Succeed to create config.json";
    } else {
        qDebug() << "Failed to create config.json";
    }
}
