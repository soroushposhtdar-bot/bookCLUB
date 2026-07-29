// common/Interfaces/IStudySessionRepository.h
//
// Repository for group-reading study sessions.
#pragma once

#include <QString>
#include <QVector>
#include <QStringList>

namespace bookclub::common {

class StudySession;

class IStudySessionRepository {
public:
    virtual ~IStudySessionRepository() = default;

    virtual StudySession* findById(const QString& id) const = 0;
    virtual QVector<StudySession*> findByBook(const QString& bookId) const = 0;
    virtual QVector<StudySession*> findActiveByBook(const QString& bookId) const = 0;
    virtual QVector<StudySession*> findByHost(const QString& hostUserId) const = 0;
    virtual QVector<StudySession*> findByParticipant(const QString& userId) const = 0;
    virtual bool save(StudySession* session) = 0;
    virtual bool update(StudySession* session) = 0;
    virtual bool remove(const QString& id) = 0;
    virtual bool addParticipant(const QString& sessionId, const QString& userId) = 0;
    virtual bool removeParticipant(const QString& sessionId, const QString& userId) = 0;
    virtual QStringList participants(const QString& sessionId) const = 0;
};

IStudySessionRepository* createStudySessionRepository();

} // namespace bookclub::common
