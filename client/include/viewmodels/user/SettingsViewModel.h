// =============================================================================
//  SettingsViewModel.h
// =============================================================================
//  Multi-section settings page VM. Holds the active section, every setting
//  field, and save indicators.
// =============================================================================
#ifndef SETTINGSVIEWMODEL_H
#define SETTINGSVIEWMODEL_H

#include <QObject>
#include <QQmlEngine>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class UserService;

class SettingsViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(UserService* userService READ userService WRITE setUserService NOTIFY userServiceChanged)

    Q_PROPERTY(int activeSection READ activeSection WRITE setActiveSection NOTIFY activeSectionChanged)

    // ----- General -----
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY generalChanged)
    Q_PROPERTY(bool reduceAnimations READ reduceAnimations WRITE setReduceAnimations NOTIFY generalChanged)

    // ----- Appearance -----
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY appearanceChanged)              // "light" | "dark" | "auto"
    Q_PROPERTY(QString accentName READ accentName WRITE setAccentName NOTIFY appearanceChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY appearanceChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY appearanceChanged)

    // ----- Notifications -----
    Q_PROPERTY(bool notifNewBooks READ notifNewBooks WRITE setNotifNewBooks NOTIFY notifChanged)
    Q_PROPERTY(bool notifDiscounts READ notifDiscounts WRITE setNotifDiscounts NOTIFY notifChanged)
    Q_PROPERTY(bool notifSales READ notifSales WRITE setNotifSales NOTIFY notifChanged)
    Q_PROPERTY(bool notifReviews READ notifReviews WRITE setNotifReviews NOTIFY notifChanged)
    Q_PROPERTY(bool notifEmailDigest READ notifEmailDigest WRITE setNotifEmailDigest NOTIFY notifChanged)

    // ----- Reading -----
    Q_PROPERTY(QString readerTheme READ readerTheme WRITE setReaderTheme NOTIFY readingChanged)  // "light" | "sepia" | "dark"
    Q_PROPERTY(int readerFontSize READ readerFontSize WRITE setReaderFontSize NOTIFY readingChanged)
    Q_PROPERTY(bool readerSync READ readerSync WRITE setReaderSync NOTIFY readingChanged)
    Q_PROPERTY(QString downloadLocation READ downloadLocation WRITE setDownloadLocation NOTIFY readingChanged)
    Q_PROPERTY(bool autoDownload READ autoDownload WRITE setAutoDownload NOTIFY readingChanged)

    // ----- Privacy -----
    Q_PROPERTY(bool shareReading READ shareReading WRITE setShareReading NOTIFY privacyChanged)
    Q_PROPERTY(bool shareWishlist READ shareWishlist WRITE setShareWishlist NOTIFY privacyChanged)
    Q_PROPERTY(bool personalAds READ personalAds WRITE setPersonalAds NOTIFY privacyChanged)

    // ----- Save state -----
    Q_PROPERTY(bool saving READ saving NOTIFY savingChanged)
    Q_PROPERTY(bool saved READ saved NOTIFY savingChanged)

    // ----- Storage / Cache -----
    //   cacheSize + storageUsed are now NOTIFIER-bound (not CONSTANT) so they
    //   update after clearCache() is called. storagePct is a 0..1 qreal used
    //   by the SettingsPage storage bar.
    Q_PROPERTY(QString cacheSize READ cacheSize NOTIFY storageChanged)
    Q_PROPERTY(QString storageUsed READ storageUsed NOTIFY storageChanged)
    Q_PROPERTY(qreal storagePct READ storagePct NOTIFY storageChanged)

