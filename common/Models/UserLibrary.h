#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "common/Models/LibraryShelf.h"

namespace bookclub::common {

class UserLibrary : public QObject {
    Q_OBJECT
public:
    explicit UserLibrary(QObject* parent = nullptr);
    ~UserLibrary() override = default;

    const QString& userId() const;
    const QStringList& purchasedBookIds() const;
    const QStringList& savedBookIds() const;
    const QVector<LibraryShelf*>& shelves() const;
    const QString& lastOpenedBookId() const;
    const QString& lastOpenedPage() const;

    void setUserId(const QString& userId);
    void setPurchasedBookIds(const QStringList& ids);
    void setSavedBookIds(const QStringList& ids);
    void setShelves(const QVector<LibraryShelf*>& shelves);
    void setLastOpenedBookId(const QString& bookId);
    void setLastOpenedPage(const QString& page);
    void addPurchasedBook(const QString& bookId);
    void addSavedBook(const QString& bookId);
    void removeSavedBook(const QString& bookId);
    LibraryShelf* createShelf(const QString& name, const QString& description = {});
    bool removeShelf(const QString& shelfId);

signals:
    void libraryChanged();

private:
    QString m_userId;
    QStringList m_purchasedBookIds;
    QStringList m_savedBookIds;
    QVector<LibraryShelf*> m_shelves;
    QString m_lastOpenedBookId;
    QString m_lastOpenedPage;
};

} // namespace bookclub::common
