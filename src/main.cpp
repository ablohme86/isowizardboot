#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QCommandLineParser>
#include <QQuickWindow>
#include <QImage>
#include <QIcon>
#include <QTimer>
#include "backend.h"
#include "diskoperations.h"
#include "settings.h"

int main(int argc, char *argv[])
{
    if (argc > 1 && QByteArray(argv[1]) == "--write-image") {
        QCoreApplication app(argc, argv);
        return Disks::runWriter(app.arguments());
    }
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);
    app.setApplicationName("IsoWizardBoot");
    app.setOrganizationName("IsoWizardBoot");
    app.setApplicationVersion("1.0.0");
    app.setDesktopFileName("isowizardboot");
    QIcon appIcon;
    for (int size : {16, 24, 32, 48, 64, 128, 256, 512})
        appIcon.addFile(QStringLiteral(":/icons/isowizardboot-%1.png").arg(size));
    app.setWindowIcon(appIcon);
    QCommandLineParser parser;
    parser.setApplicationDescription("Write USB-compatible ISO/IMG files to USB devices.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({{"i", "image"}, "Preselect an ISO/IMG file.", "file"});
    parser.addOption({"screenshot", "Save a screenshot and exit (for UI testing).", "file"});
    parser.process(app);

    Settings settings;
    Backend backend;
    if (parser.isSet("image")) backend.selectImage(QUrl::fromLocalFile(parser.value("image")));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("settings", &settings);
    QObject::connect(&settings, &Settings::languageChanged, &engine, &QQmlApplicationEngine::retranslate);
    QObject::connect(&settings, &Settings::languageChanged, &backend, &Backend::retranslate);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    engine.loadFromModule("isowizard", "Main");
    if (parser.isSet("screenshot")) {
        QTimer::singleShot(1500, &app, [&]() {
            auto *window = engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            app.exit(window && window->grabWindow().save(parser.value("screenshot")) ? 0 : 1);
        });
    }

    return app.exec();
}
