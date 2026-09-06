#pragma once

#include <QJsonObject>
#include <QObject>
#include <QTranslator>

class JsonTranslator : public QTranslator
{
public:
    explicit JsonTranslator(QObject *parent = nullptr);
    QString translate(const char *, const char *source, const char *, int) const override;
    bool isEmpty() const override { return m_translations.isEmpty(); }
private:
    QJsonObject m_translations;
};

class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
public:
    explicit Settings(QObject *parent = nullptr);
    ~Settings() override;
    QString language() const { return m_language; }
    QString theme() const { return m_theme; }
    void setLanguage(const QString &language);
    void setTheme(const QString &theme);
signals:
    void languageChanged();
    void themeChanged();
private:
    JsonTranslator m_translator;
    QString m_language = "en", m_theme = "dark";
};

QString translatedMessage(const QString &source);
