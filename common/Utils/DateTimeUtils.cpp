// common/Utils/DateTimeUtils.cpp
#include "common/Utils/DateTimeUtils.h"
#include <QTimeZone>

namespace bookclub::common {

QDateTime DateTimeUtils::nowUtc() {
    return QDateTime::currentDateTimeUtc();
}

QDateTime DateTimeUtils::nowLocal() {
    return QDateTime::currentDateTime();
}

QString DateTimeUtils::nowIsoString() {
    return toIsoString(nowUtc());
}

QString DateTimeUtils::toIsoString(const QDateTime& dt) {
    return dt.toString(Qt::ISODateWithMs);
}

QString DateTimeUtils::toDisplayString(const QDateTime& dt) {
    return dt.toString("yyyy-MM-dd HH:mm:ss");
}

QString DateTimeUtils::toDateString(const QDateTime& dt) {
    return dt.toString("yyyy-MM-dd");
}

QString DateTimeUtils::toTimeString(const QDateTime& dt) {
    return dt.toString("HH:mm:ss");
}

QDateTime DateTimeUtils::fromIsoString(const QString& isoString) {
    return QDateTime::fromString(isoString, Qt::ISODateWithMs);
}

QDateTime DateTimeUtils::fromString(const QString& str, const QString& format) {
    return QDateTime::fromString(str, format);
}

qint64 DateTimeUtils::secondsBetween(const QDateTime& from, const QDateTime& to) {
    return from.secsTo(to);
}

qint64 DateTimeUtils::daysBetween(const QDateTime& from, const QDateTime& to) {
    return from.daysTo(to);
}

QString DateTimeUtils::formatDuration(qint64 seconds, bool shortFormat) {
    if (seconds < 0) seconds = -seconds;
    qint64 days = seconds / 86400;
    qint64 hours = (seconds % 86400) / 3600;
    qint64 minutes = (seconds % 3600) / 60;
    qint64 secs = seconds % 60;

    QStringList parts;
    if (days > 0) {
        parts << QString::number(days) + (shortFormat ? "d" : " روز");
    }
    if (hours > 0 || days > 0) {
        parts << QString::number(hours) + (shortFormat ? "h" : " ساعت");
    }
    if (minutes > 0 || hours > 0 || days > 0) {
        parts << QString::number(minutes) + (shortFormat ? "m" : " دقیقه");
    }
    if (parts.isEmpty() || secs > 0) {
        parts << QString::number(secs) + (shortFormat ? "s" : " ثانیه");
    }
    return parts.join(shortFormat ? " " : " و ");
}

bool DateTimeUtils::isExpired(const QDateTime& expiresAt) {
    return expiresAt.isValid() && nowUtc() > expiresAt;
}

bool DateTimeUtils::isInPast(const QDateTime& dt) {
    return dt.isValid() && dt < nowUtc();
}

bool DateTimeUtils::isInFuture(const QDateTime& dt) {
    return dt.isValid() && dt > nowUtc();
}

bool DateTimeUtils::isValidDateTime(const QDateTime& dt) {
    return dt.isValid();
}

QDateTime DateTimeUtils::addSeconds(const QDateTime& dt, qint64 seconds) {
    return dt.addSecs(seconds);
}

QDateTime DateTimeUtils::addDays(const QDateTime& dt, int days) {
    return dt.addDays(days);
}

QDateTime DateTimeUtils::addMonths(const QDateTime& dt, int months) {
    return dt.addMonths(months);
}

} // namespace bookclub::common
