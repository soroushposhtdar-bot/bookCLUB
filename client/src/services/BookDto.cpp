#include "services/BookDto.h"
#include <QJsonArray>
#include <QLocale>

namespace bookclub::client {

void BookDto::fromJson(const QJsonObject& j) {
    m_id            = j.value("id").toString();
    m_title         = j.value("title").toString();
    m_authorName    = j.value("authorName").toString();
    m_publisherId   = j.value("publisherId").toString();
    m_publisherName = j.value("publisherName").toString();
    m_description   = j.value("description").toString();
    // v9: prefer `coverImagePath` (server canonical), fall back to
    // `coverImage` (publisher QML editor's field name).
    m_coverImage    = j.value("coverImagePath").toString();
    if (m_coverImage.isEmpty()) m_coverImage = j.value("coverImage").toString();
    // v9: load coverColor/coverAccent so the publisher catalog and the
    // user's home/search page show the publisher's chosen colors.
    m_coverColor    = j.value("coverColor").toString();
    m_coverAccent   = j.value("coverAccent").toString();
    m_pdfFilePath   = j.value("pdfFilePath").toString();
    m_basePrice     = j.value("basePrice").toDouble();
    m_discountValue = j.value("discountValue").toDouble();
    m_price         = j.value("price").toDouble();
    if (m_price == 0) m_price = m_basePrice - m_discountValue;
    m_averageRating = j.value("averageRating").toDouble();
    m_ratingCount   = j.value("ratingCount").toInt();
    m_totalSales    = j.value("totalSales").toInt();
    m_isFree        = (m_price == 0.0);
    m_active        = j.value("isActive").toBool(true);
    // v15: read the `purchased` flag that the server now includes in
    // GetBookDetails responses. Falls back to false when the server
    // doesn't send it (e.g. older server, or home-section book lists).
    m_purchased     = j.value("purchased").toBool(false);
    m_createdAt     = QDateTime::fromString(j.value("createdAt").toString(), Qt::ISODate);
    m_createdAtText = m_createdAt.toString(QStringLiteral("MMM d, yyyy"));

    const QJsonArray ga = j.value("genreIds").toArray();
    for (const auto& v : ga) m_genreIds.append(v.toString());
}

int BookDto::discountPercent() const {
    if (m_basePrice <= 0) return 0;
    return static_cast<int>(m_discountValue / m_basePrice * 100.0);
}

QString BookDto::priceText() const {
    if (m_isFree) return QStringLiteral("Free");
    return QLocale().toString(m_price, 'f', 2);
}

QString BookDto::basePriceText() const {
    return QLocale().toString(m_basePrice, 'f', 2);
}

} // namespace bookclub::client
