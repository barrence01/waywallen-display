#pragma once

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QString>

struct KdeScreenIdentity {
    enum class Source
    {
        None,
        Serial,
        Edid,
        Connector,
    };

    QString id;
    Source  source { Source::None };

    QString sourceName() const {
        switch (source) {
        case Source::Serial: return QStringLiteral("serial");
        case Source::Edid: return QStringLiteral("edid");
        case Source::Connector: return QStringLiteral("connector");
        case Source::None: return {};
        }
        return {};
    }
};

inline bool kdeEdidLooksValid(const QByteArray& bytes) {
    if (bytes.size() < 128) return false;
    for (char byte : bytes) {
        if (byte != 0) return true;
    }
    return false;
}

inline QByteArray kdeConnectedEdid(const QString& connector,
                                   const QString& drmRoot = QStringLiteral("/sys/class/drm")) {
    const auto name = connector.trimmed();
    if (name.isEmpty()) return {};

    QDir drm(drmRoot);
    if (! drm.exists()) return {};

    const QString suffix = QLatin1Char('-') + name;
    QByteArray    best;
    const auto    entries = drm.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& entry : entries) {
        if (entry != name && ! entry.endsWith(suffix)) continue;

        const QString base = drm.filePath(entry);
        QFile         statusFile(base + QStringLiteral("/status"));
        if (statusFile.open(QIODevice::ReadOnly)) {
            const auto status = QString::fromUtf8(statusFile.readAll()).trimmed();
            if (status != QLatin1String("connected")) continue;
        }

        QFile edidFile(base + QStringLiteral("/edid"));
        if (! edidFile.open(QIODevice::ReadOnly)) continue;
        const auto bytes = edidFile.readAll();
        if (! kdeEdidLooksValid(bytes)) continue;
        if (bytes.size() > best.size()) best = bytes;
    }
    return best;
}

inline KdeScreenIdentity makeKdeScreenIdentity(const QString& manufacturer, const QString& model,
                                               const QString& serial, const QString& connector,
                                               const QByteArray& edid) {
    const auto mfg  = manufacturer.trimmed();
    const auto mdl  = model.trimmed();
    const auto ser  = serial.trimmed();
    const auto conn = connector.trimmed();

    QByteArray        payload;
    KdeScreenIdentity identity;
    if (kdeEdidLooksValid(edid)) {
        payload         = edid;
        identity.source = KdeScreenIdentity::Source::Edid;
    } else if (! ser.isEmpty()) {
        payload = QStringLiteral("manufacturer=%1|model=%2|serial=%3").arg(mfg, mdl, ser).toUtf8();
        identity.source = KdeScreenIdentity::Source::Serial;
    } else if (! conn.isEmpty()) {
        payload         = QStringLiteral("connector=%1").arg(conn).toUtf8();
        identity.source = KdeScreenIdentity::Source::Connector;
    } else {
        return identity;
    }

    const auto md5 = QCryptographicHash::hash(payload, QCryptographicHash::Md5).toHex();
    identity.id    = QStringLiteral("kde-") + QString::fromLatin1(md5);
    return identity;
}
