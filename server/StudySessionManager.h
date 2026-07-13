#pragma once

#include <QObject>
#include <QString>

namespace bookclub::common {
class StudySession;
}

namespace bookclub::server {

class StudySessionManager : public QObject {
    Q_OBJECT
public:
    explicit StudySessionManager(QObject* parent = nullptr);
    ~StudySessionManager() override = default;

    bookclub::common::StudySession* createSession(const QString& hostUserId, const QString& bookId);
    bool joinSession(const QString& sessionId, const QString& userId);
    bool leaveSession(const QString& sessionId, const QString& userId);
    bool updatePage(const QString& sessionId, int page);
    bool updateZoom(const QString& sessionId, double zoom);
    bool closeSession(const QString& sessionId);

signals:
    void sessionCreated(const QString& sessionId);
    void sessionUpdated(const QString& sessionId);
    void sessionClosed(const QString& sessionId);
    void sessionError(const QString& message);

};

} // namespace bookclub::server
