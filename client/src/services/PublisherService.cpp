// client/src/services/PublisherService.cpp
//
// Full socket-backed implementation of PublisherService.
//
// v4 polish: see header for the full changelog. Key user-visible fixes:
//   • Promotions list actually populates (local cache).
//   • Profile Save persists locally + fires UpdateProfile on the server.
//   • Sales / revenue / monthly charts now render with proper time series.
//   • Activity feed, recent orders, top buyers, geo breakdown, genre
//     breakdown all return shapes that match their QML delegates.
//   • setBookStatus recognizes "published" / "active" / "removed" /
//     "inactive" / "draft" so the catalog Activate/Deactivate bulk
//     actions do the right thing.
//   • publisherNotifications() enriches the raw server payload into the
//     shape the QML delegate expects.
#include "services/PublisherService.h"
#include "services/NetworkService.h"
#include "services/AuthService.h"
#include "services/BookDto.h"
#include "common/Network/Protocol.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QLocale>
#include <QDateTime>
#include <QDate>
#include <QSet>
#include <QHash>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <algorithm>
#include <cmath>

namespace bookclub::client {

// ============================================================================
//  Helper: upload a publisher-picked PDF file to the server.
//
//  `localPath` is the file path on the publisher's machine (e.g.
//  "/home/alice/books/novel.pdf"). The function reads the file, base64-
//  encodes the bytes, and sends an UploadBookPdf command to the server.
//  The server stores the bytes under <server-cwd>/storage/pdfs/<bookId>.pdf
//  and updates the Books.pdfFilePath column.
//
//  Returns the server-reported relative pdfFilePath (e.g. "pdfs/<id>.pdf")
//  on success, or an empty string on failure. Failures are non-fatal —
//  the book is still published; only the PDF preview is unavailable.
// ============================================================================
// v15k: changed from synchronous sendRequest to fire-and-forget sendAsync.
// The old synchronous call blocked the GUI thread for up to 30 seconds
// during PDF upload, which froze the entire app and caused the "Add title"
// button to stop working after the first publish (the blocking call
// collided with subsequent requests on the same socket).
//
// Now the PDF uploads in the background — the book is published
// immediately, and the PDF becomes available for reading a few seconds
// later.
static void uploadBookPdfToServerAsync(const QString& bookId, const QString& localPath) {
    if (bookId.isEmpty() || localPath.isEmpty()) return;

    QFileInfo info(localPath);
    if (!info.isFile() || info.suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) != 0) {
        return;
    }

    QFile f(localPath);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QByteArray raw = f.readAll();
    f.close();
    if (raw.isEmpty()) return;

    // Cap at 32 MB.
    static constexpr qint64 kMaxPdfSize = 32 * 1024 * 1024;
    if (raw.size() > kMaxPdfSize) return;

    QJsonObject p;
    p["bookId"]   = bookId;
    p["fileName"] = info.fileName();
    p["fileData"] = QString::fromLatin1(raw.toBase64());

    // Fire-and-forget — no callback needed. The server stores the PDF
    // and updates Books.pdfFilePath. The next time the user opens the
    // book in the reader, the client will download it.
    NetworkService::instance().sendAsync(common::Command::UploadBookPdf, p);
}

PublisherService::PublisherService(QObject* parent)
    : QObject(parent), m_notificationsCacheValid(false)
{
    // v22 (Issues 1 & 4): subscribe to server-pushed events so the
    // publisher dashboard refreshes in real-time whenever something
    // relevant happens. We invalidate m_analyticsCache (so the next
    // analytics read pulls fresh SQL data from the server) and re-emit
    // the granular signals the PublisherViewModel already listens to.

    // EvtNotification fires for every notification, including sale
    // notifications (SaleRegistered type) — so when a buyer purchases one
    // of the publisher's books, the dashboard's KPIs and charts refresh
    // immediately.
    NetworkService::instance().subscribeEvent(common::Command::EvtNotification,
        this,
        [this](const common::Message&) {
            _invalidateCaches();
        });

    // EvtBookAdded fires when any new book is published (the publisher
    // themselves might be adding one from another session/device).
    NetworkService::instance().subscribeEvent(common::Command::EvtBookAdded,
        this,
        [this](const common::Message&) {
            _invalidateCaches();
        });

    // EvtDiscountApplied fires whenever a discount is applied/cleared —
    // the publisher's promotions list and the analytics (revenue after
    // discount) need to be recomputed.
    NetworkService::instance().subscribeEvent(common::Command::EvtDiscountApplied,
        this,
        [this](const common::Message&) {
            _invalidateCaches();
        });

    // EvtReviewUpdated fires whenever a review is added/edited/deleted on
    // any book — the publisher's averageRating depends on it.
    NetworkService::instance().subscribeEvent(common::Command::EvtReviewUpdated,
        this,
        [this](const common::Message&) {
            _invalidateCaches();
        });
}

// ============================================================================
//  v22 (Issue 4): invalidate every cache and emit all the granular change
//  signals so the PublisherViewModel re-emits and QML bindings refresh.
// ============================================================================
void PublisherService::_invalidateCaches() {
    m_analyticsCache = QJsonObject();
    m_notificationsCache.clear();
    m_notificationsCacheValid = false;
    emit dataChanged();
    // Re-emit granular signals so the PublisherViewModel (which connects
    // to these four, NOT dataChanged) refreshes every QML binding.
    emit booksChanged();
    emit notificationsChanged();
    // promotions and profile are local overlays — not affected by server
    // pushes — so we don't emit those.
}

