// common/Utils/DateTimeUtils.h
#pragma once

#include <QDateTime>
#include <QString>

namespace bookclub::common {

class DateTimeUtils {
public:
    // ---- Current Time ----
    static QDateTime nowUtc();
    static QDateTime nowLocal();
    static QString nowIsoString();

    // ---- Formatting ----
    static QString toIsoString(const QDateTime& dt);
    static QString toDisplayString(const QDateTime& dt);
    static QString toDateString(const QDateTime& dt);
    static QString toTimeString(const QDateTime& dt);

    // ---- Parsing ----
    static QDateTime fromIsoString(const QString& isoString);
    static QDateTime fromString(const QString& str, const QString& format = "yyyy-MM-ddTHH:mm:ss");

    // ---- Calculations ----
    static qint64 secondsBetween(const QDateTime& from, const QDateTime& to);
    static qint64 daysBetween(const QDateTime& from, const QDateTime& to);
    static QString formatDuration(qint64 seconds, bool shortFormat = false);

    // ---- Checks ----
    static bool isExpired(const QDateTime& expiresAt);
    static bool isInPast(const QDateTime& dt);
    static bool isInFuture(const QDateTime& dt);
    static bool isValidDateTime(const QDateTime& dt);

    // ---- Add/Subtract ----
    static QDateTime addSeconds(const QDateTime& dt, qint64 seconds);
    static QDateTime addDays(const QDateTime& dt, int days);
    static QDateTime addMonths(const QDateTime& dt, int months);
};

} // namespace bookclub::common
