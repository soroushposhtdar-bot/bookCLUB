#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QQmlEngine>
#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::client {

class ShelfDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id           READ id           CONSTANT)
    Q_PROPERTY(QString name         READ name         CONSTANT)
    Q_PROPERTY(QString description  READ description  CONSTANT)
    Q_PROPERTY(QStringList bookIds  READ bookIds      CONSTANT)
    Q_PROPERTY(int      bookCount   READ bookCount    CONSTANT)
    Q_PROPERTY(QString color        READ color        CONSTANT)
    Q_PROPERTY(bool     favorite    READ favorite     CONSTANT)
    Q_PROPERTY(bool     isPrivate   READ isPrivate    CONSTANT)
    Q_PROPERTY(bool     isSystemShelf READ isSystemShelf CONSTANT)

public:
    explicit ShelfDto(QObject* parent = nullptr) : QObject(parent) {}

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    QStringList bookIds() const { return m_bookIds; }
    int bookCount() const { return m_bookIds.size(); }
    QString color() const { return m_color; }
    bool favorite() const { return m_favorite; }
    bool isPrivate() const { return m_isPrivate; }
    bool isSystemShelf() const { return m_isSystemShelf; }

    void fromJson(const QJsonObject& j) {
        m_id = j.value("id").toString();
        m_name = j.value("name").toString();
        m_description = j.value("description").toString();
        m_color = j.value("color").toString("#1A73E8");
        m_favorite = j.value("favorite").toBool(false);
        m_isPrivate = j.value("isPrivate").toBool(false);
        m_isSystemShelf = j.value("isSystemShelf").toBool(false);
        const QJsonArray arr = j.value("bookIds").toArray();
        for (const auto& v : arr) m_bookIds.append(v.toString());
    }

private:
    QString m_id, m_name, m_description, m_color;
    QStringList m_bookIds;
    bool m_favorite = false, m_isPrivate = false, m_isSystemShelf = false;
};

// -----------------------------------------------------------------------------

class PurchaseDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString orderId      READ orderId      CONSTANT)
    Q_PROPERTY(QString dateText     READ dateText     CONSTANT)
    Q_PROPERTY(QString relativeDate READ relativeDate CONSTANT)
    Q_PROPERTY(QStringList bookIds  READ bookIds      CONSTANT)
    Q_PROPERTY(double   total       READ total        CONSTANT)
    Q_PROPERTY(QString  totalText   READ totalText    CONSTANT)
    Q_PROPERTY(QString  titlesSummary READ titlesSummary CONSTANT)
    Q_PROPERTY(int      itemCount   READ itemCount    CONSTANT)
    Q_PROPERTY(QString  discountText READ discountText CONSTANT)

public:
    explicit PurchaseDto(QObject* parent = nullptr) : QObject(parent) {}

    QString orderId() const { return m_orderId; }
    QString dateText() const { return m_dateText; }
    QString relativeDate() const { return m_relativeDate; }
    QStringList bookIds() const { return m_bookIds; }
    double total() const { return m_total; }
    QString totalText() const { return QString::number(m_total, 'f', 2); }
    QString titlesSummary() const { return m_titlesSummary; }
    int itemCount() const { return m_bookIds.size(); }
    QString discountText() const { return m_discountText; }

    void fromJson(const QJsonObject& j) {
        m_orderId = j.value("orderId").toString();
        m_dateText = j.value("date").toString();
        m_relativeDate = j.value("relativeDate").toString(m_dateText);
        m_total = j.value("total").toDouble();
        m_titlesSummary = j.value("titlesSummary").toString();
        m_discountText = j.value("discountText").toString(QStringLiteral("No discount"));
        const QJsonArray arr = j.value("bookIds").toArray();
        for (const auto& v : arr) m_bookIds.append(v.toString());
    }

private:
    QString m_orderId, m_dateText, m_relativeDate, m_titlesSummary, m_discountText;
    QStringList m_bookIds;
    double m_total = 0;
};

