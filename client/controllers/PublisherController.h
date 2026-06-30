#pragma once

#include <QObject>
#include <QString>

namespace bookclub::client {

class PublisherController : public QObject {
    Q_OBJECT
public:
    explicit PublisherController(QObject* parent = nullptr);
    ~PublisherController() override = default;

    void loadMyBooks();
    void createBook();
    void updateBook(const QString& bookId);
    void deactivateBook(const QString& bookId);
    void activateBook(const QString& bookId);
    void applyTimedDiscount(const QString& bookId);
    void loadAnalytics();

signals:
    void bookListChanged();
    void analyticsChanged();
    void errorOccurred(const QString& message);

};

} // namespace bookclub::client
