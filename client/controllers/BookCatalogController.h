#pragma once

#include <QObject>
#include <QStringList>

namespace bookclub::client {

class BookCatalogController : public QObject {
    Q_OBJECT
public:
    explicit BookCatalogController(QObject* parent = nullptr);
    ~BookCatalogController() override = default;

    void loadHomeSections();
    void search(const QString& keyword);
    void searchByAuthor(const QString& author);
    void searchByPublisher(const QString& publisher);
    void searchByGenre(const QString& genreId);
    void loadBookDetails(const QString& bookId);

signals:
    void featuredBooksChanged();
    void newBooksChanged();
    void bestSellersChanged();
    void freeBooksChanged();
    void searchResultsChanged();
    void bookDetailsLoaded(const QString& bookId);
    void errorOccurred(const QString& message);

private:
    QString m_lastKeyword;
    QStringList m_lastGenres;
};

} // namespace bookclub::client