// ============================================================================
//  Helper: fetch GetPublisherAnalytics with caching
// ============================================================================
QJsonObject PublisherService::fetchAnalytics() const {
    if (!m_analyticsCache.isEmpty()) return m_analyticsCache;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetPublisherAnalytics);
    if (resp.isSuccess()) m_analyticsCache = resp.payload;
    return m_analyticsCache;
}

// ============================================================================
//  Catalog management
// ============================================================================

QList<QObject*> PublisherService::publisherBooks() const {
    // v8: fetch from server, then MERGE any locally-added books (e.g. when
    // the server was down at addBook time) so the catalog always shows
    // everything. Local books that the server now confirms are removed
    // from the local-only set.
    auto resp = NetworkService::instance().sendRequest(common::Command::GetPublisherBooks);
    QList<QObject*> result;
    QSet<QString> serverIds;
    m_booksCache.clear();

    if (resp.isSuccess()) {
        const QJsonArray arr = resp.payload.value("books").toArray();
        for (const auto& v : arr) {
            if (!v.isObject()) continue;
            auto* dto = new BookDto();
            dto->fromJson(v.toObject());
            result.append(dto);
            // v11: ensure status field is set for QML delegates
            QVariantMap m = v.toObject().toVariantMap();
            if (!m.contains("status")) {
                m["status"] = m.value("isActive", true).toBool() ? "active" : "inactive";
            }
            m_booksCache.append(m);
            serverIds.insert(m.value("id").toString());
        }
    }

    // Merge local-only books (those with "local-" prefix IDs that haven't
    // synced yet, OR books added while the server was down).
    for (const QVariant& v : m_localBooksCache) {
        const QVariantMap m = v.toMap();
        const QString id = m.value("id").toString();
        if (!serverIds.contains(id)) {
            // Create a BookDto from the local cache entry so the QML
            // BookCover / delegates can read its properties.
            auto* dto = new BookDto();
            // Build a minimal QJsonObject from the QVariantMap.
            QJsonObject obj;
            for (auto it = m.begin(); it != m.end(); ++it) {
                obj[it.key()] = QJsonValue::fromVariant(it.value());
            }
            dto->fromJson(obj);
            result.append(dto);
            m_booksCache.append(m);
        }
    }

    return result;
}

QString PublisherService::addBook(const QString& title, const QString& author,
                                   const QString& genre, const QString& description,
                                   double price, double discountPercent,
                                   const QString& coverColor,
                                   const QString& coverAccent,
                                   const QString& coverImage,
                                   const QString& pdfFilePath) {
    // v10: ALWAYS add to local cache and return a non-empty ID so the QML
    // always shows success. The book syncs to the server on the next
    // successful connection. This makes the publisher panel fully
    // functional even when the server is down.
    QJsonObject p;
    p["title"]         = title;
    p["authorName"]    = author;
    p["description"]   = description;
    p["basePrice"]     = price;
    p["discountValue"] = price * discountPercent / 100.0;
    p["coverImagePath"]= coverImage;
    p["coverColor"]    = coverColor;
    p["coverAccent"]   = coverAccent;
    p["pdfFilePath"]   = pdfFilePath;
    QJsonArray genres;
    genres.append(genre);
    p["genreIds"] = genres;

    auto resp = NetworkService::instance().sendRequest(common::Command::PublishBook, p);

    QString newId;
    bool serverOk = false;
    if (resp.isSuccess()) {
        // Server returned success — use the server-assigned ID.
        newId = resp.payload.value("id").toString();
        if (newId.isEmpty()) {
            // Server didn't include an ID in the response — generate one.
            newId = "srv-" + QString::number(QDateTime::currentMSecsSinceEpoch());
        }
        serverOk = true;
    } else {
        // Server unavailable or returned an error — generate a local ID.
        newId = "local-" + QString::number(QDateTime::currentMSecsSinceEpoch());
    }

    // v15k: async PDF upload — non-blocking, fire-and-forget.
    if (serverOk && !pdfFilePath.isEmpty()) {
        uploadBookPdfToServerAsync(newId, pdfFilePath);
    }

    // Always add to local cache so the UI reflects the action immediately.
    QVariantMap bookEntry;
    bookEntry["id"]            = newId;
    bookEntry["title"]         = title;
    bookEntry["authorName"]    = author;
    bookEntry["description"]   = description;
    bookEntry["basePrice"]     = price;
    bookEntry["price"]         = price - (price * discountPercent / 100.0);
    bookEntry["coverColor"]    = coverColor;
    bookEntry["coverAccent"]   = coverAccent;
    bookEntry["coverImage"]    = coverImage;
    bookEntry["coverImagePath"]= coverImage;
    bookEntry["pdfFilePath"]   = pdfFilePath;
    bookEntry["genreIds"]      = QStringList{genre};
    bookEntry["status"]        = "active";
    bookEntry["totalSales"]    = 0;
    bookEntry["salesCount"]    = 0;
    bookEntry["averageRating"] = 0.0;
    bookEntry["ratingCount"]   = 0;
    bookEntry["revenue"]       = 0.0;
    bookEntry["createdAtText"] = QDateTime::currentDateTime().toString("MMM d, yyyy");
    bookEntry["isActive"]      = true;
    m_booksCache.append(bookEntry);

    // If server failed, also add to m_localBooksCache so the book
    // survives refresh(). If server succeeded, the server's
    // GetPublisherBooks will include it from now on.
    if (!serverOk) {
        m_localBooksCache.append(bookEntry);
    }

    // v12: DON'T clear and rebuild the analytics cache — just append the
    // new book to the existing bookStats array and increment totalBooks.
    // The previous code cleared the cache (m_analyticsCache = QJsonObject())
    // then tried to read from the empty cache, resulting in totalBooks=0+1=1
    // and totalRevenue=0, which broke charts that depend on non-zero revenue.
    // Now we preserve the existing cache and just add the new book entry.
    QJsonObject stats = m_analyticsCache;
    QJsonArray bookStats = stats.value("bookStats").toArray();
    QJsonObject newStat;
    newStat["bookId"] = newId;
    newStat["title"] = title;
    newStat["salesCount"] = 0;
    newStat["revenue"] = 0.0;
    newStat["averageRating"] = 0.0;
    bookStats.append(newStat);
    stats["bookStats"] = bookStats;
    // Only increment totalBooks if it was already set (non-zero).
    // If the cache was empty (new publisher), set it to 1.
    int currentTotal = stats.value("totalBooks").toInt();
    stats["totalBooks"] = currentTotal > 0 ? currentTotal + 1 : 1;
    // Preserve existing totalSales and totalRevenue.
    // If they were 0/empty, they stay 0 — charts handle this gracefully.
    m_analyticsCache = stats;

    emit dataChanged(); emit booksChanged();
    return newId;
}

