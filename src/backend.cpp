#include "backend.h"
#include "diskoperations.h"
#include "settings.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

Backend::Backend(QObject *parent) : QObject(parent)
{
    connect(&m_refresh, &QTimer::timeout, this, &Backend::refreshDevices);
    m_refresh.start(4000);
    QTimer::singleShot(0, this, &Backend::refreshDevices);
    connect(&m_writer, &QProcess::readyReadStandardOutput, this, &Backend::readEvents);
    connect(&m_writer, &QProcess::readyReadStandardError, this, [this]() {
        const auto text = QString::fromLocal8Bit(m_writer.readAllStandardError()).trimmed();
        if (!text.isEmpty()) appendLog(text);
    });
    connect(&m_writer, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_busy = false;
            fail("Could not start pkexec. Install polkit to write USB devices.");
        }
    });
    connect(&m_writer, &QProcess::finished, this, [this](int code, QProcess::ExitStatus exitStatus) {
        readEvents();
        m_busy = false;
        if (!m_terminalEvent) {
            if (m_cancelling && m_stage == "authorizing") {
                m_stage = "cancelled";
                m_status = "Operation cancelled";
                m_detail = "Administrator approval was cancelled. Nothing was written.";
                appendLog(m_detail);
            } else fail(code == 126 ? "Administrator approval was cancelled. Nothing was written."
                 : code == 127 ? "Could not obtain administrator approval. Check that a polkit agent is running."
                 : exitStatus == QProcess::CrashExit ? "The writer stopped unexpectedly. The USB device may be incomplete."
                 : "Writing did not finish. See the activity log for details.");
        }
        m_cancelling = false;
        emit stateChanged();
        refreshDevices();
    });
}

QString Backend::imageName() const { return QFileInfo(m_imagePath).fileName(); }
QString Backend::imageSizeText() const { return Disks::formatBytes(m_imageSize); }
QString Backend::status() const { return translatedMessage(m_status); }
QString Backend::scanError() const { return translatedMessage(m_scanError); }
QString Backend::detail() const
{
    if (m_cancelling && m_busy && !m_terminalEvent)
        return translatedMessage("Waiting for the current disk operation to finish. Keep the USB device connected.");
    if (m_stage != "writing" && m_stage != "verifying") return translatedMessage(m_detail);
    QString result = tr("%1 of %2").arg(Disks::formatBytes(m_done), Disks::formatBytes(m_total));
    if (m_remaining >= 0) {
        const QString remaining = m_remaining >= 60 ? tr("about %1 min left").arg((m_remaining + 59) / 60)
                                                    : tr("about %1 s left").arg(m_remaining);
        result += "  ·  " + Disks::formatBytes(qint64(m_speed)) + "/s  ·  " + remaining;
    }
    return result;
}
QString Backend::log() const
{
    QString result;
    for (const auto &entry : m_logs) result += entry.first + "  " + translatedMessage(entry.second) + '\n';
    return result;
}
void Backend::retranslate() { emit stateChanged(); emit devicesChanged(); emit logChanged(); }

void Backend::refreshDevices()
{
    if (m_busy) return;
    QString error;
    const auto found = Disks::scan(&error);
    if (found != m_devices || error != m_scanError) {
        m_devices = found;
        m_scanError = error;
        emit devicesChanged();
    }
}

void Backend::appendLog(const QString &text)
{
    m_logs.append({QDateTime::currentDateTime().toString("HH:mm:ss"), text});
    emit logChanged();
}

void Backend::fail(const QString &text)
{
    m_stage = "error";
    m_status = "Unable to continue";
    m_detail = text;
    appendLog(text);
    emit stateChanged();
}

void Backend::selectImage(const QUrl &url)
{
    if (m_busy) return;
    const auto path = QFileInfo(url.toLocalFile()).canonicalFilePath();
    QFile file(path);
    const auto suffix = QFileInfo(path).suffix().toLower();
    if (!url.isLocalFile() || (suffix != "iso" && suffix != "img") || !file.open(QIODevice::ReadOnly)
        || !QFileInfo(path).isFile() || file.size() <= 0) {
        fail("Choose a readable, uncompressed .iso or .img file that is not empty.");
        return;
    }
    m_imagePath = path;
    m_imageSize = file.size();
    m_identity = Disks::imageIdentity(path);
    m_progress = 0;
    m_stage = "idle";
    m_status = "Image ready";
    m_detail = "Choose the USB device you want to use.";
    appendLog("Selected image: " + path + " (" + imageSizeText() + ")");
    emit imageChanged();
    emit stateChanged();
}

