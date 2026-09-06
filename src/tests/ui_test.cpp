#include "backend.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryFile>
#include <QtTest>

class UiTest : public QObject
{
    Q_OBJECT
private slots:
    void imageSelectionAndConfirmation()
    {
        Backend backend;
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
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
            QVERIFY(backend.detail().contains("byttet ut"));
        }
    }
};

int main(int argc, char **argv)
{
    QQuickStyle::setStyle("Basic");
    QGuiApplication application(argc, argv);
    UiTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "ui_test.moc"
