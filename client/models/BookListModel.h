#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QVariant>
#include <QHash>
#include <QByteArray>
#include <QModelIndex>

namespace bookclub::client {

class BookListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        AuthorRole,
        PublisherRole,
        GenreIdsRole,
        PriceRole,
        DiscountRole,
        AverageRatingRole,
        CoverRole,
        PdfRole,
        ActiveRole
    };
    Q_ENUM(Roles)

    explicit BookListModel(QObject* parent = nullptr);
    ~BookListModel() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setBookIds(const QStringList& ids);
    void setTitle(const QString& title);

private:
    QStringList m_bookIds;
    QString m_title;
};

} // namespace bookclub::client
