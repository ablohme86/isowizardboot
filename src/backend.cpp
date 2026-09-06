#include "backend.h"
#include "diskoperations.h"
#include "settings.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

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
            fail("Kunne ikke starte pkexec. Installer polkit for å skrive USB-enheter.");
        }
    });
    connect(&m_writer, &QProcess::finished, this, [this](int code, QProcess::ExitStatus exitStatus) {
        readEvents();
        m_busy = false;
        if (!m_terminalEvent) {
            fail(code == 126 ? "Administratorgodkjenning ble avbrutt. Ingenting ble skrevet."
                 : code == 127 ? "Kunne ikke få administratorgodkjenning. Kontroller at en polkit-agent kjører."
                 : exitStatus == QProcess::CrashExit ? "Skriveprosessen stoppet uventet. USB-enheten kan være ufullstendig."
                 : "Skrivingen ble ikke fullført. Se aktivitetsloggen for detaljer.");
        }
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
    m_status = "Kunne ikke fortsette";
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
        fail("Velg en lesbar, ukomprimert .iso- eller .img-fil som ikke er tom.");
        return;
    }
    m_imagePath = path;
    m_imageSize = file.size();
    m_identity = Disks::imageIdentity(path);
    m_progress = 0;
    m_stage = "idle";
    m_status = "Bildefilen er klar";
    m_detail = "Velg USB-enheten som skal brukes.";
    appendLog("Valgt bildefil: " + path + " (" + imageSizeText() + ")");
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
    if (selected.isEmpty()) { fail("USB-enheten er ikke lenger tilgjengelig. Velg enheten på nytt."); return; }
    if (selected["identity"].toString() != expectedIdentity || selected["sequence"].toString().isEmpty()) {
        fail("USB-enheten er byttet ut eller kunne ikke identifiseres sikkert. Velg og bekreft målet på nytt."); return;
    }
    if (m_identity.isEmpty() || Disks::imageIdentity(m_imagePath) != m_identity) {
        fail("Bildefilen er endret eller ikke tilgjengelig. Velg filen på nytt."); return;
    }
    if (m_imageSize > selected["size"].toLongLong()) { fail("Bildefilen er større enn USB-enheten."); return; }
    if (selected["mounted"].toBool()) {
        fail("Avmonter USB-enhetens partisjoner i filbehandleren før du starter. Ikke koble fra selve enheten."); return;
    }
    m_verify = verify;
    m_terminalEvent = false;
    m_pending.clear();
    m_busy = true;
    m_progress = 0;
    m_stage = "authorizing";
    m_status = "Venter på godkjenning";
    m_detail = "Godkjenn administratortilgang i systemdialogen for å starte skrivingen.";
    appendLog("Starter: " + imageName() + " → " + selected["label"].toString());
    emit stateChanged();
    m_writer.start("/usr/bin/pkexec", {QCoreApplication::applicationFilePath(), "--write-image", m_imagePath,
        devicePath, selected["size"].toString(), selected["serial"].toString(), m_identity,
        verify ? "verify" : "no-verify", selected["sequence"].toString()});
}

void Backend::cancel()
{
    if (!m_busy) return;
    m_writer.write("cancel\n");
    if (m_stage == "authorizing") m_writer.terminate();
    m_status = "Avbryter …";
    m_detail = "Venter på at enheten skal fullføre pågående diskoperasjon. Ikke trekk ut USB-enheten ennå.";
    appendLog("Avbrudd forespurt.");
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
        if (changed) m_stageStarted = QDateTime::currentMSecsSinceEpoch();
        m_stage = stage;
        const qint64 done = event["done"].toInteger();
        const qint64 total = event["total"].toInteger();
        const double fraction = total > 0 ? double(done) / double(total) : 0;
        if (stage == "writing" || stage == "verifying") {
            m_progress = stage == "writing" ? fraction * (m_verify ? 0.8 : 0.98) : 0.8 + fraction * 0.19;
            m_status = stage == "writing" ? "Skriver til USB …" : "Verifiserer innhold …";
            const double seconds = qMax(0.1, (QDateTime::currentMSecsSinceEpoch() - m_stageStarted) / 1000.0);
            const double speed = done / seconds;
            m_done = done; m_total = total; m_speed = speed;
            m_remaining = seconds > 1 && done > 0 ? int((total - done) / speed) : -1;
        } else if (stage == "syncing") {
            m_status = "Fullfører skriving …";
            m_detail = "Tømmer diskbufferen. Dette kan ta litt tid; la USB-enheten stå tilkoblet.";
        } else if (stage == "complete" || stage == "error" || stage == "cancelled") {
            m_terminalEvent = true;
            m_status = stage == "complete" ? "USB-enheten er klar!" : stage == "cancelled" ? "Operasjonen ble avbrutt" : "Skrivingen feilet";
            m_detail = event["message"].toString();
            if (stage == "complete") m_progress = 1;
        }
        if (changed) {
            appendLog(m_status);
            if (stage != "writing" && stage != "verifying") appendLog(m_detail);
        }
        emit stateChanged();
    }
}
