#pragma once

#include <QObject>
#include <QString>

namespace bookclub::client {

class StudySessionController : public QObject {
    Q_OBJECT
public:
    explicit StudySessionController(QObject* parent = nullptr);
    ~StudySessionController() override = default;

    void createSession(const QString& bookId);
    void joinSession(const QString& sessionId);
    void leaveSession();
    void setCurrentPage(int page);
    void setZoom(double zoom);
    void inviteParticipant(const QString& userId);
    void syncState();

signals:
    void sessionCreated(const QString& sessionId);
    void sessionJoined(const QString& sessionId);
    void sessionLeft();
    void sessionUpdated();
    void errorOccurred(const QString& message);

};

} // namespace bookclub::client