void Backend::start(const QString &devicePath, const QString &expectedIdentity, bool verify)
{
    if (m_busy) return;
    refreshDevices();
    QVariantMap selected;
    for (const auto &value : m_devices)
        if (value.toMap()["path"].toString() == devicePath) selected = value.toMap();
    if (selected.isEmpty()) { fail("The USB device is no longer available. Select it again."); return; }
    if (selected["identity"].toString() != expectedIdentity || selected["sequence"].toString().isEmpty()) {
        fail("The USB device was replaced or could not be safely identified. Select and confirm the target again."); return;
    }
    if (m_identity.isEmpty() || Disks::imageIdentity(m_imagePath) != m_identity) {
        fail("The image has changed or is unavailable. Select the file again."); return;
    }
    if (m_imageSize > selected["size"].toLongLong()) { fail("The image is larger than the USB device."); return; }
    if (selected["mounted"].toBool()) {
        fail("Unmount the USB partitions in your file manager before starting. Keep the device connected."); return;
    }
    m_verify = verify;
    m_cancelling = false;
    m_terminalEvent = false;
    m_pending.clear();
    m_busy = true;
    m_progress = 0;
    m_stage = "authorizing";
    m_status = "Waiting for approval";
    m_detail = "Approve administrator access in the system dialog to start writing.";
    appendLog("Starting: " + imageName() + " → " + selected["label"].toString());
    emit stateChanged();
    m_writer.start("/usr/bin/pkexec", {QCoreApplication::applicationFilePath(), "--write-image", m_imagePath,
        devicePath, selected["size"].toString(), selected["serial"].toString(), m_identity,
        verify ? "verify" : "no-verify", selected["sequence"].toString()});
}

void Backend::cancel()
{
    if (!m_busy) return;
    m_cancelling = true;
    m_writer.write("cancel\n");
    if (m_stage == "authorizing") m_writer.terminate();
    m_status = "Cancelling …";
    m_detail = "Waiting for the current disk operation to finish. Keep the USB device connected.";
    appendLog("Cancellation requested.");
    emit stateChanged();
}

void Backend::readEvents()
{
    m_pending += m_writer.readAllStandardOutput();
    while (m_pending.contains('\n')) {
        const auto newline = m_pending.indexOf('\n');
        const auto line = m_pending.left(newline);
        m_pending.remove(0, newline + 1);
        const auto event = QJsonDocument::fromJson(line).object();
        if (event.isEmpty()) continue;
        const auto stage = event["stage"].toString();
        const bool changed = stage != m_stage;
        if (changed) m_stageTimer.start();
        m_stage = stage;
        const qint64 done = event["done"].toInteger();
        const qint64 total = event["total"].toInteger();
        const double fraction = total > 0 ? double(done) / double(total) : 0;
        if (stage == "writing" || stage == "verifying") {
            m_progress = fraction;
            m_status = stage == "writing" ? "Copying to USB …" : "Verifying contents …";
            const double seconds = qMax(0.1, m_stageTimer.elapsed() / 1000.0);
            const double speed = done / seconds;
            m_done = done; m_total = total; m_speed = speed;
            m_remaining = seconds > 1 && done > 0 ? int(std::ceil((total - done) / speed)) : -1;
        } else if (stage == "complete" || stage == "error" || stage == "cancelled") {
            m_terminalEvent = true;
            m_status = stage == "complete" ? "Your USB device is ready!" : stage == "cancelled" ? "Operation cancelled" : "Writing failed";
            m_detail = event["message"].toString();
            if (stage == "complete") m_progress = 1;
        }
        if (changed) {
            appendLog(m_status);
            if (stage != "writing" && stage != "verifying") appendLog(m_detail);
        }
        if (m_cancelling && !m_terminalEvent) m_status = "Cancelling …";
        emit stateChanged();
    }
}
