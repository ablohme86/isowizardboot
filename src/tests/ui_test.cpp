#include "backend.h"
#include "settings.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QSettings>
#include <QtTest>

class UiTest : public QObject
{
    Q_OBJECT
private slots:
    void init() { QSettings().clear(); }
    void imageSelectionAndConfirmation()
    {
        Settings settings;
        Backend backend;
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        engine.rootContext()->setContextProperty("settings", &settings);
        engine.load(QUrl::fromLocalFile(QStringLiteral(MAIN_QML_PATH)));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        auto *writeButton = window->findChild<QQuickItem *>("writeButton");
        QVERIFY(writeButton);
        QVERIFY(!writeButton->isEnabled());
        QVERIFY(!window->property("ready").toBool());
        QTemporaryFile image("/tmp/isowizard-ui-XXXXXX.iso");
        QVERIFY(image.open());
        image.write(QByteArray(1024 * 1024, 'I')); image.flush();
        backend.selectImage(QUrl::fromLocalFile(image.fileName()));
        QCOMPARE(backend.imageSize(), 1024 * 1024);
        QVERIFY(!writeButton->isEnabled());
        backend.refreshDevices();
        QVariantMap device;
        for (const auto &entry : backend.devices()) {
            if (!entry.toMap()["mounted"].toBool()) { device = entry.toMap(); break; }
        }
        if (!device.isEmpty()) {
            window->setProperty("selectedPath", device["path"]);
            QVERIFY(QMetaObject::invokeMethod(window, "updateSelection"));
            QVERIFY(writeButton->isEnabled());
            QVERIFY(QMetaObject::invokeMethod(writeButton, "clicked"));
            auto *dialog = window->findChild<QObject *>("confirmDialog");
            auto *confirm = window->findChild<QQuickItem *>("confirmWriteButton");
            QVERIFY(dialog); QVERIFY(confirm);
            QVERIFY(dialog->property("visible").toBool());
            QVERIFY(!confirm->isEnabled());
            QVERIFY(!backend.busy());
            QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
            QVERIFY(!backend.busy());
            window->setProperty("selectedPath", "/dev/isowizard-nonexistent");
            QVERIFY(QMetaObject::invokeMethod(window, "updateSelection"));
            QVERIFY(!writeButton->isEnabled());
        }
        window->resize(820, 740);
        QTest::qWait(100);
        QVERIFY(!window->grabWindow().isNull());
        QVERIFY(writeButton->width() > 100);
        const auto position = writeButton->mapToScene(QPointF(0, 0));
        QVERIFY(position.x() >= 0);
        QVERIFY(position.x() + writeButton->width() <= window->width());
    }

    void refusesMissingAndReplacedTargets()
    {
        Backend backend;
        backend.start("/dev/isowizard-nonexistent", "nonexistent", true);
        QVERIFY(!backend.busy());
        QCOMPARE(backend.stage(), QString("error"));
        if (!backend.devices().isEmpty()) {
            backend.start(backend.devices().first().toMap()["path"].toString(), "wrong-device-identity", true);
            QVERIFY(!backend.busy());
            QVERIFY(backend.detail().contains("replaced"));
        }
    }

    void preferencesSwitchLiveAndPersist()
    {
        Settings settings;
        Backend backend;
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        engine.rootContext()->setContextProperty("settings", &settings);
        connect(&settings, &Settings::languageChanged, &engine, &QQmlApplicationEngine::retranslate);
        connect(&settings, &Settings::languageChanged, &backend, &Backend::retranslate);
        engine.load(QUrl::fromLocalFile(QStringLiteral(MAIN_QML_PATH)));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        auto click = [&](const char *name) {
            auto *button = window->findChild<QQuickItem *>(name);
            return button && QMetaObject::invokeMethod(button, "clicked");
        };
        QCOMPARE(settings.language(), QString("en"));
        QCOMPARE(settings.theme(), QString("dark"));
        QCOMPARE(backend.status(), QString("Ready when you are"));
        QVERIFY(click("settingsButton"));
        auto *dialog = window->findChild<QObject *>("settingsDialog");
        QVERIFY(dialog); QVERIFY(dialog->property("visible").toBool());
        QVERIFY(click("norwegianButton"));
        QCOMPARE(settings.language(), QString("no-NB"));
        QCOMPARE(dialog->property("title").toString(), QString("Innstillinger"));
        QCOMPARE(backend.status(), QString("Klar når du er"));
        QCOMPARE(window->findChild<QObject *>("writeButton")->property("text").toString(), QString("Skriv til USB  →"));
        QVERIFY(click("lightThemeButton"));
        QVERIFY(window->property("lightTheme").toBool());
        QCOMPARE(window->color(), QColor("#f7f7f4"));
        const QString captureDir = qEnvironmentVariable("ISOWIZARD_TEST_SCREENSHOTS");
        QTest::qWait(100);
        if (!captureDir.isEmpty()) QVERIFY(window->grabWindow().save(captureDir + "/settings-norwegian-light.png"));
        QCOMPARE(QSettings().value("appearance/language").toString(), QString("no-NB"));
        QCOMPARE(QSettings().value("appearance/theme").toString(), QString("light"));
        {
            Settings restored;
            QCOMPARE(restored.language(), QString("no-NB"));
            QCOMPARE(restored.theme(), QString("light"));
        }
        QVERIFY(click("englishButton"));
        QCOMPARE(dialog->property("title").toString(), QString("Settings"));
        QCOMPARE(backend.status(), QString("Ready when you are"));
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        QTest::qWait(100);
        if (!captureDir.isEmpty()) QVERIFY(window->grabWindow().save(captureDir + "/english-light.png"));
        settings.setTheme("dark");
        QVERIFY(!window->property("lightTheme").toBool());
        settings.setLanguage("invalid");
        settings.setTheme("invalid");
        QCOMPARE(settings.language(), QString("en"));
        QCOMPARE(settings.theme(), QString("dark"));
    }
};

int main(int argc, char **argv)
{
    QQuickStyle::setStyle("Basic");
    QGuiApplication application(argc, argv);
    application.setOrganizationName("IsoWizardBootTests");
    application.setApplicationName("UiTests");
    QTemporaryDir settingsDirectory;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    UiTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "ui_test.moc"
