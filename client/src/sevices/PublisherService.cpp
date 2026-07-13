// =============================================================================
//  PublisherService.cpp
// =============================================================================
#include "services/PublisherService.h"
#include "services/BookDto.h"

#include <QDateTime>
#include <QLocale>
#include <QHash>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

namespace bookclub::client {

PublisherService::PublisherService(QObject* parent)
    : QObject(parent)
{
    _seedPromotions();
    _seedNotifications();
    _seedProfile();
}

void PublisherService::setDataStore(MockDataStore* store) {
    m_store = store;
}

// ---- Catalog ----

QList<QObject*> PublisherService::publisherBooks() const {
    QList<QObject*> out;
    if (!m_store) return out;
    for (const auto& b : m_store->books()) {
        out.append(new BookDto(b));
    }
    return out;
}

QString PublisherService::addBook(const QString& title, const QString& author,
                                const QString& genre, const QString& description,
                                double price, double discountPercent,
                                const QString& coverColor, const QString& coverAccent,
                                const QString& coverImage,
                                const QString& pdfFilePath) {
    if (!m_store) return {};
    QString id = m_store->addBook(title, author, genre, description,
                                   price, discountPercent, coverColor, coverAccent);
    if (!id.isEmpty()) {
        // If the caller supplied a cover image or PDF path, patch the book.
        // Pass 11 args so coverImage and pdfFilePath land in the correct
        // parameters (previously only 9 args were passed, which put
        // coverImage in the coverColor slot and pdfFilePath in coverAccent).
        if (!coverImage.isEmpty() || !pdfFilePath.isEmpty()) {
            m_store->updateBook(id, QString(), QString(), QString(), QString(),
                                -1.0, 0.0, QString(), QString(),
                                coverImage, pdfFilePath);
        }
        emit booksChanged();
    }
    return id;
}

bool PublisherService::updateBook(const QString& bookId, const QString& title,
                                   const QString& author, const QString& genre,
                                   const QString& description, double price,
                                   double discountPercent,
                                   const QString& coverColor, const QString& coverAccent,
                                   const QString& coverImage,
                                   const QString& pdfFilePath) {
    if (!m_store || bookId.isEmpty()) return false;
    bool ok = m_store->updateBook(bookId, title, author, genre, description,
                                    price, discountPercent, coverColor, coverAccent);
    // Patch cover image / PDF path if supplied. Pass 11 args so the file
    // paths land in the correct parameters (coverImage, pdfFilePath), not
    // in coverColor/coverAccent as the previous 9-arg call did.
    if (ok && (!coverImage.isEmpty() || !pdfFilePath.isEmpty())) {
        m_store->updateBook(bookId, QString(), QString(), QString(), QString(),
                            -1.0, 0.0, QString(), QString(),
                            coverImage, pdfFilePath);
    }
    if (ok) emit booksChanged();
    return ok;
}

bool PublisherService::removeBook(const QString& bookId) {
    // Soft-delete: mark the book as "removed" so it can be re-published later.
    // The book row stays in the catalog with status="removed" and is filtered
    // out of the storefront by `MockBook::active=false`.
    if (!m_store || bookId.isEmpty()) return false;
    bool ok = m_store->setBookStatus(bookId, QStringLiteral("removed"));
    if (ok) emit booksChanged();
    return ok;
}

bool PublisherService::setBookStatus(const QString& bookId, const QString& status) {
    if (!m_store || bookId.isEmpty()) return false;
    bool ok = m_store->setBookStatus(bookId, status);
    if (ok) emit booksChanged();
    return ok;
}

// ---- Analytics ----

QString PublisherService::totalRevenue() const {
    if (!m_store) return QStringLiteral("$0");
    double total = 0;
    for (const auto& b : m_store->books()) total += b.totalSales * b.price;
    return QLocale().toCurrencyString(total, QStringLiteral("$"));
}

int PublisherService::totalUnitsSold() const {
    if (!m_store) return 0;
    int total = 0;
    for (const auto& b : m_store->books()) total += b.totalSales;
    return total;
}

int PublisherService::activeTitleCount() const {
    if (!m_store) return 0;
    int n = 0;
    for (const auto& b : m_store->books()) {
        if (b.status == QStringLiteral("published") && b.active) ++n;
    }
    return n;
}

QString PublisherService::averageRating() const {
    if (!m_store) return QStringLiteral("0.00");
    double sum = 0; int count = 0;
    for (const auto& b : m_store->books()) {
        if (b.ratingCount > 0) { sum += b.averageRating; ++count; }
    }
    return QString::number(count > 0 ? sum / count : 0.0, 'f', 2);
}

QList<QObject*> PublisherService::topSellingBooks(int count) const {
    QList<QObject*> out;
    if (!m_store) return out;
    auto books = m_store->books();
    std::sort(books.begin(), books.end(), [](const MockBook& a, const MockBook& b) {
        return a.totalSales > b.totalSales;
    });
    int n = qMin(count, books.size());
    for (int i = 0; i < n; ++i) out.append(new BookDto(books[i]));
    return out;
}

// Top N most-viewed books. We don't track per-book view events in the mock,
// so we use `ratingCount` as a proxy — books with more ratings have been seen
// by more readers. The synthesized `viewCount` returned in the QVariantList
// form below is a deterministic multiple of ratingCount to make the number
// feel plausible on the dashboard.
QList<QObject*> PublisherService::topViewedBooks(int count) const {
    QList<QObject*> out;
    if (!m_store) return out;
    auto books = m_store->books();
    std::sort(books.begin(), books.end(), [](const MockBook& a, const MockBook& b) {
        return a.ratingCount > b.ratingCount;
    });
    int n = qMin(count, books.size());
    for (int i = 0; i < n; ++i) out.append(new BookDto(books[i]));
    return out;
}

QVariantList PublisherService::topBooks() const {
    QVariantList out;
    if (!m_store) return out;
    auto books = m_store->books();
    std::sort(books.begin(), books.end(), [](const MockBook& a, const MockBook& b) {
        return a.totalSales > b.totalSales;
    });
    int n = qMin(5, books.size());  // spec §3-3: top 5 bestsellers
    for (int i = 0; i < n; ++i) {
        const auto& b = books[i];
        QVariantMap m;
        m["title"] = b.title;
        m["authorName"] = b.authorName;
        m["totalSales"] = b.totalSales;
        m["price"] = b.price;
        m["averageRating"] = b.averageRating;
        m["ratingCount"] = b.ratingCount;
        m["coverColor"] = b.coverColor;
        m["coverAccent"] = b.coverAccent;
        m["coverImage"] = b.coverImage;
        out.append(m);
    }
    return out;
}

// spec §3-3: Top N least-selling books (ascending by totalSales).
QVariantList PublisherService::leastSellingBooks(int count) const {
    QVariantList out;
    if (!m_store) return out;
    auto books = m_store->books();
    // Filter out removed books — they're not “selling” at all.
    QList<MockBook> active;
    for (const auto& b : books) {
        if (b.status != QStringLiteral("removed")) active.append(b);
    }
    std::sort(active.begin(), active.end(), [](const MockBook& a, const MockBook& b) {
        return a.totalSales < b.totalSales;
    });
    int n = qMin(count, active.size());
    for (int i = 0; i < n; ++i) {
        const auto& b = active[i];
        QVariantMap m;
        m["title"] = b.title;
        m["authorName"] = b.authorName;
        m["totalSales"] = b.totalSales;
        m["price"] = b.price;
        m["averageRating"] = b.averageRating;
        m["ratingCount"] = b.ratingCount;
        m["coverColor"] = b.coverColor;
        m["coverAccent"] = b.coverAccent;
        m["coverImage"] = b.coverImage;
        out.append(m);
    }
    return out;
}

QVariantList PublisherService::topViewedBooksVariant(int count) const {
    QVariantList out;
    if (!m_store) return out;
    auto books = m_store->books();
    std::sort(books.begin(), books.end(), [](const MockBook& a, const MockBook& b) {
        return a.ratingCount > b.ratingCount;
    });
    int n = qMin(count, books.size());
    for (int i = 0; i < n; ++i) {
        const auto& b = books[i];
        QVariantMap m;
        m["title"] = b.title;
        m["authorName"] = b.authorName;
        m["totalSales"] = b.totalSales;
        m["price"] = b.price;
        m["averageRating"] = b.averageRating;
        m["ratingCount"] = b.ratingCount;
        m["coverColor"] = b.coverColor;
        m["coverAccent"] = b.coverAccent;
        // Synthesize a view count from ratingCount (deterministic multiplier).
        m["viewCount"] = b.ratingCount * 7 + b.totalSales;
        out.append(m);
    }
    return out;
}

QVariantList PublisherService::revenueSeries(int days) const {
    QVariantList out;
    double base = 3000;
    for (int i = 0; i < days; ++i) {
        double v = base * (0.7 + (i * 0.13) / days + (i % 5) * 0.04);
        out.append(QVariant(v));
    }
    return out;
}

QVariantList PublisherService::genreBreakdown() const {
    QVariantList out;
    if (!m_store) return out;
    QHash<QString, int> counts;
    for (const auto& b : m_store->books()) {
        for (const auto& g : b.genreIds) counts[g]++;
    }
    int total = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) total += it.value();
    static const QStringList palette = {"#1A73E8","#1E8E3E","#F29900","#9C27B0","#D93025","#00897B","#795548","#3F51B5"};
    int idx = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QVariantMap m;
        m["name"] = it.key();
        m["value"] = it.value();
        m["color"] = palette[idx % palette.size()];
        m["share"] = total > 0 ? qreal(it.value()) / total : 0.0;
        out.append(m);
        ++idx;
    }
    return out;
}

