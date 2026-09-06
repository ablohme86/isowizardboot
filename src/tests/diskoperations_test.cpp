#include "diskoperations.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryFile>
#include <QtTest>
#include <unistd.h>

class DiskOperationsTest : public QObject
{
    Q_OBJECT
private slots:
    void onlySafeDisks()
    {
        QJsonObject disk{{"name", "test-device-that-does-not-exist"}, {"path", "/dev/test-device"},
            {"type", "disk"}, {"tran", "usb"}, {"size", 16000000000LL}, {"rm", true}, {"ro", false}};
        QVERIFY(Disks::eligible(disk));
        auto internal = disk; internal["tran"] = "nvme"; internal["rm"] = false;
        QVERIFY(!Disks::eligible(internal));
        auto readOnly = disk; readOnly["ro"] = true;
        QVERIFY(!Disks::eligible(readOnly));
        auto partition = disk; partition["type"] = "part";
        QVERIFY(!Disks::eligible(partition));
        auto root = disk; root["children"] = QJsonArray{QJsonObject{{"mountpoints", QJsonArray{"/"}}}};
        QVERIFY(!Disks::eligible(root));
        auto nested = disk; nested["children"] = QJsonArray{QJsonObject{{"children", QJsonArray{QJsonObject{{"mountpoints", QJsonArray{"/home"}}}}}}};
        QVERIFY(!Disks::eligible(nested));
        auto swap = disk; swap["mountpoints"] = QJsonArray{"[SWAP]"};
        QVERIFY(!Disks::eligible(swap));
        auto mountedUsb = disk; mountedUsb["children"] = QJsonArray{QJsonObject{{"mountpoints", QJsonArray{"/run/media/user/USB"}}}};
        QVERIFY(Disks::eligible(mountedUsb));
        QVERIFY(Disks::mounted(mountedUsb));
        QVERIFY(!Disks::mounted(disk));
        QString error;
        const auto devices = Disks::parseDevices(QJsonDocument(QJsonObject{{"blockdevices", QJsonArray{disk, internal, root, swap}}}).toJson(), &error);
        QVERIFY(error.isEmpty());
        QCOMPARE(devices.size(), 1);
        QCOMPARE(devices[0].toMap()["path"].toString(), QString("/dev/test-device"));
        Disks::parseDevices("invalid", &error);
        QVERIFY(!error.isEmpty());
    }

    void writeAndVerify()
    {
        QTemporaryFile source, target;
        QVERIFY(source.open()); QVERIFY(target.open());
        QByteArray payload(5 * 1024 * 1024 + 127, Qt::Uninitialized);
        for (qsizetype i = 0; i < payload.size(); ++i) payload[i] = char((i * 37 + i / 251) % 256);
        QCOMPARE(source.write(payload), payload.size());
        QVERIFY(source.flush()); QVERIFY(source.seek(0));
        QStringList stages;
        QString error;
        QVERIFY2(Disks::transfer(source.handle(), target.handle(), payload.size(), true,
            [&](const QString &stage, qint64 done, qint64 total) { stages.append(stage); QVERIFY(done <= total); },
            [] { return false; }, &error), qPrintable(error));
        QVERIFY(stages.contains("writing")); QVERIFY(stages.contains("syncing")); QVERIFY(stages.contains("verifying"));
        QVERIFY(target.seek(0)); QCOMPARE(target.readAll(), payload);
    }

    void detectsReadbackCorruption()
    {
        QTemporaryFile source, target;
        QVERIFY(source.open()); QVERIFY(target.open());
        source.write("original contents"); source.flush(); source.seek(0);
        QString error;
        const bool result = Disks::transfer(source.handle(), target.handle(), source.size(), true,
            [&](const QString &stage, qint64, qint64) {
                if (stage == "verifying") QCOMPARE(::pwrite(target.handle(), "X", 1, 0), ssize_t(1));
            }, [] { return false; }, &error);
        QVERIFY(!result); QVERIFY(error.contains("samsvarer ikke"));
    }

    void cancelBeforeWrite()
    {
        QTemporaryFile source, target;
        QVERIFY(source.open()); QVERIFY(target.open());
        source.write("image"); source.flush(); source.seek(0);
        target.write("keep this data"); target.flush(); target.seek(0);
        QString error;
        QVERIFY(!Disks::transfer(source.handle(), target.handle(), source.size(), false,
            [](const QString &, qint64, qint64) {}, [] { return true; }, &error));
        target.seek(0); QCOMPARE(target.readAll(), QByteArray("keep this data"));
        QVERIFY(error.startsWith("Avbrutt"));
    }

    void rejectsTruncatedSource()
    {
        QTemporaryFile source, target;
        QVERIFY(source.open()); QVERIFY(target.open());
        source.write("short"); source.flush(); source.seek(0);
        QString error;
        QVERIFY(!Disks::transfer(source.handle(), target.handle(), 500, false,
            [](const QString &, qint64, qint64) {}, [] { return false; }, &error));
        QVERIFY(error.contains("kortere"));
    }

    void fileIdentityChanges()
    {
        QTemporaryFile source;
        QVERIFY(source.open()); source.write("a"); source.flush();
        const auto before = Disks::imageIdentity(source.fileName());
        QVERIFY(!before.isEmpty()); source.write("b"); source.flush();
        QVERIFY(before != Disks::imageIdentity(source.fileName()));
        QVERIFY(Disks::imageIdentity("/tmp").isEmpty());
    }
};

QTEST_GUILESS_MAIN(DiskOperationsTest)
#include "diskoperations_test.moc"