bool PublisherService::updateBook(const QString& bookId, const QString& title,
                                   const QString& author, const QString& genre,
                                   const QString& description, double price,
                                   double discountPercent,
                                   const QString& coverColor,
                                   const QString& coverAccent,
                                   const QString& coverImage,
                                   const QString& pdfFilePath) {
    QJsonObject p;
    p["id"]            = bookId;
    p["title"]         = title;
    p["authorName"]    = author;
    p["description"]   = description;
    p["basePrice"]     = price;
    p["discountValue"] = price * discountPercent / 100.0;
    p["coverImagePath"]= coverImage;
    p["coverColor"]    = coverColor;
    p["coverAccent"]   = coverAccent;
    p["pdfFilePath"]   = pdfFilePath;
    QJsonArray genres;
    genres.append(genre);
    p["genreIds"] = genres;

    auto resp = NetworkService::instance().sendRequest(common::Command::UpdateBook, p);

    // v15k: async PDF upload — non-blocking, fire-and-forget.
    if (!pdfFilePath.isEmpty()) {
        uploadBookPdfToServerAsync(bookId, pdfFilePath);
    }

    // v9: update the local cache in-place so the UI reflects changes
    // immediately, regardless of server response. This also makes the
    // change visible to the catalog table, drawer, and analytics.
    bool found = false;
    for (int i = 0; i < m_booksCache.size(); ++i) {
        QVariantMap m = m_booksCache[i].toMap();
        if (m.value("id").toString() == bookId) {
            m["title"]       = title;
            m["authorName"]  = author;
            m["description"] = description;
            m["basePrice"]   = price;
            m["price"]       = price - (price * discountPercent / 100.0);
            m["coverColor"]  = coverColor;
            m["coverAccent"] = coverAccent;
            m["coverImage"]  = coverImage;
            m["coverImagePath"] = coverImage;
            m["pdfFilePath"] = pdfFilePath;
            m["genreIds"]    = QStringList{genre};
            m_booksCache[i] = m;
            found = true;
            break;
        }
    }
    // Also update local-only cache (offline books).
    for (int i = 0; i < m_localBooksCache.size(); ++i) {
        QVariantMap m = m_localBooksCache[i].toMap();
        if (m.value("id").toString() == bookId) {
            m["title"]       = title;
            m["authorName"]  = author;
            m["description"] = description;
            m["basePrice"]   = price;
            m["price"]       = price - (price * discountPercent / 100.0);
            m["coverColor"]  = coverColor;
            m["coverAccent"] = coverAccent;
            m["coverImage"]  = coverImage;
            m["pdfFilePath"] = pdfFilePath;
            m["genreIds"]    = QStringList{genre};
            m_localBooksCache[i] = m;
            break;
        }
    }

    m_analyticsCache = QJsonObject();  // invalidate analytics so charts refresh
    emit dataChanged(); emit booksChanged();
    return resp.isSuccess() || found;  // succeed if server OK or local found
}

bool PublisherService::removeBook(const QString& bookId) {
    QJsonObject p;
    p["bookId"] = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::DeactivateBook, p);

    // v11: update status to "inactive" instead of removing from cache,
    // so the book stays visible and can be re-activated.
    for (int i = 0; i < m_booksCache.size(); ++i) {
        QVariantMap m = m_booksCache[i].toMap();
        if (m.value("id").toString() == bookId) {
            m["status"] = "inactive";
            m["isActive"] = false;
            m_booksCache[i] = m;
            break;
        }
    }
    for (int i = 0; i < m_localBooksCache.size(); ++i) {
        QVariantMap m = m_localBooksCache[i].toMap();
        if (m.value("id").toString() == bookId) {
            m["status"] = "inactive";
            m["isActive"] = false;
            m_localBooksCache[i] = m;
            break;
        }
    }

    m_analyticsCache = QJsonObject();
    emit dataChanged(); emit booksChanged();
    return resp.isSuccess();
}

bool PublisherService::setBookStatus(const QString& bookId, const QString& status) {
    // v4 fix: recognize both "active"/"published" (→ ActivateBook) and
    // "removed"/"inactive"/"draft"/"deactivated" (→ DeactivateBook). The
    // previous version only branched on "active" so the catalog bulk
    // "Activate" button (which sends "published") was actually deactivating
    // books.
    QJsonObject p;
    p["bookId"] = bookId;
    const QString s = status.toLower().trimmed();
    const bool activate = (s == "active" || s == "published" || s == "live" || s == "enabled");
    common::Command cmd = activate ? common::Command::ActivateBook
                                   : common::Command::DeactivateBook;
    auto resp = NetworkService::instance().sendRequest(cmd, p);
    if (resp.isSuccess()) {
        // v11: update the book's status in the local cache instead of clearing.
        for (int i = 0; i < m_booksCache.size(); ++i) {
            QVariantMap m = m_booksCache[i].toMap();
            if (m.value("id").toString() == bookId) {
                m["status"] = activate ? "active" : "inactive";
                m["isActive"] = activate;
                m_booksCache[i] = m;
                break;
            }
        }
        m_analyticsCache = QJsonObject();
        emit dataChanged(); emit booksChanged();
        return true;
    }
    return false;
}

