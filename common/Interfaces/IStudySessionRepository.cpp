// common/Interfaces/IStudySessionRepository.cpp
#include "common/Interfaces/IStudySessionRepository.h"
#include "common/Models/StudySession.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>

namespace bookclub::common {

namespace {
StudySession* sessionFromCurrentRecord(QSqlQuery& q)
{
    auto* s = new StudySession;
    s->setId(q.value("id").toString());
    s->setBookId(q.value("bookId").toString());
    s->setHostUserId(q.value("hostUserId").toString());
    s->setState(static_cast<StudySessionState>(q.value("state").toInt()));
    s->setCurrentPage(q.value("currentPage").toInt());
    s->setZoomLevel(q.value("zoomLevel").toDouble());
    s->setCreatedAt(q.value("createdAt").toDateTime());
    return s;
}
} // namespace

class StudySessionRepositoryImpl : public IStudySessionRepository {
public:
    StudySession* findById(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT * FROM StudySessions WHERE id = ?", {id});
        if (!q.next()) return nullptr;
        auto* s = sessionFromCurrentRecord(q);
        s->setParticipantUserIds(participants(id));
        return s;
    }

    QVector<StudySession*> findByBook(const QString& bookId) const override
    {
        QVector<StudySession*> out;
        auto q = DbConnection::run(
            "SELECT * FROM StudySessions WHERE bookId = ? ORDER BY createdAt DESC",
            {bookId}
        );
        while (q.next()) {
            auto* s = sessionFromCurrentRecord(q);
            s->setParticipantUserIds(participants(s->id()));
            out.append(s);
        }
        return out;
    }

    QVector<StudySession*> findActiveByBook(const QString& bookId) const override
    {
        QVector<StudySession*> out;
        auto q = DbConnection::run(
            "SELECT * FROM StudySessions WHERE bookId = ? AND state IN (0,1) "
            "ORDER BY createdAt DESC",
            {bookId}
        );
        while (q.next()) {
            auto* s = sessionFromCurrentRecord(q);
            s->setParticipantUserIds(participants(s->id()));
            out.append(s);
        }
        return out;
    }

    QVector<StudySession*> findByHost(const QString& hostUserId) const override
    {
        QVector<StudySession*> out;
        auto q = DbConnection::run(
            "SELECT * FROM StudySessions WHERE hostUserId = ? ORDER BY createdAt DESC",
            {hostUserId}
        );
        while (q.next()) {
            auto* s = sessionFromCurrentRecord(q);
            s->setParticipantUserIds(participants(s->id()));
            out.append(s);
        }
        return out;
    }

    QVector<StudySession*> findByParticipant(const QString& userId) const override
    {
        QVector<StudySession*> out;
        auto q = DbConnection::run(
            "SELECT s.* FROM StudySessions s "
            "JOIN StudySessionParticipants p ON p.sessionId = s.id "
            "WHERE p.userId = ? ORDER BY s.createdAt DESC",
            {userId}
        );
        while (q.next()) {
            auto* s = sessionFromCurrentRecord(q);
            s->setParticipantUserIds(participants(s->id()));
            out.append(s);
        }
        return out;
    }

    bool save(StudySession* session) override
    {
        if (!session) return false;
        if (session->id().isEmpty()) session->setId(IdGenerator::generateUuid());
        if (!session->createdAt().isValid()) session->setCreatedAt(QDateTime::currentDateTime());

        return DbConnection::execOk(
            "INSERT INTO StudySessions (id, bookId, hostUserId, state, currentPage, zoomLevel, createdAt) "
            "VALUES (?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(id) DO UPDATE SET state = excluded.state, "
            "  currentPage = excluded.currentPage, zoomLevel = excluded.zoomLevel",
            {session->id(), session->bookId(), session->hostUserId(),
             static_cast<int>(session->state()),
             session->currentPage(), session->zoomLevel(),
             session->createdAt()}
        );
    }

    bool update(StudySession* session) override { return save(session); }

    bool remove(const QString& id) override
    {
        return DbConnection::execOk("DELETE FROM StudySessions WHERE id = ?", {id});
    }

    bool addParticipant(const QString& sessionId, const QString& userId) override
    {
        return DbConnection::execOk(
            "INSERT OR IGNORE INTO StudySessionParticipants (sessionId, userId) VALUES (?, ?)",
            {sessionId, userId}
        );
    }

    bool removeParticipant(const QString& sessionId, const QString& userId) override
    {
        return DbConnection::execOk(
            "DELETE FROM StudySessionParticipants WHERE sessionId = ? AND userId = ?",
            {sessionId, userId}
        );
    }

    QStringList participants(const QString& sessionId) const override
    {
        QStringList ids;
        auto q = DbConnection::run(
            "SELECT userId FROM StudySessionParticipants WHERE sessionId = ?",
            {sessionId}
        );
        while (q.next()) ids.append(q.value(0).toString());
        return ids;
    }
};

IStudySessionRepository* createStudySessionRepository() {
    static StudySessionRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
