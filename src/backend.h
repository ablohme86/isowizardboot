#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QString imageName READ imageName NOTIFY imageChanged)
    Q_PROPERTY(QString imagePath READ imagePath NOTIFY imageChanged)
    Q_PROPERTY(QString imageSizeText READ imageSizeText NOTIFY imageChanged)
    Q_PROPERTY(qint64 imageSize READ imageSize NOTIFY imageChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(double progress READ progress NOTIFY stateChanged)
    Q_PROPERTY(QString stage READ stage NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY stateChanged)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)
    Q_PROPERTY(QString scanError READ scanError NOTIFY devicesChanged)
public:
    explicit Backend(QObject *parent = nullptr);
    QVariantList devices() const { return m_devices; }
    QString imageName() const;
    QString imagePath() const { return m_imagePath; }
    QString imageSizeText() const;
    qint64 imageSize() const { return m_imageSize; }
    bool busy() const { return m_busy; }
    double progress() const { return m_progress; }
    QString stage() const { return m_stage; }
    QString status() const;
    QString detail() const;
    QString log() const;
    QString scanError() const;
    void retranslate();
    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void selectImage(const QUrl &url);
    Q_INVOKABLE void start(const QString &devicePath, const QString &expectedIdentity, bool verify);
    Q_INVOKABLE void cancel();
signals:
    void devicesChanged();
    void imageChanged();
    void stateChanged();
    void logChanged();
private:
    void appendLog(const QString &text);
    void fail(const QString &text);
    void readEvents();
    QProcess m_writer;
    QTimer m_refresh;
    QVariantList m_devices;
    QString m_imagePath, m_identity, m_scanError;
    QList<QPair<QString, QString>> m_logs;
    qint64 m_imageSize = 0;
    bool m_busy = false, m_verify = true, m_terminalEvent = false;
    bool m_cancelling = false;
    double m_progress = 0;
    QString m_stage = "idle", m_status = "Ready when you are", m_detail = "Choose an image and a USB device to get started.";
    QByteArray m_pending;
    qint64 m_stageStarted = 0;
    qint64 m_done = 0, m_total = 0;
    double m_speed = 0;
    int m_remaining = -1;
};