public:
    explicit SettingsViewModel(QObject* parent = nullptr);

    UserService* userService() const { return m_userService; }
    void setUserService(UserService* s);

    int activeSection() const { return m_activeSection; }
    void setActiveSection(int v) { if (m_activeSection != v) { m_activeSection = v; emit activeSectionChanged(); } }

    // Getters + setters for every property above
    QString language() const { return m_language; }
    void setLanguage(const QString& v) { if (m_language != v) { m_language = v; emit generalChanged(); } }
    bool reduceAnimations() const { return m_reduceAnimations; }
    void setReduceAnimations(bool v) { if (m_reduceAnimations != v) { m_reduceAnimations = v; emit generalChanged(); } }

    QString theme() const { return m_theme; }
    void setTheme(const QString& v) { if (m_theme != v) { m_theme = v; emit appearanceChanged(); } }
    QString accentName() const { return m_accentName; }
    void setAccentName(const QString& v) { if (m_accentName != v) { m_accentName = v; emit appearanceChanged(); } }
    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString& v) { if (m_fontFamily != v) { m_fontFamily = v; emit appearanceChanged(); } }
    int fontSize() const { return m_fontSize; }
    void setFontSize(int v) { if (m_fontSize != v) { m_fontSize = v; emit appearanceChanged(); } }

    bool notifNewBooks() const { return m_notifNewBooks; }
    void setNotifNewBooks(bool v) { if (m_notifNewBooks != v) { m_notifNewBooks = v; emit notifChanged(); } }
    bool notifDiscounts() const { return m_notifDiscounts; }
    void setNotifDiscounts(bool v) { if (m_notifDiscounts != v) { m_notifDiscounts = v; emit notifChanged(); } }
    bool notifSales() const { return m_notifSales; }
    void setNotifSales(bool v) { if (m_notifSales != v) { m_notifSales = v; emit notifChanged(); } }
    bool notifReviews() const { return m_notifReviews; }
    void setNotifReviews(bool v) { if (m_notifReviews != v) { m_notifReviews = v; emit notifChanged(); } }
    bool notifEmailDigest() const { return m_notifEmailDigest; }
    void setNotifEmailDigest(bool v) { if (m_notifEmailDigest != v) { m_notifEmailDigest = v; emit notifChanged(); } }

    QString readerTheme() const { return m_readerTheme; }
    void setReaderTheme(const QString& v) { if (m_readerTheme != v) { m_readerTheme = v; emit readingChanged(); } }
    int readerFontSize() const { return m_readerFontSize; }
    void setReaderFontSize(int v) { if (m_readerFontSize != v) { m_readerFontSize = v; emit readingChanged(); } }
    bool readerSync() const { return m_readerSync; }
    void setReaderSync(bool v) { if (m_readerSync != v) { m_readerSync = v; emit readingChanged(); } }
    QString downloadLocation() const { return m_downloadLocation; }
    void setDownloadLocation(const QString& v) { if (m_downloadLocation != v) { m_downloadLocation = v; emit readingChanged(); } }
    bool autoDownload() const { return m_autoDownload; }
    void setAutoDownload(bool v) { if (m_autoDownload != v) { m_autoDownload = v; emit readingChanged(); } }

    bool shareReading() const { return m_shareReading; }
    void setShareReading(bool v) { if (m_shareReading != v) { m_shareReading = v; emit privacyChanged(); } }
    bool shareWishlist() const { return m_shareWishlist; }
    void setShareWishlist(bool v) { if (m_shareWishlist != v) { m_shareWishlist = v; emit privacyChanged(); } }
    bool personalAds() const { return m_personalAds; }
    void setPersonalAds(bool v) { if (m_personalAds != v) { m_personalAds = v; emit privacyChanged(); } }

    bool saving() const { return m_saving; }
    bool saved() const { return m_saved; }

    QString cacheSize() const { return m_cacheSize; }
    QString storageUsed() const { return m_storageUsed; }
    qreal storagePct() const { return m_storagePct; }

    Q_INVOKABLE void save();
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE void resetToDefaults();

signals:
    void userServiceChanged();
    void activeSectionChanged();
    void generalChanged();
    void appearanceChanged();
    void notifChanged();
    void readingChanged();
    void privacyChanged();
    void savingChanged();
    void cacheCleared();
    void storageChanged();

protected:
    void onAsyncReady() override;

private:
    UserService* m_userService = nullptr;
    int m_activeSection = 0;   // 0=General,1=Appearance,2=Notifications,3=Privacy,4=Reading,5=Account,6=About

    QString m_language = "English";
    bool m_reduceAnimations = false;

    QString m_theme = "light";
    QString m_accentName = "blue";
    QString m_fontFamily = "Inter";
    int m_fontSize = 14;

    bool m_notifNewBooks = true;
    bool m_notifDiscounts = true;
    bool m_notifSales = true;
    bool m_notifReviews = true;
    bool m_notifEmailDigest = false;

    QString m_readerTheme = "light";
    int m_readerFontSize = 16;
    bool m_readerSync = true;
    QString m_downloadLocation = "~/BookClub/downloads";
    bool m_autoDownload = false;

    bool m_shareReading = false;
    bool m_shareWishlist = true;
    bool m_personalAds = true;

    bool m_saving = false;
    bool m_saved = false;

    // Storage / cache state — mutable so clearCache() can update them.
    QString m_cacheSize = QStringLiteral("48.2 MB");
    QString m_storageUsed = QStringLiteral("1.4 GB of 5 GB");
    qreal m_storagePct = 0.28;   // 1.4 / 5.0
};

} // namespace bookclub::client

#endif // SETTINGSVIEWMODEL_H
