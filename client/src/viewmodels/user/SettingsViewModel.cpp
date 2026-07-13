// =============================================================================
//  SettingsViewModel.cpp
// =============================================================================
#include "viewmodels/user/SettingsViewModel.h"

namespace bookclub::client {

SettingsViewModel::SettingsViewModel(QObject* parent)
    : UserViewModelBase(parent)
{}

void SettingsViewModel::setUserService(UserService* s) {
    if (m_userService == s) return;
    m_userService = s;
    emit userServiceChanged();
}

void SettingsViewModel::save() {
    if (m_saving) return;
    m_saving = true;
    m_saved = false;
    emit savingChanged();
    beginAsync(500);
}

void SettingsViewModel::onAsyncReady() {
    m_saving = false;
    m_saved = true;
    emit savingChanged();
    // Reset "saved" indicator after 2.5s
    QTimer::singleShot(2500, this, [this](){
        m_saved = false;
        emit savingChanged();
    });
    finishAsync();
}

void SettingsViewModel::clearCache() {
    // Simulate clearing the download cache: drop the cache size to ~0 and
    // adjust the storage-used figure + bar percentage accordingly.
    m_cacheSize = QStringLiteral("0 MB");
    // Storage drops by the previous cache size (48.2 MB ≈ 0.048 GB).
    m_storageUsed = QStringLiteral("1.35 GB of 5 GB");
    m_storagePct = 0.27;   // 1.35 / 5.0
    emit cacheCleared();
    emit storageChanged();
}

void SettingsViewModel::resetToDefaults() {
    m_language = "English";
    m_reduceAnimations = false;
    m_theme = "light";
    m_accentName = "blue";
    m_fontFamily = "Inter";
    m_fontSize = 14;
    m_notifNewBooks = true;
    m_notifDiscounts = true;
    m_notifSales = true;
    m_notifReviews = true;
    m_notifEmailDigest = false;
    m_readerTheme = "light";
    m_readerFontSize = 16;
    m_readerSync = true;
    m_downloadLocation = "~/BookClub/downloads";
    m_autoDownload = false;
    m_shareReading = false;
    m_shareWishlist = true;
    m_personalAds = true;

    emit generalChanged();
    emit appearanceChanged();
    emit notifChanged();
    emit readingChanged();
    emit privacyChanged();
}

} // namespace bookclub::client
