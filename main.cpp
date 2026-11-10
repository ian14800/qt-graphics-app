// Copyright 2026 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
//

#include "ArcGISRuntimeEnvironment.h"
#include "Authentication/AuthenticationManager.h"
#include "Authentication/TokenCredential.h"
#include "Authentication/ArcGISAuthenticationChallenge.h"
#include "Authentication/ArcGISAuthenticationChallengeHandler.h"
#include "QtGraphicsTester.h"

#include <QDir>
#include <QFuture>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>
#include <QDebug>
#include <QSslSocket>
#include <QFileInfo>

using namespace Esri::ArcGISRuntime;
using namespace Esri::ArcGISRuntime::Authentication;

class SimpleCredentialHandler : public ArcGISAuthenticationChallengeHandler
{
public:
    SimpleCredentialHandler(const QString& username, const QString& password)
        : m_username(username), m_password(password) {}


    void handleArcGISAuthenticationChallenge(ArcGISAuthenticationChallenge* challenge) override
    {
        TokenCredential::createWithChallengeAsync(
            challenge,
            m_username,
            m_password,
            std::nullopt
        ).then([challenge](TokenCredential* credential)
        {
            challenge->continueWithCredential(credential);
        });
    }

private:
    QString m_username;
    QString m_password;
};


int main(int argc, char *argv[])
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    qputenv("QT_ENABLE_FEATURE_TOGGLE_MOBILE", "true");
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if (qEnvironmentVariableIsEmpty("QT_FEATURE_TOGGLE_DIR")) {
        const QString appDir = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath();
        qputenv("QT_FEATURE_TOGGLE_DIR", QDir(appDir).absolutePath().toUtf8());
    }
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setVersion(3, 2);
    QSurfaceFormat::setDefaultFormat(fmt);
#endif

    QGuiApplication app(argc, argv);

    QFile envFile(":/config/.env");
    if (!envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Could not read bundeled credentials file";
        return 1;
    }

    QHash<QString, QString> env;
    while(!envFile.atEnd()) {
        const QString line = QString::fromUtf8(envFile.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        const qsizetype separator = line.indexOf('=');
        if (separator <= 0) {
            qCritical() << "Invalid line in bundled credentials file";
            return 1;
        }

        env.insert(line.left(separator).trimmed(), line.mid(separator + 1).trimmed());
    }

    const QString username = env.value("ARCGIS_USERNAME");
    const QString password = env.value("ARCGIS_PASSWORD");
    if (username.isEmpty() || password.isEmpty()) {
        qCritical() << "ArcGIS credentials are missing from bundled credentials file";
        return 1;
    }

    auto* handler = new SimpleCredentialHandler(username, password);

    ArcGISRuntimeEnvironment::authenticationManager()->setArcGISAuthenticationChallengeHandler(handler);

    QtGraphicsTester::init();

    QQmlApplicationEngine engine;

    engine.addImportPath(QDir(QCoreApplication::applicationDirPath()).filePath("qml"));
    const QUrl mainUrl(QStringLiteral("qrc:/qml/main.qml"));
    engine.load(mainUrl);
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to create the main QML window:" << mainUrl;
        return 1;
    }

    return QGuiApplication::exec();
}