// ---- Geographic breakdown ----
//   Replaces the hardcoded _regions array in PublisherSalesPage.qml.
//   Derives a deterministic distribution from the catalog's sales data so
//   the chart always adds up to ~100%.

QVariantList PublisherService::geographicBreakdown() const {
    QVariantList out;
    if (!m_store) return out;
    // Compute total revenue to proportionally assign to regions.
    double totalRev = 0.0;
    for (const auto& b : m_store->books()) totalRev += b.totalSales * b.price;
    if (totalRev <= 0.0) totalRev = 1.0;

    struct Region { QString name; double share; };
    // Deterministic shares summing to ~1.0.
    static const QList<Region> regions = {
        {QStringLiteral("North America"), 0.42},
        {QStringLiteral("Europe"),        0.28},
        {QStringLiteral("Asia Pacific"),  0.18},
        {QStringLiteral("Latin America"), 0.07},
        {QStringLiteral("Middle East"),   0.03},
        {QStringLiteral("Africa"),        0.02}
    };
    static const QStringList palette = {"#1A73E8","#1E8E3E","#F29900","#9C27B0","#D93025","#00897B"};
    for (int i = 0; i < regions.size(); ++i) {
        QVariantMap m;
        m["name"]     = regions[i].name;
        m["share"]    = regions[i].share;
        m["revenue"]  = totalRev * regions[i].share;
        m["revenueText"] = QStringLiteral("$%1").arg(totalRev * regions[i].share, 0, 'f', 0);
        m["color"]    = palette[i % palette.size()];
        out.append(m);
    }
    return out;
}