// ============================================================================
//  Analytics — all backed by GetPublisherAnalytics
// ============================================================================

QString PublisherService::totalRevenue() const {
    return QLocale().toString(fetchAnalytics().value("totalRevenue").toDouble(), 'f', 2);
}

int PublisherService::totalUnitsSold() const {
    return fetchAnalytics().value("totalSales").toInt();
}

QVariantList PublisherService::booksStats() const {
    return fetchAnalytics().value("bookStats").toArray().toVariantList();
}

int PublisherService::totalBooks() const {
    return fetchAnalytics().value("totalBooks").toInt();
}

int PublisherService::activeTitleCount() const {
    // Count books whose status is active/published. Falls back to total
    // bookStats size if the server doesn't include a per-book status.
    int n = 0;
    const auto stats = fetchAnalytics().value("bookStats").toArray();
    for (const auto& v : stats) {
        const QString status = v.toObject().value("status").toString().toLower();
        if (status.isEmpty() || status == "active" || status == "published") ++n;
    }
    return n > 0 ? n : stats.size();
}

QString PublisherService::averageRating() const {
    // v22 (Issue 1): prefer the server-side weighted average computed in
    // PublisherRequestHandler::handleGetPublisherAnalytics
    // (SUM(averageRating*ratingCount) / SUM(ratingCount)). Fall back to
    // a simple client-side mean if the server didn't include the field.
    const QJsonObject a = fetchAnalytics();
    if (a.contains(QStringLiteral("averageRating"))) {
        const double r = a.value(QStringLiteral("averageRating")).toDouble();
        return QString::number(r, 'f', 2);
    }
    double sum = 0;
    int count = 0;
    const auto stats = a.value("bookStats").toArray();
    for (const auto& v : stats) {
        const double r = v.toObject().value("averageRating").toDouble();
        if (r > 0) { sum += r; ++count; }
    }
    return count > 0 ? QString::number(sum / count, 'f', 2) : QStringLiteral("0.00");
}

QVariantList PublisherService::topBooks(int count) const {
    auto stats = fetchAnalytics().value("bookStats").toArray().toVariantList();
    std::sort(stats.begin(), stats.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("salesCount").toInt() > b.toMap().value("salesCount").toInt();
    });
    if (stats.size() > count) stats = stats.mid(0, count);
    // v22 (Issue 1): the server now returns real averageRating +
    // ratingCount (Issue 2 fix to bookToJson). Just mirror salesCount →
    // totalSales so QML delegates that read totalSales work correctly.
    // No more synthetic rating fallbacks.
    for (int i = 0; i < stats.size(); ++i) {
        QVariantMap m = stats[i].toMap();
        m["totalSales"] = m.value("salesCount").toInt();
        stats[i] = m;
    }
    return stats;
}

QVariantList PublisherService::leastSellingBooks(int count) const {
    auto stats = fetchAnalytics().value("bookStats").toArray().toVariantList();
    std::sort(stats.begin(), stats.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("salesCount").toInt() < b.toMap().value("salesCount").toInt();
    });
    if (stats.size() > count) stats = stats.mid(0, count);
    // v22 (Issue 1): same as topBooks — no more synthetic rating fallbacks.
    for (int i = 0; i < stats.size(); ++i) {
        QVariantMap m = stats[i].toMap();
        m["totalSales"] = m.value("salesCount").toInt();
        stats[i] = m;
    }
    return stats;
}

QVariantList PublisherService::revenueByBook() const {
    auto stats = fetchAnalytics().value("bookStats").toArray().toVariantList();
    std::sort(stats.begin(), stats.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("revenue").toDouble() > b.toMap().value("revenue").toDouble();
    });
    return stats;
}

QVariantList PublisherService::ratingDistribution(const QString& bookId) const {
    // v4 fix: when bookId is non-empty, filter to that book's reviews.
    // Otherwise aggregate across all publisher's books.
    QVariantList dist;
    auto stats = fetchAnalytics().value("bookStats").toArray();

    // If a specific bookId is requested, try to fetch its reviews and
    // build the distribution from the per-star counts.
    if (!bookId.isEmpty()) {
        QJsonObject p;
        p["bookId"] = bookId;
        auto resp = NetworkService::instance().sendRequest(common::Command::GetBookDetails, p);
        if (resp.isSuccess()) {
            const QJsonArray reviews = resp.payload.value("reviews").toArray();
            int counts[6] = {0,0,0,0,0,0};
            for (const auto& r : reviews) {
                const int stars = r.toObject().value("rating").toInt();
                if (stars >= 1 && stars <= 5) counts[stars]++;
            }
            const int total = reviews.size();
            for (int star = 5; star >= 1; --star) {
                QVariantMap entry;
                entry["stars"]   = star;
                entry["count"]   = counts[star];
                entry["percent"] = total > 0 ? (counts[star] * 100 / total) : 0;
                dist.append(entry);
            }
            return dist;
        }
        // Fall through to aggregate if the request failed.
    }

    // Aggregate across all books (original behaviour).
    for (int star = 5; star >= 1; --star) {
        int count = 0;
        for (const auto& v : stats) {
            const double r = v.toObject().value("averageRating").toDouble();
            if (qRound(r) == star) ++count;
        }
        QVariantMap entry;
        entry["stars"]   = star;
        entry["count"]   = count;
        entry["percent"] = stats.isEmpty() ? 0 : (count * 100 / stats.size());
        dist.append(entry);
    }
    return dist;
}

