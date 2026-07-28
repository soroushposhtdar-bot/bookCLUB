// src/client/controllers/StudySessionController.h
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace bookclub::common { class Message; }

namespace bookclub::client {

class ReaderController;

class StudySessionController : public QObject {
    Q_OBJECT
public:
    explicit StudySessionController(QObject* parent = nullptr);
    ~StudySessionController() override;

    // ---- Public Methods ----
    void createSession(const QString& bookId);
    void joinSession(const QString& sessionId);
    void leaveSession();
    void setCurrentPage(int page);
    void setZoom(double zoom);
    void inviteParticipant(const QString& userId);
    void syncState();

    // ---- Accessors ----
    QStringList getParticipants() const;
    int getParticipantCount() const;
    bool isParticipant(const QString& userId) const;
    QString getSessionId() const;
    QString getBookId() const;
    int getCurrentPage() const;
    double getZoom() const;
    bool isInSession() const;

    // ---- Reader Integration ----
    void setReaderController(ReaderController* reader);

signals:
    void sessionCreated(const QString& sessionId);
    void sessionJoined(const QString& sessionId);
    void sessionLeft();
    void sessionUpdated();
    void errorOccurred(const QString& message);

private:
    void handleCreateSessionResponse(const common::Message& response);
    void handleJoinSessionResponse(const common::Message& response);
    void handleLeaveSessionResponse(const common::Message& response);
    void handleSyncStateResponse(const common::Message& response);

    // ---- Private Slots for Reader Connection ----
    void onReaderPageChanged(int page);
    void onReaderZoomChanged(double zoom);

    ReaderController* m_readerController;
    QString m_sessionId;
    QString m_bookId;
    QStringList m_participants;
    int m_currentPage = 0;
    double m_zoom = 1.0;
    bool m_isInSession;
};

} // namespace bookclub::client