// ---- Activity feed ----
//   Replaces the hardcoded _activity array in PublisherDashboardPage.qml.
//   Synthesizes a deterministic feed from recent catalog events.

QVariantList PublisherService::activityFeed(int count) const {
    QVariantList out;
    if (!m_store) return out;

    // Build a feed from the top-selling books (deterministic, stable across refreshes).
    auto books = m_store->books();
    std::sort(books.begin(), books.end(), [](const MockBook& a, const MockBook& b) {
        return a.totalSales > b.totalSales;
    });

    static const QStringList eventTypes = {"sale", "review", "promo", "publish", "flag"};
    static const QStringList eventIcons = {"shopping_cart", "star", "campaign", "menu_book", "report"};
    static const QStringList eventTones = {"success", "info", "warning", "info", "error"};
    static const QStringList customers = {
        "Alice R.", "Bob B.", "Carol M.", "David K.", "Evan T.",
        "Fatima Z.", "George M.", "Hana K."
    };

    int n = qMin(count, books.size() * 2);
    for (int i = 0; i < n; ++i) {
        const auto& b = books[i % books.size()];
        const int evtIdx = i % eventTypes.size();
        QVariantMap m;
        m["icon"] = eventIcons[evtIdx];
        m["tone"] = eventTones[evtIdx];
        m["time"] = QStringLiteral("%1m ago").arg(2 + i * 7);
        m["bookTitle"] = b.title;

        QString text;
        if (eventTypes[evtIdx] == "sale") {
            text = QStringLiteral("New sale: <b>%1</b> × %2 copies").arg(b.title).arg(1 + (i % 5));
        } else if (eventTypes[evtIdx] == "review") {
            text = QStringLiteral("<b>%1</b> rated <b>%2</b> %3 stars").arg(customers[i % customers.size()]).arg(b.title).arg(3 + (i % 3));
        } else if (eventTypes[evtIdx] == "promo") {
            text = QStringLiteral("Promotion <b>SUMMER25</b> applied to <b>%1</b>").arg(b.title);
        } else if (eventTypes[evtIdx] == "publish") {
            text = QStringLiteral("You published a new title: <b>%1</b>").arg(b.title);
        } else {
            text = QStringLiteral("A review on <b>%1</b> was flagged for moderation").arg(b.title);
        }
        m["text"] = text;
        out.append(m);
    }
    return out;
}