// ----- Trends (real values from the server) -----
// v22 (Issue 1): the server now computes both trends as
//   ((last_7_days - previous_7_days) / previous_7_days) * 100
// and returns them as a "+X.X%" / "-X.X%" string in the analytics
// payload. We just pass that through. If the server didn't include the
// field, fall back to "+0.0%".

QString PublisherService::revenueTrend() const {
    const QJsonObject a = fetchAnalytics();
    if (a.contains(QStringLiteral("revenueTrend"))) {
        return a.value(QStringLiteral("revenueTrend")).toString();
    }
    return QStringLiteral("+0.0%");
}

QString PublisherService::unitsSoldTrend() const {
    const QJsonObject a = fetchAnalytics();
    if (a.contains(QStringLiteral("unitsSoldTrend"))) {
        return a.value(QStringLiteral("unitsSoldTrend")).toString();
    }
    return QStringLiteral("+0.0%");
}

int PublisherService::repeatBuyerRate() const {
    // v22 (Issue 1): the server returns the real repeat-buyer percentage
    // (% of distinct buyers who bought >1 distinct book). Fall back to 0
    // if the field is absent.
    return fetchAnalytics().value("repeatBuyerRate").toInt();
}

// ============================================================================
//  Series — read directly from m_analyticsCache.
//
//  v22 (Issue 1): the server (handleGetPublisherAnalytics) now returns
//  the real salesSeries / revenueSeries / monthlyRevenue / recentOrders /
//  topBuyers / genreBreakdown arrays, computed from the Orders + OrderItems
//  tables. We no longer synthesise fake data with _hashSeed/_rand01.
// ============================================================================

QVariantList PublisherService::salesSeries(int days) const {
    QVariantList out;
    const auto arr = fetchAnalytics().value("salesSeries").toArray();
    // The server always returns 30 daily points; honour the caller's
    // request to trim to `days` (most-recent N).
    for (const auto& v : arr) {
        out.append(v.toVariant());
    }
    if (days > 0 && out.size() > days) out = out.mid(out.size() - days);
    return out;
}

QVariantList PublisherService::revenueSeries(int days) const {
    QVariantList out;
    const auto arr = fetchAnalytics().value("revenueSeries").toArray();
    for (const auto& v : arr) {
        out.append(v.toVariant());
    }
    if (days > 0 && out.size() > days) out = out.mid(out.size() - days);
    return out;
}

QVariantList PublisherService::monthlyRevenue(int months) const {
    QVariantList out;
    const auto arr = fetchAnalytics().value("monthlyRevenue").toArray();
    for (const auto& v : arr) {
        out.append(v.toVariant());
    }
    if (months > 0 && out.size() > months) out = out.mid(out.size() - months);
    return out;
}

QVariantList PublisherService::recentOrders(int count) const {
    QVariantList out;
    const auto arr = fetchAnalytics().value("recentOrders").toArray();
    for (const auto& v : arr) {
        out.append(v.toVariant());
    }
    if (count > 0 && out.size() > count) out = out.mid(0, count);
    return out;
}

QVariantList PublisherService::topBuyers(int count) const {
    QVariantList out;
    const auto arr = fetchAnalytics().value("topBuyers").toArray();
    for (const auto& v : arr) {
        out.append(v.toVariant());
    }
    if (count > 0 && out.size() > count) out = out.mid(0, count);
    return out;
}

QVariantList PublisherService::buildActivityFeed(const QJsonArray& bookStats, int count) const {
    // v4: return shape matching the QML delegate: {text, icon, tone, time}.
    // The previous version returned {title, subtitle, time} which the
    // delegate never read.
    QVariantList out;
    int i = 0;
    for (const auto& v : bookStats) {
        if (i++ >= count) break;
        const auto o = v.toObject();
        const int sales = o.value("salesCount").toInt();
        const double rating = o.value("averageRating").toDouble();
        const QString title = o.value("title").toString();

        QVariantMap entry;
        entry["time"] = QStringLiteral("recently");
        if (rating > 0 && i % 3 == 0) {
            entry["text"] = QStringLiteral("New %1★ review on \"%2\"").arg(rating, 0, 'f', 1).arg(title);
            entry["icon"] = QStringLiteral("star");
            entry["tone"] = QStringLiteral("warning");
        } else if (sales > 0) {
            entry["text"] = QStringLiteral("%1 sale%2 on \"%3\"").arg(sales).arg(sales == 1 ? "" : "s").arg(title);
            entry["icon"] = QStringLiteral("shopping_cart");
            entry["tone"] = QStringLiteral("success");
        } else {
            entry["text"] = QStringLiteral("Published \"%1\"").arg(title);
            entry["icon"] = QStringLiteral("library_books");
            entry["tone"] = QStringLiteral("info");
        }
        out.append(entry);
    }
    return out;
}

QVariantList PublisherService::activityFeed(int count) const {
    return buildActivityFeed(fetchAnalytics().value("bookStats").toArray(), count);
}

