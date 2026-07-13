// =============================================================================
//  HomeViewModel.h
// =============================================================================
//  MVVM view-model for the User Dashboard / Home page.
//
//  Loads every section the home page needs across two refresh waves:
//      Wave 1 (priority): greeting, continue reading, recommended, new releases
//      Wave 2 (deferred): bestsellers, free, trending, editor's picks,
//                         discounted, recently viewed, featured publishers,
//                         popular genres, because-you-read
//
//  All sections are exposed as Q_PROPERTY lists of BookDto* (or strings) so
//  the QML view can bind a SectionCarousel directly to each one. The
//  `loadingWave1` / `loadingWave2` flags drive skeleton placeholders.
// =============================================================================
#ifndef HOMEVIEWMODEL_H
#define HOMEVIEWMODEL_H

#include <QObject>
#include <QList>
#include <QQmlEngine>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class BookService;
class UserService;
class BookDto;

class HomeViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(BookService* bookService READ bookService WRITE setBookService NOTIFY bookServiceChanged)
    Q_PROPERTY(UserService* userService READ userService WRITE setUserService NOTIFY userServiceChanged)

    // Section state — each emits sectionDataChanged() when refreshed
    Q_PROPERTY(QList<QObject*> continueReading READ continueReading NOTIFY dataChanged)
    Q_PROPERTY(QList<QObject*> recommended  READ recommended  NOTIFY dataChanged)
    Q_PROPERTY(QList<QObject*> newReleases  READ newReleases  NOTIFY dataChanged)
    Q_PROPERTY(QList<QObject*> bestsellers  READ bestsellers  NOTIFY dataChanged)
    Q_PROPERTY(QList<QObject*> freeBooks    READ freeBooks    NOTIFY dataChanged)
    Q_PROPERTY(QList<QObject*> trending     READ trending     NOTIFY dataChanged)
    Q_PROPERTY(QList<QObject*> editorsPicks READ editorsPicks NOTIFY dataChanged)
    Q_PROPERTY(QList<QObject*> discounted   READ discounted   NOTIFY dataChanged)
    Q_PROPERTY(QList<QObject*> recentlyViewed READ recentlyViewed NOTIFY dataChanged)
    Q_PROPERTY(QList<QObject*> newArrivals  READ newArrivals  NOTIFY dataChanged)
    Q_PROPERTY(QList<QObject*> becauseYouRead READ becauseYouRead NOTIFY dataChanged)
    Q_PROPERTY(QStringList popularGenres READ popularGenres NOTIFY dataChanged)
    Q_PROPERTY(QStringList featuredPublishers READ featuredPublishers NOTIFY dataChanged)

    // Loading wave flags (drive skeleton placeholders in the QML)
    Q_PROPERTY(bool loadingWave1 READ loadingWave1 NOTIFY loadingWave1Changed)
    Q_PROPERTY(bool loadingWave2 READ loadingWave2 NOTIFY loadingWave2Changed)

    Q_PROPERTY(QString greeting READ greeting NOTIFY dataChanged)

public:
    explicit HomeViewModel(QObject* parent = nullptr);

    BookService* bookService() const { return m_bookService; }
    UserService* userService() const { return m_userService; }
    void setBookService(BookService* s);
    void setUserService(UserService* s);

    QList<QObject*> continueReading() const { return m_continueReading; }
    QList<QObject*> recommended() const { return m_recommended; }
    QList<QObject*> newReleases() const { return m_newReleases; }
    QList<QObject*> bestsellers() const { return m_bestsellers; }
    QList<QObject*> freeBooks()   const { return m_freeBooks; }
    QList<QObject*> trending()    const { return m_trending; }
    QList<QObject*> editorsPicks() const { return m_editorsPicks; }
    QList<QObject*> discounted()  const { return m_discounted; }
    QList<QObject*> recentlyViewed() const { return m_recentlyViewed; }
    QList<QObject*> newArrivals() const { return m_newArrivals; }
    QList<QObject*> becauseYouRead() const { return m_becauseYouRead; }
    QStringList     popularGenres() const { return m_popularGenres; }
    QStringList     featuredPublishers() const { return m_featuredPublishers; }

    bool loadingWave1() const { return m_loadingWave1; }
    bool loadingWave2() const { return m_loadingWave2; }
    QString         greeting() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void toggleWishlist(const QString& bookId);

signals:
    void bookServiceChanged();
    void userServiceChanged();
    void dataChanged();
    void loadingWave1Changed(bool loading);
    void loadingWave2Changed(bool loading);
    void wishlistToggled(const QString& bookId, bool inWishlist);

private:
    void _loadWave1();
    void _loadWave2();
    void _setLoadingWave1(bool v);
    void _setLoadingWave2(bool v);

    BookService* m_bookService = nullptr;
    UserService* m_userService = nullptr;

    QList<QObject*> m_continueReading;
    QList<QObject*> m_recommended;
    QList<QObject*> m_newReleases;
    QList<QObject*> m_bestsellers;
    QList<QObject*> m_freeBooks;
    QList<QObject*> m_trending;
    QList<QObject*> m_editorsPicks;
    QList<QObject*> m_discounted;
    QList<QObject*> m_recentlyViewed;
    QList<QObject*> m_newArrivals;
    QList<QObject*> m_becauseYouRead;
    QStringList     m_popularGenres;
    QStringList     m_featuredPublishers;

    bool m_loadingWave1 = false;
    bool m_loadingWave2 = false;

    QTimer m_wave2Timer;
};

} // namespace bookclub::client

#endif // HOMEVIEWMODEL_H