// ---- Repeat-buyer rate ----
//   Synthesizes a deterministic 55-70% rate from the catalog's sales volume.
//   In a real backend this would come from the orders table.

int PublisherService::repeatBuyerRate() const {
    if (!m_store) return 0;
    int totalSales = 0;
    for (const auto& b : m_store->books()) totalSales += b.totalSales;
    if (totalSales == 0) return 0;
    // Deterministic: 55 + (totalSales % 16) → range 55-70.
    return 55 + (totalSales % 16);
}

// ---- Units-sold trend ----
//   Returns "+X.X%" or "-X.X%" comparing last 7 days vs previous 7 days.
//   Uses the same revenueSeries shape but applied to units (approximated).

QString PublisherService::unitsSoldTrend() const {
    // Synthesize from revenueTrend (both are ±a few %).
    const QString rt = revenueTrend();
    // Scale the revenue trend slightly for units (units tend to be more volatile).
    bool ok = false;
    double pct = rt.mid(rt.indexOf(QRegularExpression("[+-]"))).chopped(1).toDouble(&ok);
    if (!ok) return QStringLiteral("+0.0%");
    pct *= 0.85;
    return (pct >= 0 ? QStringLiteral("+") : QStringLiteral(""))
           + QString::number(pct, 'f', 1) + QStringLiteral("%");
}

// ---- Per-book rating distribution (spec §3-3) ----
//   Returns a 5-element QVariantList of { stars, count, share, label } for
//   the given book. If the book has no reviews, all counts are 0.

QVariantList PublisherService::ratingDistribution(const QString& bookId) const {
    QVariantList out;
    if (!m_store || bookId.isEmpty()) {
        // Return 5 zero-filled entries so the chart always renders.
        for (int s = 5; s >= 1; --s) {
            QVariantMap m;
            m["stars"] = s;
            m["count"] = 0;
            m["share"] = 0.0;
            m["label"] = QStringLiteral("%1★").arg(s);
            out.append(m);
        }
        return out;
    }
    // Count reviews per star rating for this book.
    int counts[6] = {0,0,0,0,0,0};  // index 1..5
    const auto reviews = m_store->reviewsForBook(bookId);
    for (const auto& r : reviews) {
        if (r.rating >= 1 && r.rating <= 5) ++counts[r.rating];
    }
    int total = 0;
    for (int s = 1; s <= 5; ++s) total += counts[s];
    for (int s = 5; s >= 1; --s) {
        QVariantMap m;
        m["stars"] = s;
        m["count"] = counts[s];
        m["share"] = total > 0 ? qreal(counts[s]) / total : 0.0;
        m["label"] = QStringLiteral("%1★").arg(s);
        out.append(m);
    }
    return out;
}

// ---- Extended analytics (Phase 6) ----

QVariantList PublisherService::monthlyRevenue(int months) const {
    QVariantList out;
    if (!m_store) return out;

    // Build a deterministic per-month revenue series by walking the catalog's
    // total sales and distributing them across the last `months` months using
    // a stable multiplier per month (so the chart shape is consistent across
    // refreshes). Each entry: { label, value, month, year }.
    const QDateTime now = QDateTime::currentDateTime();
    QDate cur = now.date().addMonths(-(months - 1));
    static const QStringList monthLabels = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    // Synthesize a base revenue from the catalog total, then vary it per
    // month with a sinusoidal pattern (Q3 peak, Q1 trough) plus deterministic
    // noise from the month index.
    double totalRev = 0.0;
    for (const auto& b : m_store->books()) totalRev += b.totalSales * b.price;
    const double monthlyBase = totalRev / qMax(1, months);

    for (int i = 0; i < months; ++i) {
        const int m = cur.month();          // 1-12
        const int y = cur.year();
        // Seasonal multiplier: peak in months 6-8 (summer reading), trough in
        // months 12-2 (winter). Sin curve gives a smooth shape.
        const double seasonal = 0.85 + 0.30 * std::sin((m - 1) * M_PI / 6.0);
        // Deterministic noise — varies ±10% per month, stable across runs.
        const double noise = 0.90 + 0.20 * ((i * 17 % 11) / 10.0);
        const double value = monthlyBase * seasonal * noise;

        QVariantMap entry;
        entry["label"] = monthLabels.at(m - 1);
        entry["value"] = value;
        entry["month"] = m;
        entry["year"]  = y;
        out.append(entry);
        cur = cur.addMonths(1);
    }
    return out;
}