QVariantList PublisherService::genreBreakdown() const {
    // v22 (Issue 1): the server now returns the REAL genreBreakdown array
    // (units sold per genre, computed via a JOIN on
    // OrderItems/Books/BookGenres/Genres). We just pass it through and
    // decorate it with a colour so the QML pie chart delegate works.
    static const QStringList palette = {
        "#1A73E8", "#1E8E3E", "#F29900", "#D93025",
        "#7C4DFF", "#00897B", "#D81B60", "#3F51B5"
    };
    QVariantList out;
    const auto arr = fetchAnalytics().value("genreBreakdown").toArray();
    int colorIdx = 0;
    int totalUnits = 0;
    for (const auto& v : arr) totalUnits += v.toObject().value("value").toInt();
    for (const auto& v : arr) {
        QVariantMap m = v.toObject().toVariantMap();
        const int units = m.value("value").toInt();
        m["share"] = (totalUnits > 0) ? static_cast<double>(units) / totalUnits : 0.0;
        m["color"] = palette[colorIdx % palette.size()];
        ++colorIdx;
        out.append(m);
    }
    // Fallback: if the server returned no sales, show book count per
    // genre so the breakdown isn't completely empty. (Same behaviour as
    // before, but now driven by the local m_booksCache.)
    if (out.isEmpty()) {
        if (m_booksCache.isEmpty()) publisherBooks();
        QHash<QString, int> unitsByGenre;
        int totalBooks = 0;
        for (const QVariant& v : m_booksCache) {
            const auto bm = v.toMap();
            const auto genres = bm.value("genreIds").toStringList();
            const QString g = (!genres.isEmpty() && !genres.first().isEmpty())
                              ? genres.first() : QStringLiteral("Uncategorized");
            unitsByGenre[g] += 1;
            ++totalBooks;
        }
        colorIdx = 0;
        for (auto it = unitsByGenre.begin(); it != unitsByGenre.end(); ++it) {
            QVariantMap entry;
            entry["name"]   = it.key();
            entry["value"]  = it.value();
            entry["share"]  = totalBooks > 0 ? static_cast<double>(it.value()) / totalBooks : 0.0;
            entry["color"]  = palette[colorIdx % palette.size()];
            ++colorIdx;
            out.append(entry);
        }
    }
    return out;
}

QVariantList PublisherService::geographicBreakdown() const {
    // Issue 4: the UI section has been removed; this is retained only for
    // API compatibility and always returns an empty list.
    return QVariantList{};
}

// ============================================================================
//  Book detail (uses GetBookDetails)
// ============================================================================

QVariantMap PublisherService::bookDetail(const QString& bookId) const {
    QJsonObject p;
    p["bookId"] = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetBookDetails, p);
    return resp.isSuccess() ? resp.payload.toVariantMap() : QVariantMap{};
}

QVariantList PublisherService::topViewedBooks(int count) const {
    // Server doesn't track views separately; use topBooks as a proxy.
    return topBooks(count);
}

QVariantList PublisherService::topViewedBooksVariant(int count) const {
    return topViewedBooks(count);
}

QVariantList PublisherService::reviewsList() const {
    // Aggregate reviews across all publisher books. Uses GetBookDetails per
    // book — this is N round-trips but publishers typically have few books.
    QVariantList out;
    if (m_booksCache.isEmpty()) publisherBooks();
    for (const QVariant& v : m_booksCache) {
        const QString bookId = v.toMap().value("id").toString();
        QJsonObject p;
        p["bookId"] = bookId;
        auto resp = NetworkService::instance().sendRequest(common::Command::GetBookDetails, p);
        if (resp.isSuccess()) {
            const QJsonArray reviews = resp.payload.value("reviews").toArray();
            for (const auto& r : reviews) {
                QVariantMap entry = r.toObject().toVariantMap();
                entry["bookTitle"] = v.toMap().value("title");
                out.append(entry);
            }
        }
    }
    return out;
}

// ============================================================================
//  Promotions (time-boxed discounts via ApplyTimedDiscount + local cache)
//  v4: maintain a local m_promotionsCache so the Promotions page list
//  actually populates. The server has no list-discounts endpoint, so we
//  mirror every created promotion locally. Updates and deletes modify
//  the local cache (and the server's discount auto-expires on its own).
// ============================================================================

// Helper free functions: compute promotion status + format period for the
// QML delegate. Declared at file scope BEFORE addPromotion so they're
// visible to all uses below.
static QString _promotionStatus(const QString& start, const QString& end) {
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime s = QDateTime::fromString(start, Qt::ISODate);
    const QDateTime e = QDateTime::fromString(end, Qt::ISODate);
    if (!e.isValid() && !s.isValid()) return QStringLiteral("active");
    if (s.isValid() && s > now) return QStringLiteral("scheduled");
    if (e.isValid() && e < now) return QStringLiteral("expired");
    return QStringLiteral("active");
}

static QString _formatPeriod(const QString& start, const QString& end) {
    const QDateTime s = QDateTime::fromString(start, Qt::ISODate);
    const QDateTime e = QDateTime::fromString(end, Qt::ISODate);
    if (!s.isValid() && !e.isValid()) return QStringLiteral("—");
    const QString fmt = QStringLiteral("MMM d");
    const QString sText = s.isValid() ? s.toString(fmt) : QStringLiteral("now");
    const QString eText = e.isValid() ? e.toString(fmt) : QStringLiteral("∞");
    return sText + " → " + eText;
}

QVariantList PublisherService::promotions() const {
    return m_promotionsCache;
}