// -----------------------------------------------------------------------------

class NotificationDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id        READ id        CONSTANT)
    Q_PROPERTY(QString title     READ title     CONSTANT)
    Q_PROPERTY(QString message   READ message   CONSTANT)
    Q_PROPERTY(QString body      READ message   CONSTANT)  // alias used by NotificationItem.qml
    Q_PROPERTY(int     type      READ type      CONSTANT)
    Q_PROPERTY(int     state     READ state     CONSTANT)
    Q_PROPERTY(QString createdAt READ createdAt CONSTANT)
    Q_PROPERTY(QString category  READ category  CONSTANT)
    Q_PROPERTY(bool    isRead    READ isRead    WRITE setRead NOTIFY readChanged)
    Q_PROPERTY(bool    read      READ isRead    WRITE setRead NOTIFY readChanged)  // alias used by NotificationItem.qml
    Q_PROPERTY(QString timeText  READ timeText  CONSTANT)
    Q_PROPERTY(QString relativeTime READ timeText CONSTANT)  // alias used by NotificationItem.qml
    Q_PROPERTY(QString bookId    READ bookId    CONSTANT)
    Q_PROPERTY(QString relatedEntityId READ bookId CONSTANT)
    Q_PROPERTY(QString iconName  READ iconName  CONSTANT)
    Q_PROPERTY(QString accentColor READ accentColor CONSTANT)

public:
    explicit NotificationDto(QObject* parent = nullptr) : QObject(parent) {}

    QString id() const { return m_id; }
    QString title() const { return m_title; }
    QString message() const { return m_message; }
    int type() const { return m_type; }
    int state() const { return m_state; }
    QString createdAt() const { return m_timeText; }
    QString category() const {
        switch (m_type) {
            case 1: return "recommendation";
            case 2: return "discount";
            case 3: return "purchase";
            case 4: return "review";
            case 5: return "publisher";
            case 6: return "system";
            default: return "system";
        }
    }
    bool isRead() const { return m_isRead; }
    void setRead(bool v) { if (m_isRead != v) { m_isRead = v; emit readChanged(); } }
    QString timeText() const { return m_timeText; }
    QString bookId() const { return m_relatedEntityId; }
    QString iconName() const {
        switch (m_type) {
            case 1: return "auto_stories";   // recommendation — new book in favourite genre
            case 2: return "local_offer";     // discount
            case 3: return "shopping_cart";   // purchase / sale
            case 4: return "rate_review";     // review
            case 5: return "campaign";        // publisher
            case 6: return "info_outline";    // system
            default: return "notifications";
        }
    }
    QString accentColor() const {
        switch (m_type) {
            case 1: return QStringLiteral("#1A73E8");  // blue — recommendation
            case 2: return QStringLiteral("#1E8E3E");  // green — discount
            case 3: return QStringLiteral("#F29900");  // orange — purchase
            case 4: return QStringLiteral("#9C27B0");  // purple — review
            case 5: return QStringLiteral("#1A73E8");  // blue — publisher
            case 6: return QStringLiteral("#5F6368");  // grey — system
            default: return QStringLiteral("#1A73E8");
        }
    }

    void fromJson(const QJsonObject& j) {
        m_id = j.value("id").toString();
        m_title = j.value("title").toString();
        m_message = j.value("message").toString();
        m_type = j.value("type").toInt();
        m_state = j.value("state").toInt();
        // Use setRead() so the readChanged signal is emitted if the QML
        // bindings need to update (previously m_isRead was set directly,
        // which silently skipped the signal).
        setRead(j.value("isRead").toBool(j.value("state").toInt() >= 1));
        m_timeText = j.value("createdAt").toString();
        m_relatedEntityId = j.value("relatedEntityId").toString();
    }

signals:
    void readChanged();

private:
    QString m_id, m_title, m_message, m_timeText, m_relatedEntityId;
    int m_type = 0, m_state = 0;
    bool m_isRead = false;
};

} // namespace bookclub::client