QVariantList PublisherService::recentOrders(int count) const {
    QVariantList out;
    if (!m_store) return out;

    // Synthesize a recent-orders feed by picking books from the catalog in
    // sales-ranked order, then generating timestamps spread across the last
    // 24 hours. Customer names are deterministic per book so the same book
    // always shows the same buyer (plausible for repeat-purchase patterns).
    auto books = m_store->books();
    std::sort(books.begin(), books.end(), [](const MockBook& a, const MockBook& b) {
        return a.totalSales > b.totalSales;
    });

    static const QStringList customers = {
        "Alice Walker", "Bob Johnson", "Charlie Brown", "Diana Prince",
        "Evan Torres", "Fatima Zahra", "George Miller", "Hana Kim",
        "Ian Smith", "Julia Reyes", "Kai Lee", "Liam O'Brien"
    };
    static const QStringList statuses = {
        "Completed", "Completed", "Completed", "Pending", "Refunded"
    };
    const QDateTime now = QDateTime::currentDateTime();

    int n = qMin(count, books.size() * 2);
    for (int i = 0; i < n; ++i) {
        const auto& b = books[i % books.size()];
        // Spread orders across the last 24 hours, most recent first.
        const QDateTime when = now.addSecs(-(i * 137 + 60));

        QVariantMap m;
        m["orderId"]   = QStringLiteral("ORD-%1").arg(10000 + i, 5, 10, QChar('0'));
        m["bookId"]    = b.id;
        m["bookTitle"] = b.title;
        m["customer"]  = customers[(i * 5) % customers.size()];
        m["quantity"]  = 1 + (i % 3);
        m["total"]     = b.price * m["quantity"].toInt();
        m["time"]      = when.toString(QStringLiteral("MMM d, hh:mm"));
        m["status"]    = statuses[i % statuses.size()];
        out.append(m);
    }
    return out;
}

QVariantList PublisherService::topBuyers(int count) const {
    QVariantList out;
    if (!m_store) return out;

    // Pick top buyers deterministically from the customer pool. Each buyer
    // gets a stable books-purchased count + total-spent derived from the
    // catalog's sales data.
    static const QStringList customers = {
        "Alice Walker", "Bob Johnson", "Charlie Brown", "Diana Prince",
        "Evan Torres", "Fatima Zahra", "George Miller", "Hana Kim"
    };
    static const QStringList avatars = {
        "#1A73E8", "#1E8E3E", "#F29900", "#9C27B0",
        "#D93025", "#00897B", "#795548", "#3F51B5"
    };
    double catalogTotal = 0.0;
    for (const auto& b : m_store->books()) catalogTotal += b.totalSales * b.price;
    const double avgPerBuyer = catalogTotal / qMax(1, customers.size());

    int n = qMin(count, customers.size());
    for (int i = 0; i < n; ++i) {
        QVariantMap m;
        const QString& name = customers[i];
        QStringList parts = name.split(' ', Qt::SkipEmptyParts);
        QString initials;
        for (const auto& p : parts) {
            if (!p.isEmpty()) initials += p[0].toUpper();
        }
        m["username"]     = name.toLower().replace(' ', '_');
        m["displayName"]  = name;
        m["initials"]     = initials.left(2);
        m["avatarColor"]  = avatars[i % avatars.size()];
        m["books"]        = 3 + (i * 7 % 25);
        m["totalSpent"]   = avgPerBuyer * (1.4 - i * 0.12);  // top buyer spent most
        m["lastOrder"]    = QStringLiteral("%1 days ago").arg(i + 1);
        out.append(m);
    }
    return out;
}

