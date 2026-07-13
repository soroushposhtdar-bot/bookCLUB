#include "common/Models/StudySession.h"

namespace bookclub::common {

StudySession::StudySession(QObject* parent) : QObject(parent) {}

StudySession::StudySession(const QString& id, QObject* parent)
    : QObject(parent), m_id(id) {}

const QString& StudySession::id() const { return m_id; }
const QString& StudySession::bookId() const { return m_bookId; }
const QString& StudySession::hostUserId() const { return m_hostUserId; }
const QStringList& StudySession::participantUserIds() const { return m_participantUserIds; }
StudySessionState StudySession::state() const { return m_state; }
const QDateTime& StudySession::createdAt() const { return m_createdAt; }
int StudySession::currentPage() const { return m_currentPage; }
double StudySession::zoomLevel() const { return m_zoomLevel; }
bool StudySession::synced() const { return m_synced; }

void StudySession::setId(const QString& id) { m_id = id; }
void StudySession::setBookId(const QString& bookId) {
    if (m_bookId != bookId) {
        m_bookId = bookId;
        emit sessionChanged();
    }
}
void StudySession::setHostUserId(const QString& hostUserId) { m_hostUserId = hostUserId; }

void StudySession::setParticipantUserIds(const QStringList& ids) {
    if (m_participantUserIds != ids) {
        m_participantUserIds = ids;
        emit sessionChanged();
    }
}

void StudySession::setState(StudySessionState state) {
    if (m_state != state) {
        m_state = state;
        emit sessionChanged();
    }
}

void StudySession::setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }

void StudySession::setCurrentPage(int page) {
    if (m_currentPage != page) {
        m_currentPage = page;
        emit pageChanged(page);
        emit sessionChanged();
    }
}

void StudySession::setZoomLevel(double zoom) {
    if (qFuzzyCompare(m_zoomLevel, zoom)) return;
    m_zoomLevel = zoom;
    emit zoomChanged(zoom);
    emit sessionChanged();
}

void StudySession::setSynced(bool synced) {
    if (m_synced != synced) {
        m_synced = synced;
        emit sessionChanged();
    }
}

void StudySession::addParticipant(const QString& userId) {
    if (!m_participantUserIds.contains(userId)) {
        m_participantUserIds.append(userId);
        emit participantJoined(userId);
        emit sessionChanged();
    }
}

void StudySession::removeParticipant(const QString& userId) {
    if (m_participantUserIds.removeOne(userId)) {
        emit participantLeft(userId);
        emit sessionChanged();
    }
}

} // namespace bookclub::common
