#pragma once

#include <QJsonObject>
#include <QStringList>
#include <QVariantList>
#include <functional>

namespace Disks {
QString formatBytes(qint64 bytes);
bool eligible(const QJsonObject &disk);
bool mounted(const QJsonObject &disk);
QVariantList parseDevices(const QByteArray &json, QString *error);
QVariantList scan(QString *error);
QString imageIdentity(const QString &path);
// File descriptors allow the data path to be tested with temporary files.
bool transfer(int source, int target, qint64 size, bool verify,
              const std::function<void(const QString &, qint64, qint64)> &progress,
              const std::function<bool()> &cancelled, QString *error);
int runWriter(const QStringList &arguments);
}