QVariantMap PublisherService::bookDetail(const QString& bookId) const {
    QVariantMap out;
    if (!m_store || bookId.isEmpty()) return out;
    for (const auto& b : m_store->books()) {
        if (b.id != bookId) continue;
        out["id"]            = b.id;
        out["title"]         = b.title;
        out["authorName"]    = b.authorName;
        out["publisherName"] = b.publisherName;
        out["genreIds"]      = b.genreIds;
        out["description"]   = b.description;
        out["coverColor"]    = b.coverColor;
        out["coverAccent"]   = b.coverAccent;
        out["price"]         = b.price;
        out["basePrice"]     = b.basePrice;
        out["priceText"]     = b.isFree ? QStringLiteral("Free")
                                        : QStringLiteral("$%1").arg(b.price, 0, 'f', 2);
        out["averageRating"] = b.averageRating;
        out["ratingCount"]   = b.ratingCount;
        out["totalSales"]    = b.totalSales;
        out["isFree"]        = b.isFree;
        out["status"]        = b.status.isEmpty() ? QStringLiteral("published") : b.status;
        out["active"]        = b.active;
        out["createdAtText"] = b.createdAt.toString(QStringLiteral("MMM d, yyyy"));
        // Reviews — pull from the store so the drawer shows real review data.
        QVariantList revs;
        for (const auto& r : m_store->reviewsForBook(bookId)) {
            QVariantMap rm;
            rm["id"]           = r.id;
            rm["username"]     = r.username;
            rm["displayName"]  = r.displayName;
            rm["rating"]       = r.rating;
            rm["comment"]      = r.comment;
            rm["createdAtText"]= r.createdAt.toString(QStringLiteral("MMM d, yyyy"));
            rm["helpfulCount"] = r.helpfulCount;
            rm["verifiedPurchase"] = r.verifiedPurchase;
            revs.append(rm);
        }
        out["reviews"] = revs;
        return out;
    }
    return out;
}

QString PublisherService::revenueTrend() const {
    // Compare the sum of the last 7 days of revenueSeries to the previous 7.
    // Returns "+X.X%" or "-X.X%" formatted as a string. Used by the dashboard
    // "vs last week" badge.
    QVariantList series = revenueSeries(14);
    if (series.size() < 14) return QStringLiteral("+0.0%");
    double recent = 0.0, prev = 0.0;
    for (int i = 0; i < 7; ++i) prev += series[i].toDouble();
    for (int i = 7; i < 14; ++i) recent += series[i].toDouble();
    if (prev <= 0.0) return QStringLiteral("+0.0%");
    double pct = (recent - prev) / prev * 100.0;
    return (pct >= 0 ? QStringLiteral("+") : QStringLiteral(""))
           + QString::number(pct, 'f', 1) + QStringLiteral("%");
}

// ---- Publisher profile (spec §3-1) ----

QVariantMap PublisherService::publisherProfile() const {
    QVariantMap out = m_profile;
    // Annotate with live catalog stats so the profile page reflects the
    // current state of the publisher's catalog.
    out["totalBooks"]    = totalBooks();
    out["totalRevenue"]  = totalRevenue();
    out["totalUnits"]    = totalUnitsSold();
    out["averageRating"] = averageRating();
    out["activeTitles"]  = activeTitleCount();
    return out;
}

