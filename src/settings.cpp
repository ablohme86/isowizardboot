#include "settings.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QLocale>
#include <QSettings>

JsonTranslator::JsonTranslator(QObject *parent) : QTranslator(parent)
{
    QFile file(":/i18n/no-NB.i18n");
    if (file.open(QIODevice::ReadOnly))
        m_translations = QJsonDocument::fromJson(file.readAll()).object()["translations"].toObject();
}

QString JsonTranslator::translate(const char *, const char *source, const char *, int) const
{
    return m_translations.value(QString::fromUtf8(source)).toString();
}

Settings::Settings(QObject *parent) : QObject(parent)
{
    const QSettings saved;
    const QString language = saved.value("appearance/language", "en").toString();
    m_theme = saved.value("appearance/theme", "dark").toString() == "light" ? "light" : "dark";
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    if (language == "no-NB") setLanguage(language);
}

Settings::~Settings() { QCoreApplication::removeTranslator(&m_translator); }

void Settings::setLanguage(const QString &language)
{
    if ((language != "en" && language != "no-NB") || language == m_language) return;
    m_language = language;
    if (language == "no-NB") QCoreApplication::installTranslator(&m_translator);
    else QCoreApplication::removeTranslator(&m_translator);
    QLocale::setDefault(language == "no-NB" ? QLocale(QLocale::NorwegianBokmal, QLocale::Norway)
                                          : QLocale(QLocale::English, QLocale::UnitedStates));
    QSettings().setValue("appearance/language", language);
    emit languageChanged();
}

void Settings::setTheme(const QString &theme)
{
    if ((theme != "dark" && theme != "light") || theme == m_theme) return;
    m_theme = theme;
    QSettings().setValue("appearance/theme", theme);
    emit themeChanged();
}

QString translatedMessage(const QString &source)
{
    const auto translated = QCoreApplication::translate("App", source.toUtf8().constData());
    if (translated != source) return translated;
    // Keep paths and operating-system diagnostics intact while translating the label.
    const auto colon = source.indexOf(": ");
    if (colon >= 0) {
        const QString prefix = source.left(colon + 2);
        return QCoreApplication::translate("App", prefix.toUtf8().constData()) + source.mid(colon + 2);
    }
    return source;
}
