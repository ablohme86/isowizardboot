#include "diskoperations.h"

#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QTextStream>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/fs.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
QString systemError() { return QString::fromLocal8Bit(strerror(errno)); }

bool systemMount(const QJsonObject &node)
{
    for (const auto &value : node["mountpoints"].toArray()) {
        const auto mount = value.toString();
        if (!mount.isEmpty() && !mount.startsWith("/run/media/") && !mount.startsWith("/media/") && !mount.startsWith("/mnt/"))
            return true;
    }
    for (const auto &child : node["children"].toArray())
        if (systemMount(child.toObject())) return true;
    return false;
}

bool hasHolders(const QJsonObject &node)
{
    const QString name = node["name"].toString();
    if (!QDir("/sys/class/block/" + name + "/holders").entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty())
        return true;
    for (const auto &child : node["children"].toArray())
        if (hasHolders(child.toObject())) return true;
    return false;
}

QString identity(const struct stat &info)
{
    return QString("%1:%2:%3:%4:%5").arg(qulonglong(info.st_dev)).arg(qulonglong(info.st_ino))
        .arg(info.st_size).arg(info.st_mtim.tv_sec).arg(info.st_mtim.tv_nsec);
}

void emitEvent(const QString &stage, qint64 done = 0, qint64 total = 0, const QString &message = {})
{
    const QJsonObject event{{"stage", stage}, {"done", done}, {"total", total}, {"message", message}};
    QTextStream out(stdout);
    out << QJsonDocument(event).toJson(QJsonDocument::Compact) << '\n';
    out.flush();
}
}

namespace Disks {
QString formatBytes(qint64 bytes)
{
    double value = bytes;
    const QStringList units{"B", "KiB", "MiB", "GiB", "TiB"};
    int unit = 0;
    while (value >= 1024 && unit < units.size() - 1) { value /= 1024; ++unit; }
    return QString::number(value, 'f', unit ? 1 : 0) + " " + units[unit];
}

bool eligible(const QJsonObject &disk)
{
    return disk["type"].toString() == "disk" && !disk["ro"].toBool()
        && (disk["tran"].toString() == "usb" || disk["rm"].toBool())
        && disk["size"].toInteger() > 0 && !systemMount(disk);
}

bool mounted(const QJsonObject &disk)
{
    for (const auto &value : disk["mountpoints"].toArray())
        if (!value.toString().isEmpty()) return true;
    for (const auto &child : disk["children"].toArray())
        if (mounted(child.toObject())) return true;
    return false;
}

QVariantList parseDevices(const QByteArray &json, QString *error)
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.object()["blockdevices"].isArray()) {
        *error = "Kunne ikke lese enhetslisten fra lsblk.";
        return {};
    }
    QVariantList devices;
    for (const auto &value : document.object()["blockdevices"].toArray()) {
        const auto disk = value.toObject();
        if (!eligible(disk) || hasHolders(disk)) continue;
        const auto path = disk["path"].toString();
        const auto model = disk["model"].toString().trimmed();
        const auto size = disk["size"].toInteger();
        const QString name = model.isEmpty() ? disk["name"].toString() : model;
        QFile sequenceFile("/sys/class/block/" + disk["name"].toString() + "/diskseq");
        const QString sequence = sequenceFile.open(QIODevice::ReadOnly) ? QString::fromLatin1(sequenceFile.readAll()).trimmed() : QString();
        // A kernel disk sequence distinguishes even identical USB devices reusing /dev/sdX.
        const QString deviceIdentity = QString::number(size) + ":" + disk["serial"].toString() + ":" + sequence;
        devices.append(QVariantMap{{"path", path}, {"name", name}, {"size", size},
            {"sizeText", formatBytes(size)}, {"serial", disk["serial"].toString()},
            {"identity", deviceIdentity}, {"sequence", sequence},
            {"mounted", mounted(disk)}, {"removable", disk["rm"].toBool()},
            {"label", name + "  ·  " + formatBytes(size) + "  ·  " + path}});
    }
    return devices;
}