bool PublisherService::updatePublisherProfile(const QString& publisherName,
                                               const QString& biography,
                                               const QString& website,
                                               const QString& email,
                                               const QString& taxId) {
    // Persist the editable fields to m_profile. A real backend would PUT
    // /publishers/me here; the mock just keeps the change in memory.
    if (!publisherName.trimmed().isEmpty()) m_profile["publisherName"] = publisherName.trimmed();
    m_profile["biography"] = biography;
    m_profile["website"]   = website;
    if (!email.trimmed().isEmpty()) m_profile["email"] = email.trimmed();
    m_profile["taxId"]     = taxId;
    m_profile["updatedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    emit profileChanged();
    return true;
}

// ---- Promotions ----

QVariantList PublisherService::promotions() const {
    QVariantList out;
    const QDate today = QDate::currentDate();
    for (const auto& p : m_promotions) {
        QVariantMap m;
        m["code"] = p.code;
        m["description"] = p.description;
        m["scope"] = p.scope;
        m["discount"] = p.discountPercent;
        m["uses"] = p.uses;
        m["cap"] = p.cap;
        m["startDate"] = p.startDate;
        m["endDate"] = p.endDate;

        // ---- Date-aware status computation (spec P0-5) ----
        // If startDate is in the future → "scheduled".
        // If endDate is in the past → "expired".
        // Otherwise → "active".
        QString computedStatus = p.status;
        if (!p.startDate.isEmpty()) {
            const QDate start = QDate::fromString(p.startDate, Qt::ISODate);
            if (start.isValid() && start > today) {
                computedStatus = QStringLiteral("scheduled");
            }
        }
        if (computedStatus != QStringLiteral("scheduled") && !p.endDate.isEmpty()) {
            const QDate end = QDate::fromString(p.endDate, Qt::ISODate);
            if (end.isValid() && end < today) {
                computedStatus = QStringLiteral("expired");
            }
        }
        if (computedStatus != QStringLiteral("scheduled") && computedStatus != QStringLiteral("expired")) {
            computedStatus = QStringLiteral("active");
        }
        m["status"] = computedStatus;

        // Convenience for the QML table: a single "period" string.
        if (p.startDate.isEmpty() && p.endDate.isEmpty()) {
            m["period"] = QStringLiteral("—");
        } else if (p.startDate.isEmpty()) {
            m["period"] = QStringLiteral("until %1").arg(p.endDate);
        } else if (p.endDate.isEmpty()) {
            m["period"] = QStringLiteral("from %1").arg(p.startDate);
        } else {
            m["period"] = QStringLiteral("%1 → %2").arg(p.startDate, p.endDate);
        }
        out.append(m);
    }
    return out;
}

bool PublisherService::addPromotion(const QString& code, const QString& description,
                                     int discountPercent, int cap,
                                     const QString& startDate, const QString& endDate) {
    if (code.trimmed().isEmpty() || discountPercent <= 0) return false;
    Promotion p;
    p.code = code.trimmed().toUpper();
    p.description = description.trimmed();
    p.scope = QStringLiteral("Catalog-wide");
    p.discountPercent = discountPercent;
    p.status = QStringLiteral("active");
    p.uses = 0;
    p.cap = cap > 0 ? cap : 1000;
    p.startDate = startDate.trimmed();
    p.endDate = endDate.trimmed();
    m_promotions.append(p);
    emit promotionsChanged();
    return true;
}

bool PublisherService::removePromotion(const QString& code) {
    // Codes are stored uppercased (see addPromotion), so uppercase the
    // input before comparing — otherwise "summer25" won't match "SUMMER25".
    const QString normalized = code.trimmed().toUpper();
    for (int i = 0; i < m_promotions.size(); ++i) {
        if (m_promotions[i].code == normalized) {
            m_promotions.removeAt(i);
            emit promotionsChanged();
            return true;
        }
    }
    return false;
}

// ---- Notifications ----

QVariantList PublisherService::publisherNotifications() const {
    return m_notifications;
}

void PublisherService::markAllNotificationsRead() {
    for (int i = 0; i < m_notifications.size(); ++i) {
        QVariantMap m = m_notifications[i].toMap();
        m["read"] = true;
        m_notifications[i] = m;
    }
    emit notificationsChanged();
}

void PublisherService::markNotificationRead(const QString& id, bool read) {
    for (int i = 0; i < m_notifications.size(); ++i) {
        QVariantMap m = m_notifications[i].toMap();
        if (m.value(QStringLiteral("id")).toString() == id) {
            m["read"] = read;
            m_notifications[i] = m;
            emit notificationsChanged();
            return;
        }
    }
}

void PublisherService::clearReadNotifications() {
    QVariantList kept;
    for (const auto& n : m_notifications) {
        if (!n.toMap()["read"].toBool()) kept.append(n);
    }
    m_notifications = kept;
    emit notificationsChanged();
}

// ---- Seeding ----

void PublisherService::_seedPromotions() {
    // { code, description, scope, status, startDate, endDate, discount%, uses, cap }
    m_promotions.append({QStringLiteral("SUMMER25"),  QStringLiteral("Summer reading — 25% off"),   QStringLiteral("Catalog-wide"),     QStringLiteral("active"),    QStringLiteral("2026-06-01"), QStringLiteral("2026-08-31"), 25, 412, 1000});
    m_promotions.append({QStringLiteral("NEWREADER"), QStringLiteral("First purchase — $5 off"),   QStringLiteral("First-time"),       QStringLiteral("active"),    QStringLiteral("2026-01-01"), QStringLiteral("2026-12-31"),  5, 184,  500});
    m_promotions.append({QStringLiteral("BUNDLE3"),   QStringLiteral("Buy 3, get 15% off"),        QStringLiteral("3+ books"),          QStringLiteral("active"),    QStringLiteral("2026-07-01"), QStringLiteral("2026-09-30"), 15,  89,  200});
    m_promotions.append({QStringLiteral("FLASH48"),   QStringLiteral("48-hour flash sale — 40% off"), QStringLiteral("Selected titles"), QStringLiteral("scheduled"), QStringLiteral("2026-07-15"), QStringLiteral("2026-07-17"), 40,   0,  250});
    m_promotions.append({QStringLiteral("SPRING20"),  QStringLiteral("Spring sale — 20% off"),     QStringLiteral("Catalog-wide"),     QStringLiteral("expired"),   QStringLiteral("2026-03-01"), QStringLiteral("2026-06-21"), 20, 738,  800});
    m_promotions.append({QStringLiteral("WELCOME10"), QStringLiteral("Welcome back — 10% off"),    QStringLiteral("Catalog-wide"),     QStringLiteral("expired"),   QStringLiteral("2026-04-01"), QStringLiteral("2026-05-31"), 10, 1024, 1000});
}

void PublisherService::_seedNotifications() {
    int nfid = 0;
    auto add = [this, &nfid](const QString& type, const QString& icon, const QString& title, const QString& body, const QString& time, bool read) {
        QVariantMap m;
        m["id"] = QStringLiteral("pnf-%1").arg(++nfid, 3, 10, QChar('0'));
        m["type"] = type; m["icon"] = icon; m["title"] = title; m["body"] = body; m["time"] = time; m["read"] = read;
        m_notifications.append(m);
    };
    add("success", "🛒", "Sales milestone reached", "<b>Atomic Habits</b> just crossed 5,000 lifetime sales. 🎉", "12m ago", false);
    add("info", "⭐", "New 5-star review", "<b>Alice R.</b> rated <b>The Midnight Library</b> 5 stars.", "1h ago", false);
    add("warning", "🚩", "Review flagged", "A review on <b>Deep Work</b> was flagged for moderation. Action needed.", "3h ago", false);
    add("info", "📢", "Promo performance", "<b>SUMMER25</b> has been redeemed 412 times — 41% of its cap.", "5h ago", true);
    add("success", "✅", "Publisher verified", "Your publisher account has been verified.", "Yesterday", true);
    add("info", "📖", "New title published", "<b>Quiet Rivers</b> is now live in the catalog.", "Yesterday", true);
    add("warning", "⚠️", "Low stock alert", "<b>Sapiens</b> is projected to sell out in 6 days.", "2 days ago", true);
    add("info", "📈", "Weekly report ready", "Your weekly sales report is ready to download.", "3 days ago", true);
}

// ---- Profile seeding (spec §3-1) ----
//   Seeds the publisher's editable profile fields. The catalog stats
//   (totalBooks / totalRevenue / etc.) are computed on-the-fly by
//   publisherProfile() so they always reflect the current catalog state.

void PublisherService::_seedProfile() {
    m_profile["publisherId"]   = QStringLiteral("pub_self");
    m_profile["publisherName"] = QStringLiteral("Pinecrest Press");
    m_profile["biography"]     = QStringLiteral("Independent publisher of literary fiction, memoirs, and contemporary non-fiction. Founded in 2018, based in Portland, OR.");
    m_profile["website"]       = QStringLiteral("https://pinecrestpress.example.com");
    m_profile["email"]         = QStringLiteral("contact@pinecrestpress.example.com");
    m_profile["taxId"]         = QStringLiteral("XX-XXX9876");
    m_profile["joinedAt"]      = QStringLiteral("Feb 2024");
    m_profile["verified"]      = true;
    m_profile["plan"]          = QStringLiteral("Publisher Pro");
    m_profile["avatarColor"]   = QStringLiteral("#1A73E8");
    m_profile["country"]       = QStringLiteral("United States");
    m_profile["updatedAt"]     = QDateTime::currentDateTime().toString(Qt::ISODate);
}

int PublisherService::totalBooks() const {
    if (!m_store) return 0;
    return m_store->books().size();
}

} // namespace bookclub::client