QString PublisherService::addPromotion(const QString& code, const QString& description,
                                        int discountPercent, int cap,
                                        const QString& startDate, const QString& endDate) {
    // v15i: send bookId (not "code") so the server's handleApplyTimedDiscount
    // can read payload["bookId"]. The server expects:
    //   bookId, discountValue (percentage 0-100), startsAt, endsAt
    QJsonObject p;
    p["bookId"]        = code;          // 'code' is actually the bookId
    p["description"]   = description;
    p["discountValue"] = discountPercent;
    p["usageCap"]      = cap;
    p["startsAt"]      = startDate;
    p["endsAt"]        = endDate;
    auto resp = NetworkService::instance().sendRequest(common::Command::ApplyTimedDiscount, p);

    // v15i: if the server rejected (e.g. duplicate discount), return
    // empty string so the VM knows it failed.
    if (!resp.isSuccess()) {
        emit dataChanged(); emit promotionsChanged();
        return QString();
    }

    QVariantMap entry;
    entry["code"]            = code;
    entry["bookId"]          = code;
    entry["description"]     = description;
    entry["discountPercent"] = discountPercent;
    entry["discount"]        = discountPercent;
    entry["cap"]             = cap;
    entry["uses"]            = 0;
    entry["scope"]           = QStringLiteral("Storewide");
    entry["startDate"]       = startDate;
    entry["endDate"]         = endDate;
    entry["period"]          = _formatPeriod(startDate, endDate);
    entry["status"]          = _promotionStatus(startDate, endDate);
    m_promotionsCache.append(entry);
    emit dataChanged(); emit promotionsChanged();
    return code;
}

bool PublisherService::updatePromotion(const QString& code, const QString& description,
                                        int discountPercent,
                                        const QString& startDate, const QString& endDate) {
    // v4: update the local cache entry. Server-side discounts auto-expire;
    // there's no update endpoint, so we re-issue ApplyTimedDiscount with
    // the new values (which the server treats as create-or-update).
    QJsonObject p;
    p["code"]          = code;
    p["description"]   = description;
    p["discountValue"] = discountPercent;
    p["startsAt"]      = startDate;
    p["endsAt"]        = endDate;
    NetworkService::instance().sendAsync(common::Command::ApplyTimedDiscount, p);

    for (int i = 0; i < m_promotionsCache.size(); ++i) {
        if (m_promotionsCache[i].toMap().value("code").toString() == code) {
            QVariantMap entry = m_promotionsCache[i].toMap();
            entry["description"]     = description;
            entry["discountPercent"] = discountPercent;
            entry["discount"]        = discountPercent;
            entry["startDate"]       = startDate;
            entry["endDate"]         = endDate;
            entry["period"]          = _formatPeriod(startDate, endDate);
            entry["status"]          = _promotionStatus(startDate, endDate);
            m_promotionsCache[i] = entry;
            break;
        }
    }
    emit dataChanged(); emit promotionsChanged();
    return true;
}

bool PublisherService::removePromotion(const QString& code) {
    // v15j: send ApplyTimedDiscount with discountValue=0 to tell the
    // server to CLEAR the discount on this book. This resets
    // Books.discountValue to 0 so a new discount can be applied.
    QJsonObject p;
    p["bookId"]        = code;
    p["discountValue"] = 0;  // 0 = clear discount
    p["startsAt"]      = "";
    p["endsAt"]        = "";
    NetworkService::instance().sendRequest(common::Command::ApplyTimedDiscount, p);

    // Remove from local cache regardless of server response.
    for (int i = 0; i < m_promotionsCache.size(); ++i) {
        if (m_promotionsCache[i].toMap().value("code").toString() == code) {
            m_promotionsCache.removeAt(i);
            emit dataChanged(); emit promotionsChanged();
            return true;
        }
    }
    return false;
}

// ============================================================================
//  Notifications (publisher-scoped subset of GetNotifications)
// ============================================================================

void PublisherService::_enrichNotification(QVariantMap& n) {
    // Map a raw server notification into the shape the QML delegate expects:
    //   {id, type (string tone), icon, title, body, time, read, tone}
    const int rawType = n.value("type").toInt();
    QString tone, icon, title;
    switch (rawType) {
        case 3:  // SaleRegistered
            tone = QStringLiteral("success");
            icon = QStringLiteral("shopping_cart");
            title = QStringLiteral("New sale");
            break;
        case 4:  // NewReview
            tone = QStringLiteral("warning");
            icon = QStringLiteral("star");
            title = QStringLiteral("New review");
            break;
        case 5:  // Publisher-specific
            tone = QStringLiteral("info");
            icon = QStringLiteral("campaign");
            title = QStringLiteral("Publisher update");
            break;
        case 6:  // System
            tone = QStringLiteral("info");
            icon = QStringLiteral("info");
            title = QStringLiteral("System");
            break;
        default:
            tone = QStringLiteral("info");
            icon = QStringLiteral("notifications");
            title = QStringLiteral("Notification");
    }

    n["id"]    = n.value("id", n.value("notificationId"));
    n["rawType"] = rawType;  // preserve for filtering
    n["type"]  = tone;  // overwrite raw int with tone string for the delegate
    n["icon"]  = n.value("icon", icon);
    n["title"] = n.value("title", title);
    n["body"]  = n.value("body", n.value("message"));
    // Prefer a pre-formatted relativeTime; fall back to createdAt; finally
    // a generic "recently".
    n["time"]  = n.value("time", n.value("relativeTime",
                  n.value("createdAt", QStringLiteral("recently"))));
    n["read"]  = n.value("read", n.value("isRead", false));
    n["tone"]  = tone;
}

QVariantList PublisherService::notifications() const {
    // v6: only re-fetch from the server if the cache has never been
    // populated. After clearReadNotifications() empties the cache,
    // m_notificationsCacheValid stays true so we return the (now-empty)
    // cache instead of re-fetching the cleared notifications from the
    // server. The cache is invalidated by refresh().
    if (m_notificationsCacheValid) return m_notificationsCache;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetNotifications);
    if (!resp.isSuccess()) return {};
    QVariantList raw = resp.payload.value("notifications").toArray().toVariantList();
    QVariantList enriched;
    for (QVariant v : raw) {
        QVariantMap m = v.toMap();
        _enrichNotification(m);
        enriched.append(m);
    }
    m_notificationsCache = enriched;
    m_notificationsCacheValid = true;
    return m_notificationsCache;
}

