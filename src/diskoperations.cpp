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
#include <grp.h>
#include <linux/fs.h>
#include <poll.h>
#include <pwd.h>
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
        *error = "Could not read the device list from lsblk.";
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
        *error = "Could not discover USB devices. Check that lsblk is installed.";
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
    if (size <= 0) { *error = "The image is empty."; return false; }
    QByteArray buffer(4 * 1024 * 1024, Qt::Uninitialized);
    QCryptographicHash sourceHash(QCryptographicHash::Sha256);
    QElapsedTimer timer;
    timer.start();
    qint64 done = 0;
    progress("writing", 0, size);
    while (done < size) {
        if (cancelled()) { *error = "Cancelled. The USB device contains an incomplete image and must be written again."; return false; }
        const ssize_t count = ::read(source, buffer.data(), qMin<qint64>(buffer.size(), size - done));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { *error = count == 0 ? "The image was truncated during writing." : systemError(); return false; }
        if (verify) sourceHash.addData(QByteArrayView(buffer.constData(), count));
        ssize_t written = 0;
        while (written < count) {
            if (cancelled()) { *error = "Cancelled. The USB device must be written again."; return false; }
            const ssize_t result = ::write(target, buffer.constData() + written, count - written);
            if (result < 0 && errno == EINTR) continue;
            if (result <= 0) { *error = "Write error: " + systemError(); return false; }
            written += result;
        }
        // Count only data flushed to the device, not bytes queued in Linux's
        // page cache. Bounded batches keep progress and cancellation responsive.
        int syncResult;
        do { syncResult = ::fsync(target); } while (syncResult < 0 && errno == EINTR);
        if (syncResult < 0) { *error = "Could not finish writing to the device: " + systemError(); return false; }
        done += count;
        if (timer.elapsed() >= 100 || done == size) { progress("writing", done, size); timer.restart(); }
    }
    if (cancelled()) { *error = "Cancelled before verification finished."; return false; }
    if (!verify) return true;
    // Flush and invalidate the block-device cache before reading the media again.
    struct stat targetInfo{};
    if (::fstat(target, &targetInfo)) { *error = systemError(); return false; }
    if (S_ISBLK(targetInfo.st_mode) && ::ioctl(target, BLKFLSBUF)) {
        *error = "Could not clear the disk cache before verification: " + systemError(); return false;
    }
    if (::lseek(target, 0, SEEK_SET) < 0) { *error = systemError(); return false; }
    QCryptographicHash targetHash(QCryptographicHash::Sha256);
    done = 0;
    progress("verifying", 0, size);
    while (done < size) {
        if (cancelled()) { *error = "Verification was cancelled. The USB device has not been verified."; return false; }
        const ssize_t count = ::read(target, buffer.data(), qMin<qint64>(buffer.size(), size - done));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { *error = "Could not read back the USB device: " + systemError(); return false; }
        targetHash.addData(QByteArrayView(buffer.constData(), count));
        done += count;
        if (timer.elapsed() >= 100 || done == size) { progress("verifying", done, size); timer.restart(); }
    }
    if (sourceHash.result() != targetHash.result()) { *error = "Verification failed: the USB contents do not match the image."; return false; }
    return true;
}

int runWriter(const QStringList &arguments)
{
    auto fail = [](const QString &message) { emitEvent("error", 0, 0, message); return 1; };
    if (arguments.size() != 9 || ::geteuid() != 0) return fail("Writing requires administrator approval through pkexec.");
    const auto imagePath = arguments[2];
    const auto devicePath = arguments[3];
    const auto expectedSize = arguments[4].toLongLong();
    const auto expectedSerial = arguments[5];
    const auto expectedImage = arguments[6];
    const bool verify = arguments[7] == "verify";
    const QString expectedSequence = arguments[8];
    if (expectedImage.isEmpty() || !imagePath.startsWith('/') || !devicePath.startsWith("/dev/"))
        return fail("Invalid image or device.");
    QString error;
    const auto devices = scan(&error);
    if (!error.isEmpty()) return fail(error);
    QVariantMap selected;
    for (const auto &value : devices)
        if (value.toMap()["path"].toString() == devicePath) selected = value.toMap();
    if (selected.isEmpty() || selected["size"].toLongLong() != expectedSize || selected["serial"].toString() != expectedSerial
        || expectedSequence.isEmpty() || selected["sequence"].toString() != expectedSequence)
        return fail("The USB device has changed or is unsafe to write. Refresh the device list.");
    if (selected["mounted"].toBool()) return fail("The USB device is mounted. Unmount all its partitions in your file manager and try again.");
    // Open the source with the invoking user's permissions, not root's permissions.
    bool uidOk = false;
    const uint callerUid = qEnvironmentVariable("PKEXEC_UID").toUInt(&uidOk);
    const auto *caller = uidOk ? ::getpwuid(callerUid) : nullptr;
    const gid_t privilegedGid = ::getegid();
    if (!caller || ::initgroups(caller->pw_name, caller->pw_gid) || ::setegid(caller->pw_gid) || ::seteuid(callerUid))
        return fail("Could not check access to the image.");
    const int source = ::open(QFile::encodeName(imagePath).constData(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    const int sourceErrno = errno;
    if (::seteuid(0) || ::setegid(privilegedGid) || ::setgroups(0, nullptr)) {
        if (source >= 0) ::close(source);
        return fail("Could not restore write permissions.");
    }
    if (source < 0) { errno = sourceErrno; return fail("Could not open the image: " + systemError()); }
    struct stat sourceInfo{};
    if (::fstat(source, &sourceInfo) || !S_ISREG(sourceInfo.st_mode) || identity(sourceInfo) != expectedImage
        || sourceInfo.st_size <= 0 || sourceInfo.st_size > expectedSize) {
        ::close(source); return fail("The image has changed, is empty, or is larger than the USB device.");
    }
    // O_EXCL refuses mounted block devices, including mounted partitions.
    const int target = ::open(QFile::encodeName(devicePath).constData(), O_RDWR | O_EXCL | O_CLOEXEC | O_NOFOLLOW);
    if (target < 0) { ::close(source); return fail("Could not lock the USB device for writing: " + systemError()); }
    struct stat targetInfo{};
    quint64 actualSize = 0;
    quint64 actualSequence = 0;
    if (::fstat(target, &targetInfo) || !S_ISBLK(targetInfo.st_mode) || ::ioctl(target, BLKGETSIZE64, &actualSize)
        || actualSize != quint64(expectedSize) || ::ioctl(target, BLKGETDISKSEQ, &actualSequence)
        || actualSequence != expectedSequence.toULongLong() || sourceInfo.st_dev == targetInfo.st_rdev) {
        ::close(source); ::close(target); return fail("The target is not the expected USB block device.");
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
    if (!unchanged) return fail("The image changed during writing. Write the USB device again from an unchanged file.");
    emitEvent("complete", 0, 0, verify ? "The USB device has been written and verified. You can now eject it." : "The USB device has been written. Verification was disabled. You can now eject it.");
    return 0;
}
}
