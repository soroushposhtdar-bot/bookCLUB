// src/server/StudySessionManager.h
#pragma once

#include <QObject>
#include <QMap>
#include <QList>
#include <QMutex>
#include <QPointer>
#include <QJsonArray>

#include "common/Models/StudySession.h"
#include "common/Network/Message.h"
#include "src/server/ConnectionManager.h"

namespace bookclub::server {

class ConnectionManager;
class ClientConnection;

class StudySessionManager : public QObject {
    Q_OBJECT
public:
    explicit StudySessionManager(ConnectionManager* connectionManager, QObject* parent = nullptr);
    ~StudySessionManager() override;

    // --- Session Management ---
    common::StudySession* createSession(const QString& bookId, const QString& hostUserId);
    bool joinSession(const QString& sessionId, const QString& userId);
    bool leaveSession(const QString& sessionId, const QString& userId);
    bool closeSession(const QString& sessionId);

    // --- Page Sync ---
    void syncPage(const QString& sessionId, const QString& userId, int page, double zoom);
    void syncState(const QString& sessionId, const QString& userId);

    // --- Queries ---
    common::StudySession* getSession(const QString& sessionId) const;
    QList<common::StudySession*> getActiveSessionsForBook(const QString& bookId) const;
    QList<common::StudySession*> getSessionsForUser(const QString& userId) const;

signals:
    void sessionCreated(const QString& sessionId);
    void sessionJoined(const QString& sessionId, const QString& userId);
    void sessionLeft(const QString& sessionId, const QString& userId);
    void sessionClosed(const QString& sessionId);
    void pageUpdated(const QString& sessionId, const QString& userId, int page);

private:
    void broadcastToSession(const QString& sessionId, const common::Message& message);
    void broadcastToSession(const QString& sessionId, const QJsonObject& payload);
    common::StudySession* findSession(const QString& sessionId) const;

    QMap<QString, common::StudySession*> m_sessions;
    QMap<QString, QString> m_userToSessionMap; // userId -> sessionId
    QPointer<ConnectionManager> m_connectionManager;
    mutable QMutex m_mutex;
};

} // namespace bookclub::server