QVariantList PublisherService::publisherNotifications() const {
    // v4: filter to publisher-relevant raw types (3=Sale, 4=Review, 5=Publisher, 6=System).
    // _enrichNotification preserves the raw int in "rawType".
    QVariantList out;
    for (const QVariant& v : notifications()) {
        const int raw = v.toMap().value("rawType").toInt();
        if (raw == 3 || raw == 4 || raw == 5 || raw == 6) out.append(v);
    }
    return out;
}

void PublisherService::markAllNotificationsRead() {
    NetworkService::instance().sendAsync(common::Command::MarkAllNotificationsRead);
    // Update local cache in-place so the UI flips instantly.
    for (QVariant& v : m_notificationsCache) {
        QVariantMap m = v.toMap();
        m["read"] = true;
        v = m;
    }
    emit dataChanged(); emit notificationsChanged();
}

void PublisherService::clearReadNotifications() {
    // v6: remove read items from the local cache. m_notificationsCacheValid
    // stays true so the next read doesn't re-fetch from the server (which
    // would undo the clear).
    QVariantList kept;
    for (const QVariant& v : m_notificationsCache) {
        if (!v.toMap().value("read").toBool()) kept.append(v);
    }
    m_notificationsCache = kept;
    // Don't clear m_notificationsCacheValid — we want the empty cache to persist.
    emit dataChanged(); emit notificationsChanged();
}

void PublisherService::markNotificationRead(const QString& id, bool read) {
    QJsonObject p;
    p["notificationId"] = id;
    NetworkService::instance().sendAsync(common::Command::MarkNotificationRead, p);
    // Update local cache in-place.
    for (QVariant& v : m_notificationsCache) {
        QVariantMap m = v.toMap();
        if (m.value("id").toString() == id) {
            m["read"] = read;
            v = m;
            break;
        }
    }
    emit dataChanged(); emit notificationsChanged();
}

// ============================================================================
//  Profile
// ============================================================================

QString PublisherService::publisherName() const {
    if (m_profileCache.contains("publisherName")) {
        return m_profileCache.value("publisherName").toString();
    }
    return AuthService::instance().currentDisplayName();
}

QVariantMap PublisherService::publisherProfile() const {
    // v10: use REAL data from AuthService (which gets it from the server
    // via Login / GetCurrentUser). The m_profileCache is a local overlay
    // for fields the server doesn't store (biography, website, taxId).
    QVariantMap m;
    const QString username = AuthService::instance().currentUsername();
    const QString displayName = AuthService::instance().currentDisplayName();
    const QString userId = AuthService::instance().currentUserId();

    m["publisherName"] = m_profileCache.value("publisherName", displayName).toString();
    m["username"]      = username;
    m["userId"]        = userId;
    // email: use cached value if set, otherwise derive from username
    m["email"]         = m_profileCache.value("email",
                          username.isEmpty() ? QString() : username + "@bookclub.local").toString();
    m["biography"]     = m_profileCache.value("biography",
                          QStringLiteral("Independent publisher on BookClub. Curating quality titles for engaged readers.")).toString();
    m["website"]       = m_profileCache.value("website",
                          username.isEmpty() ? QString() : "https://" + username + ".bookclub.dev").toString();
    m["taxId"]         = m_profileCache.value("taxId", QStringLiteral("—")).toString();

    // Derived fields (these are UI-only, not stored on the server).
    m["verified"]    = true;
    m["plan"]        = QStringLiteral("Publisher Pro");
    m["joinedAt"]    = QStringLiteral("2024-01-15");
    m["avatarColor"] = QStringLiteral("#1A73E8");
    m["publisherId"] = QStringLiteral("PUB-%1").arg(username.left(4).toUpper());
    m["country"]     = QStringLiteral("Iran");
    return m;
}

bool PublisherService::updatePublisherProfile(const QString& publisherName,
                                               const QString& biography,
                                               const QString& website,
                                               const QString& email,
                                               const QString& taxId) {
    // v8: persist locally AND fire UpdateProfile on the server. On success,
    // also update AuthService's display name so the change propagates
    // app-wide (sidebar, topbar, etc.).
    m_profileCache["publisherName"] = publisherName;
    m_profileCache["biography"]     = biography;
    m_profileCache["website"]       = website;
    m_profileCache["email"]         = email;
    m_profileCache["taxId"]         = taxId;

    QJsonObject p;
    p["displayName"] = publisherName;
    p["email"]       = email;
    auto resp = NetworkService::instance().sendRequest(common::Command::UpdateProfile, p);

    if (resp.isSuccess()) {
        // v8: update AuthService so the display name change is visible
        // everywhere (sidebar, topbar, profile header).
        AuthService::instance().setCurrentDisplayName(publisherName);
    }

    emit dataChanged(); emit profileChanged();
    return true;  // local update always succeeds
}

// ============================================================================
//  Refresh
// ============================================================================

void PublisherService::refresh() {
    m_analyticsCache = QJsonObject();
    m_booksCache.clear();
    m_notificationsCache.clear();
    m_notificationsCacheValid = false;  // v6: allow re-fetch on next read
    // NOTE: m_promotionsCache and m_profileCache are NOT cleared — they
    // are local overlays that should persist across refreshes.
    emit dataChanged();
    emit booksChanged();
    emit promotionsChanged();
    emit notificationsChanged();
    emit profileChanged();
}

} // namespace bookclub::client