QVariantList scan(QString *error)
{
    QProcess process;
    process.start("/usr/bin/lsblk", {"--json", "--bytes", "--output", "NAME,PATH,TYPE,SIZE,MODEL,SERIAL,TRAN,RM,RO,MOUNTPOINTS"});
    if (!process.waitForFinished(5000) || process.exitCode() != 0) {
        *error = "Kunne ikke hente USB-enheter. Kontroller at lsblk er installert.";
        return {};
    }
    return parseDevices(process.readAllStandardOutput(), error);
}

QString imageIdentity(const QString &path)
{
    struct stat info{};
    if (::stat(QFile::encodeName(path).constData(), &info) || !S_ISREG(info.st_mode)) return {};
    return identity(info);
}

bool transfer(int source, int target, qint64 size, bool verify,
              const std::function<void(const QString &, qint64, qint64)> &progress,
              const std::function<bool()> &cancelled, QString *error)
{
    if (size <= 0) { *error = "Bildefilen er tom."; return false; }
    QByteArray buffer(4 * 1024 * 1024, Qt::Uninitialized);
    QCryptographicHash sourceHash(QCryptographicHash::Sha256);
    QElapsedTimer timer;
    timer.start();
    qint64 done = 0;
    progress("writing", 0, size);
    while (done < size) {
        if (cancelled()) { *error = "Avbrutt. USB-enheten inneholder et ufullstendig bilde og må skrives på nytt."; return false; }
        const ssize_t count = ::read(source, buffer.data(), qMin<qint64>(buffer.size(), size - done));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { *error = count == 0 ? "Bildefilen ble kortere under skriving." : systemError(); return false; }
        if (verify) sourceHash.addData(QByteArrayView(buffer.constData(), count));
        ssize_t written = 0;
        while (written < count) {
            if (cancelled()) { *error = "Avbrutt. USB-enheten må skrives på nytt."; return false; }
            const ssize_t result = ::write(target, buffer.constData() + written, count - written);
            if (result < 0 && errno == EINTR) continue;
            if (result <= 0) { *error = "Skrivefeil: " + systemError(); return false; }
            written += result;
        }
        done += count;
        if (timer.elapsed() >= 100 || done == size) { progress("writing", done, size); timer.restart(); }
    }
    progress("syncing", size, size);
    if (::fsync(target)) { *error = "Kunne ikke fullføre skriving til enheten: " + systemError(); return false; }
    if (cancelled()) { *error = "Avbrutt før fullført kontroll."; return false; }
    if (!verify) return true;
    // Flush and invalidate the block-device cache before reading the media again.
    struct stat targetInfo{};
    if (::fstat(target, &targetInfo)) { *error = systemError(); return false; }
    if (S_ISBLK(targetInfo.st_mode) && ::ioctl(target, BLKFLSBUF)) {
        *error = "Kunne ikke tømme diskbufferen før verifisering: " + systemError(); return false;
    }
    if (::lseek(target, 0, SEEK_SET) < 0) { *error = systemError(); return false; }
    QCryptographicHash targetHash(QCryptographicHash::Sha256);
    done = 0;
    progress("verifying", 0, size);
    while (done < size) {
        if (cancelled()) { *error = "Verifiseringen ble avbrutt. USB-enheten er ikke verifisert."; return false; }
        const ssize_t count = ::read(target, buffer.data(), qMin<qint64>(buffer.size(), size - done));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { *error = "Kunne ikke lese tilbake USB-enheten: " + systemError(); return false; }
        targetHash.addData(QByteArrayView(buffer.constData(), count));
        done += count;
        if (timer.elapsed() >= 100 || done == size) { progress("verifying", done, size); timer.restart(); }
    }
    if (sourceHash.result() != targetHash.result()) { *error = "Verifisering feilet: USB-innholdet samsvarer ikke med bildefilen."; return false; }
    return true;
}

int runWriter(const QStringList &arguments)
{
    auto fail = [](const QString &message) { emitEvent("error", 0, 0, message); return 1; };
    if (arguments.size() != 9 || ::geteuid() != 0) return fail("Skriving krever administratorgodkjenning via pkexec.");
    const auto imagePath = arguments[2];
    const auto devicePath = arguments[3];
    const auto expectedSize = arguments[4].toLongLong();
    const auto expectedSerial = arguments[5];
    const auto expectedImage = arguments[6];
    const bool verify = arguments[7] == "verify";
    const QString expectedSequence = arguments[8];
    if (expectedImage.isEmpty() || !imagePath.startsWith('/') || !devicePath.startsWith("/dev/"))
        return fail("Ugyldig bildefil eller enhet.");
    QString error;
    const auto devices = scan(&error);
    if (!error.isEmpty()) return fail(error);
    QVariantMap selected;
    for (const auto &value : devices)
        if (value.toMap()["path"].toString() == devicePath) selected = value.toMap();
    if (selected.isEmpty() || selected["size"].toLongLong() != expectedSize || selected["serial"].toString() != expectedSerial
        || expectedSequence.isEmpty() || selected["sequence"].toString() != expectedSequence)
        return fail("USB-enheten har blitt endret eller er ikke trygg å skrive til. Oppdater enhetslisten.");
    if (selected["mounted"].toBool()) return fail("USB-enheten er montert. Avmonter alle partisjonene i filbehandleren og prøv igjen.");
    // Open the source with the invoking user's permissions, not root's permissions.
    bool uidOk = false;
    const uint callerUid = qEnvironmentVariable("PKEXEC_UID").toUInt(&uidOk);
    if (!uidOk || ::seteuid(callerUid)) return fail("Kunne ikke kontrollere tilgang til bildefilen.");
    const int source = ::open(QFile::encodeName(imagePath).constData(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    const int sourceErrno = errno;
    if (::seteuid(0)) { if (source >= 0) ::close(source); return fail("Kunne ikke gjenopprette skriverettigheter."); }
    if (source < 0) { errno = sourceErrno; return fail("Kunne ikke åpne bildefilen: " + systemError()); }
    struct stat sourceInfo{};
    if (::fstat(source, &sourceInfo) || !S_ISREG(sourceInfo.st_mode) || identity(sourceInfo) != expectedImage
        || sourceInfo.st_size <= 0 || sourceInfo.st_size > expectedSize) {
        ::close(source); return fail("Bildefilen har blitt endret, er tom eller er større enn USB-enheten.");
    }
    // O_EXCL refuses mounted block devices, including mounted partitions.
    const int target = ::open(QFile::encodeName(devicePath).constData(), O_RDWR | O_EXCL | O_CLOEXEC | O_NOFOLLOW);
    if (target < 0) { ::close(source); return fail("Kunne ikke låse USB-enheten for skriving: " + systemError()); }
    struct stat targetInfo{};
    quint64 actualSize = 0;
    quint64 actualSequence = 0;
    if (::fstat(target, &targetInfo) || !S_ISBLK(targetInfo.st_mode) || ::ioctl(target, BLKGETSIZE64, &actualSize)
        || actualSize != quint64(expectedSize) || ::ioctl(target, BLKGETDISKSEQ, &actualSequence)
        || actualSequence != expectedSequence.toULongLong() || sourceInfo.st_dev == targetInfo.st_rdev) {
        ::close(source); ::close(target); return fail("Målet er ikke den forventede USB-blokkenheten.");
    }
    bool wasCancelled = false;
    auto cancelled = [&]() {
        pollfd input{STDIN_FILENO, POLLIN | POLLHUP, 0};
        if (::poll(&input, 1, 0) > 0 && (input.revents & (POLLIN | POLLHUP | POLLERR))) wasCancelled = true;
        return wasCancelled;
    };
    const bool result = transfer(source, target, sourceInfo.st_size, verify,
        [](const QString &stage, qint64 done, qint64 total) { emitEvent(stage, done, total); }, cancelled, &error);
    struct stat finalSourceInfo{};
    const bool unchanged = ::fstat(source, &finalSourceInfo) == 0 && identity(finalSourceInfo) == expectedImage;
    ::close(source);
    if (!result) ::fsync(target);
    ::close(target);
    if (!result) { emitEvent(wasCancelled ? "cancelled" : "error", 0, 0, error); return wasCancelled ? 2 : 1; }
    if (!unchanged) return fail("Bildefilen ble endret under skriving. Skriv USB-enheten på nytt fra en uendret fil.");
    emitEvent("complete", 0, 0, verify ? "USB-enheten er skrevet og verifisert. Du kan nå løse den ut." : "USB-enheten er skrevet. Verifisering var slått av. Du kan nå løse den ut.");
    return 0;
}
}
